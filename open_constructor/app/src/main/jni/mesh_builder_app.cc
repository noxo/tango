/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <map>
#include <sstream>
#include "mesh_builder_app.h"

#define PNG_TEXTURE_SCALE 4

namespace {
    const int kSubdivisionSize = 5000;
    const int kInitialVertexCount = 30;
    const int kInitialIndexCount = 99;
    const int kGrowthFactor = 2;
    constexpr int kTangoCoreMinimumVersion = 9377;

    void onPointCloudAvailableRouter(void *context, const TangoPointCloud *point_cloud) {
        oc::MeshBuilderApp *app = static_cast<oc::MeshBuilderApp *>(context);
        app->onPointCloudAvailable((TangoPointCloud*)point_cloud);
    }

    void onFrameAvailableRouter(void *context, TangoCameraId id, const TangoImageBuffer *buffer) {
        oc::MeshBuilderApp *app = static_cast<oc::MeshBuilderApp *>(context);
        app->onFrameAvailable(id, buffer);
    }
}

namespace oc {

    bool GridIndex::operator==(const GridIndex &other) const {
        return indices[0] == other.indices[0] && indices[1] == other.indices[1] &&
               indices[2] == other.indices[2];
    }

    void MeshBuilderApp::onPointCloudAvailable(TangoPointCloud *point_cloud) {
        if (!t3dr_is_running_)
            return;

        TangoMatrixTransformData matrix_transform;
        TangoSupport_getMatrixTransformAtTime(
                point_cloud->timestamp, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION,
                TANGO_COORDINATE_FRAME_CAMERA_DEPTH, TANGO_SUPPORT_ENGINE_OPENGL,
                TANGO_SUPPORT_ENGINE_TANGO, ROTATION_0, &matrix_transform);
        if (matrix_transform.status_code != TANGO_POSE_VALID)
            return;

        binder_mutex_.lock();
        point_cloud_matrix_ = glm::make_mat4(matrix_transform.matrix);
        TangoSupport_updatePointCloud(tango.Pointcloud(), point_cloud);
        point_cloud_available_ = true;
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::onFrameAvailable(TangoCameraId id, const TangoImageBuffer *buffer) {
        if (id != TANGO_CAMERA_COLOR || !t3dr_is_running_)
            return;

        TangoMatrixTransformData matrix_transform;
        TangoSupport_getMatrixTransformAtTime(
                        buffer->timestamp, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION,
                        TANGO_COORDINATE_FRAME_CAMERA_COLOR, TANGO_SUPPORT_ENGINE_OPENGL,
                        TANGO_SUPPORT_ENGINE_TANGO, ROTATION_0, &matrix_transform);
        if (matrix_transform.status_code != TANGO_POSE_VALID)
            return;

        binder_mutex_.lock();
        if (!point_cloud_available_) {
            binder_mutex_.unlock();
            return;
        }

        image_matrix = glm::make_mat4(matrix_transform.matrix);
        Tango3DR_ImageBuffer t3dr_image;
        t3dr_image.width = buffer->width;
        t3dr_image.height = buffer->height;
        t3dr_image.stride = buffer->stride;
        t3dr_image.timestamp = buffer->timestamp;
        t3dr_image.format = static_cast<Tango3DR_ImageFormatType>(buffer->format);
        t3dr_image.data = buffer->data;

        Tango3DR_Pose t3dr_image_pose = GLCamera::Extract3DRPose(image_matrix);
        glm::quat rot = glm::quat((float) t3dr_image_pose.orientation[0],
                                  (float) t3dr_image_pose.orientation[1],
                                  (float) t3dr_image_pose.orientation[2],
                                  (float) t3dr_image_pose.orientation[3]);
        float diff = GLCamera::Diff(rot, image_rotation);
        image_rotation = rot;
        if (diff > 1) {
            binder_mutex_.unlock();
            return;
        }

        Tango3DR_PointCloud t3dr_depth;
        TangoSupport_getLatestPointCloud(tango.Pointcloud(), &front_cloud_);
        t3dr_depth.timestamp = front_cloud_->timestamp;
        t3dr_depth.num_points = front_cloud_->num_points;
        t3dr_depth.points = front_cloud_->points;

        Tango3DR_Pose t3dr_depth_pose = GLCamera::Extract3DRPose(point_cloud_matrix_);
        Tango3DR_GridIndexArray t3dr_updated;
        Tango3DR_Status ret;
        ret = Tango3DR_update(tango.Context(), &t3dr_depth, &t3dr_depth_pose,
                              &t3dr_image, &t3dr_image_pose, &t3dr_updated);
        if (ret != TANGO_3DR_SUCCESS)
        {
            binder_mutex_.unlock();
            return;
        }

        Image frame(t3dr_image, PNG_TEXTURE_SCALE);
        frame.Write(GetFileName(poses_, ".png").c_str());
        FILE* file = fopen(GetFileName(poses_, ".txt").c_str(), "w");
        for (int i = 0; i < 4; i++)
            fprintf(file, "%f %f %f %f\n", image_matrix[i][0], image_matrix[i][1],
                                           image_matrix[i][2], image_matrix[i][3]);
        fprintf(file, "%lf\n", t3dr_image.timestamp);
        fclose(file);
        poses_++;

        MeshUpdate(&t3dr_updated);
        Tango3DR_GridIndexArray_destroy(&t3dr_updated);
        point_cloud_available_ = false;
        binder_mutex_.unlock();
    }


    MeshBuilderApp::MeshBuilderApp() :  t3dr_is_running_(false),
                                        gyro(false),
                                        landscape(false),
                                        point_cloud_available_(false),
                                        poses_(0),
                                        zoom(0) {}

    void MeshBuilderApp::OnCreate(JNIEnv *env, jobject activity) {
        int version;
        TangoErrorType err = TangoSupport_GetTangoVersion(env, activity, &version);
        if (err != TANGO_SUCCESS || version < kTangoCoreMinimumVersion)
            std::exit(EXIT_SUCCESS);
    }

    void MeshBuilderApp::OnTangoServiceConnected(JNIEnv *env, jobject binder, double res,
               double dmin, double dmax, int noise, bool land, std::string dataset) {
        landscape = land;

        TangoService_setBinder(env, binder);
        tango.SetupConfig(dataset);

        TangoErrorType ret = TangoService_connectOnPointCloudAvailable(onPointCloudAvailableRouter);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = TangoService_connectOnFrameAvailable(TANGO_CAMERA_COLOR, this, onFrameAvailableRouter);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        binder_mutex_.lock();
        tango.Connect(this);
        tango.Setup3DR(res, dmin, dmax, noise);
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::OnPause() {
        tango.Disconnect();
        DeleteResources();
    }

    void MeshBuilderApp::OnSurfaceCreated() {
        render_mutex_.lock();
        main_scene_.InitGLContent();
        render_mutex_.unlock();
    }

    void MeshBuilderApp::OnSurfaceChanged(int width, int height) {
        render_mutex_.lock();
        main_scene_.SetupViewPort(width, height);
        render_mutex_.unlock();
    }

    void MeshBuilderApp::OnDrawFrame() {
        render_mutex_.lock();
        //camera transformation
        if (!gyro) {
            main_scene_.renderer->camera.position = glm::vec3(movex, 0, movey);
            main_scene_.renderer->camera.rotation = glm::quat(glm::vec3(yaw, pitch, 0));
            main_scene_.renderer->camera.scale    = glm::vec3(1, 1, 1);
        } else {
            TangoMatrixTransformData transform;
            TangoSupport_getMatrixTransformAtTime(
                    0, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION, TANGO_COORDINATE_FRAME_DEVICE,
                    TANGO_SUPPORT_ENGINE_OPENGL, TANGO_SUPPORT_ENGINE_OPENGL,
                    landscape ? ROTATION_90 : ROTATION_0, &transform);
            if (transform.status_code == TANGO_POSE_VALID) {
                main_scene_.renderer->camera.SetTransformation(glm::make_mat4(transform.matrix));
                main_scene_.UpdateFrustum(main_scene_.renderer->camera.position, zoom);
            }
        }
        //zoom
        glm::vec4 move = main_scene_.renderer->camera.GetTransformation() * glm::vec4(0, 0, zoom, 0);
        main_scene_.renderer->camera.position += glm::vec3(move.x, move.y, move.z);
        //render
        main_scene_.Render(gyro);
        render_mutex_.unlock();
    }

    void MeshBuilderApp::DeleteResources() {
        render_mutex_.lock();
        main_scene_.DeleteResources();
        render_mutex_.unlock();
    }

    void MeshBuilderApp::OnToggleButtonClicked(bool t3dr_is_running) {
        binder_mutex_.lock();
        t3dr_is_running_ = t3dr_is_running;
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::OnClearButtonClicked() {
        binder_mutex_.lock();
        render_mutex_.lock();
        tango.Clear();
        meshes_.clear();
        poses_ = 0;
        main_scene_.ClearDynamicMeshes();
        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::Load(std::string filename) {
        binder_mutex_.lock();
        render_mutex_.lock();
        File3d io(filename, false);
        io.ReadModel(kSubdivisionSize, main_scene_.static_meshes_);
        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::Save(std::string filename, std::string dataset) {
        binder_mutex_.lock();
        render_mutex_.lock();
        if (!dataset.empty()) {
            //get mesh to texture
            Tango3DR_Mesh mesh;
            Tango3DR_Status ret;
            ret = Tango3DR_Mesh_initEmpty(&mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            ret = Tango3DR_extractFullMesh(tango.Context(), &mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);

            //prevent crash on saving empty model
            if (mesh.num_faces != 0) {
                //get texturing context
                Tango3DR_Config textureConfig;
                textureConfig = Tango3DR_Config_create(TANGO_3DR_CONFIG_TEXTURING);
                ret = Tango3DR_Config_setDouble(textureConfig, "min_resolution", 0.01);
                if (ret != TANGO_3DR_SUCCESS)
                    std::exit(EXIT_SUCCESS);
                ret = Tango3DR_Config_setInt32(textureConfig, "texturing_backend", TANGO_3DR_CPU_TEXTURING);
                if (ret != TANGO_3DR_SUCCESS)
                    std::exit(EXIT_SUCCESS);
                Tango3DR_TexturingContext context;
                context = Tango3DR_TexturingContext_create(textureConfig, dataset.c_str(), &mesh);
                if (context == nullptr)
                    std::exit(EXIT_SUCCESS);
                Tango3DR_Config_destroy(textureConfig);

                //texturize mesh
                ret = Tango3DR_getTexturedMesh(context, &mesh);
                if (ret != TANGO_3DR_SUCCESS)
                    std::exit(EXIT_SUCCESS);

                //save and cleanup
                ret = Tango3DR_Mesh_saveToObj(&mesh, filename.c_str());
                if (ret != TANGO_3DR_SUCCESS)
                    std::exit(EXIT_SUCCESS);
                ret = Tango3DR_TexturingContext_destroy(context);
                if (ret != TANGO_3DR_SUCCESS)
                    std::exit(EXIT_SUCCESS);
                ret = Tango3DR_Mesh_destroy(&mesh);
                if (ret != TANGO_3DR_SUCCESS)
                    std::exit(EXIT_SUCCESS);

                //empty context and merge with previous OBJ
                meshes_.clear();
                tango.Clear();
                {
                    File3d io(filename, false);
                    io.ReadModel(kSubdivisionSize, main_scene_.static_meshes_);
                }
            }

            for (unsigned int i = 0; i < main_scene_.static_meshes_.size(); i++)
            {
              main_scene_.static_meshes_[i].indices.clear();
              for (unsigned int j = 0; j < main_scene_.static_meshes_[i].vertices.size(); j++)
                  main_scene_.static_meshes_[i].indices.push_back(j);
            }
            File3d io(filename, true);
            io.WriteModel(main_scene_.static_meshes_);
            for (unsigned int i = 0; i < main_scene_.static_meshes_.size(); i++)
                main_scene_.static_meshes_[i].indices.clear();
            main_scene_.ClearDynamicMeshes();
        }
        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    float MeshBuilderApp::CenterOfStaticModel(bool horizontal) {
        float min = 99999999;
        float max = -99999999;
        for (Mesh& mesh : main_scene_.static_meshes_) {
            for (glm::vec3 vec : mesh.vertices) {
                float value = horizontal ? vec.x : vec.z;
                if (min > value)
                    min = value;
                if (max < value)
                    max = value;
            }
        }
        return (min + max) * 0.5f;
    }

    void MeshBuilderApp::MeshUpdate(Tango3DR_GridIndexArray *t3dr_updated) {
        for (unsigned long it = 0; it < t3dr_updated->num_indices; ++it) {
            GridIndex updated_index;
            updated_index.indices[0] = t3dr_updated->indices[it][0];
            updated_index.indices[1] = t3dr_updated->indices[it][1];
            updated_index.indices[2] = t3dr_updated->indices[it][2];

            // Build a dynamic mesh and add it to the scene.
            SingleDynamicMesh* dynamic_mesh = meshes_[updated_index];
            if (dynamic_mesh == nullptr) {
                dynamic_mesh = new SingleDynamicMesh();
                dynamic_mesh->mesh.vertices.resize(kInitialVertexCount * 3);
                dynamic_mesh->mesh.colors.resize(kInitialVertexCount * 3);
                dynamic_mesh->mesh.indices.resize(kInitialIndexCount);
                dynamic_mesh->tango_mesh = {
                        /* timestamp */ 0.0,
                        /* num_vertices */ 0u,
                        /* num_faces */ 0u,
                        /* num_textures */ 0u,
                        /* max_num_vertices */ static_cast<uint32_t>(dynamic_mesh->mesh.vertices.capacity()),
                        /* max_num_faces */ static_cast<uint32_t>(dynamic_mesh->mesh.indices.capacity() / 3),
                        /* max_num_textures */ 0u,
                        /* vertices */ reinterpret_cast<Tango3DR_Vector3 *>(dynamic_mesh->mesh.vertices.data()),
                        /* faces */ reinterpret_cast<Tango3DR_Face *>(dynamic_mesh->mesh.indices.data()),
                        /* normals */ nullptr,
                        /* colors */ reinterpret_cast<Tango3DR_Color *>(dynamic_mesh->mesh.colors.data()),
                        /* texture_coords */ nullptr,
                        /* texture_ids */ nullptr,
                        /* textures */ nullptr};
                render_mutex_.lock();
                main_scene_.AddDynamicMesh(dynamic_mesh);
                render_mutex_.unlock();
            }
            dynamic_mesh->mutex.lock();

            while (true) {
                Tango3DR_Status ret;
                ret = Tango3DR_extractPreallocatedMeshSegment(tango.Context(), updated_index.indices,
                                                              &dynamic_mesh->tango_mesh);
                if (ret == TANGO_3DR_INSUFFICIENT_SPACE) {
                    unsigned long new_vertex_size = dynamic_mesh->mesh.vertices.capacity() * kGrowthFactor;
                    unsigned long new_index_size = dynamic_mesh->mesh.indices.capacity() * kGrowthFactor;
                    new_index_size -= new_index_size % 3;
                    dynamic_mesh->mesh.vertices.resize(new_vertex_size);
                    dynamic_mesh->mesh.colors.resize(new_vertex_size);
                    dynamic_mesh->mesh.indices.resize(new_index_size);
                    dynamic_mesh->tango_mesh.max_num_vertices = static_cast<uint32_t>(dynamic_mesh->mesh.vertices.capacity());
                    dynamic_mesh->tango_mesh.max_num_faces = static_cast<uint32_t>(dynamic_mesh->mesh.indices.capacity() / 3);
                    dynamic_mesh->tango_mesh.vertices = reinterpret_cast<Tango3DR_Vector3 *>(dynamic_mesh->mesh.vertices.data());
                    dynamic_mesh->tango_mesh.colors = reinterpret_cast<Tango3DR_Color *>(dynamic_mesh->mesh.colors.data());
                    dynamic_mesh->tango_mesh.faces = reinterpret_cast<Tango3DR_Face *>(dynamic_mesh->mesh.indices.data());
                } else
                    break;
            }
            dynamic_mesh->size = dynamic_mesh->tango_mesh.num_faces * 3;
            dynamic_mesh->mesh.texture = -1;
            dynamic_mesh->mutex.unlock();
        }
    }

    std::string MeshBuilderApp::GetFileName(int index, std::string extension) {
        std::ostringstream ss;
        ss << tango.Dataset().c_str();
        ss << "/";
        ss << index;
        ss << extension.c_str();
        return ss.str();
    }

    void MeshBuilderApp::Texturize(std::string filename, std::string dataset) {
        binder_mutex_.lock();
        render_mutex_.lock();
        if (!dataset.empty()) {
            //get mesh to texture
            Tango3DR_Mesh mesh;
            Tango3DR_Status ret;
            ret = Tango3DR_Mesh_initEmpty(&mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            ret = Tango3DR_Mesh_loadFromObj(filename.c_str(), &mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);

            //prevent crash on saving empty model
            if (mesh.num_faces == 0) {
                ret = Tango3DR_Mesh_destroy(&mesh);
                if (ret != TANGO_3DR_SUCCESS)
                    std::exit(EXIT_SUCCESS);
                render_mutex_.unlock();
                binder_mutex_.unlock();
                return;
            }

            //get texturing context
            Tango3DR_Config textureConfig;
            textureConfig = Tango3DR_Config_create(TANGO_3DR_CONFIG_TEXTURING);
            ret = Tango3DR_Config_setDouble(textureConfig, "min_resolution", 0.01);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            ret = Tango3DR_Config_setInt32(textureConfig, "texturing_backend", TANGO_3DR_GL_TEXTURING);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            Tango3DR_TexturingContext context;
            context = Tango3DR_TexturingContext_create(textureConfig, dataset.c_str(), &mesh);
            if (context == nullptr)
                std::exit(EXIT_SUCCESS);
            Tango3DR_Config_destroy(textureConfig);

            //update texturing context using stored PNGs
            for (unsigned int i = 0; i < poses_; i++) {
                glm::mat4 mat;
                double timestamp;
                FILE* file = fopen(GetFileName(i, ".txt").c_str(), "r");
                for (int i = 0; i < 4; i++)
                    fscanf(file, "%f %f %f %f\n", &mat[i][0], &mat[i][1], &mat[i][2], &mat[i][3]);
                fscanf(file, "%lf\n", &timestamp);
                fclose(file);

                Image frame(GetFileName(i, ".png"));
                Tango3DR_ImageBuffer image;
                image.width = frame.GetWidth() * PNG_TEXTURE_SCALE;
                image.height = frame.GetHeight() * PNG_TEXTURE_SCALE;
                image.stride = frame.GetWidth() * PNG_TEXTURE_SCALE;
                image.timestamp = timestamp;
                image.format = TANGO_3DR_HAL_PIXEL_FORMAT_YCrCb_420_SP;
                image.data = frame.ExtractYUV(PNG_TEXTURE_SCALE);
                Tango3DR_Pose t3dr_image_pose = GLCamera::Extract3DRPose(mat);
                ret = Tango3DR_updateTexture(context, &image, &t3dr_image_pose);
                if (ret != TANGO_3DR_SUCCESS)
                    std::exit(EXIT_SUCCESS);
                delete[] image.data;
            }

            //texturize mesh
            ret = Tango3DR_getTexturedMesh(context, &mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);

            //save and cleanup
            ret = Tango3DR_Mesh_saveToObj(&mesh, filename.c_str());
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            ret = Tango3DR_TexturingContext_destroy(context);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            ret = Tango3DR_Mesh_destroy(&mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);

            //reload the model
            tango.Clear();
            meshes_.clear();
            poses_ = 0;
            main_scene_.static_meshes_.clear();
            main_scene_.ClearDynamicMeshes();
            {
                File3d io(filename, false);
                io.ReadModel(kSubdivisionSize, main_scene_.static_meshes_);
            }
        }
        render_mutex_.unlock();
        binder_mutex_.unlock();
    }
}
