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

#include <tango-gl/conversions.h>
#include <tango-gl/util.h>
#include <unistd.h>

#include "mesh_builder/mesh_builder_app.h"

mesh_builder::MeshBuilderApp* app = 0;

namespace {
    const int kInitialVertexCount = 100;
    const int kInitialIndexCount = 99;
    const int kGrowthFactor = 5;
    constexpr int kTangoCoreMinimumVersion = 9377;

    void onPointCloudAvailableRouter(void *context, const TangoPointCloud *point_cloud) {
        mesh_builder::MeshBuilderApp *app = static_cast<mesh_builder::MeshBuilderApp *>(context);
        app->onPointCloudAvailable(point_cloud);
    }

    void onFrameAvailableRouter(void *context, TangoCameraId id, const TangoImageBuffer *buffer) {
        mesh_builder::MeshBuilderApp *app = static_cast<mesh_builder::MeshBuilderApp *>(context);
        app->onFrameAvailable(id, buffer);
    }

    void extract3DRPose(const glm::mat4 &mat, Tango3DR_Pose *pose) {
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
        tango_gl::util::DecomposeMatrix(mat, translation, rotation, scale);
        pose->translation[0] = translation[0];
        pose->translation[1] = translation[1];
        pose->translation[2] = translation[2];
        pose->orientation[0] = rotation[0];
        pose->orientation[1] = rotation[1];
        pose->orientation[2] = rotation[2];
        pose->orientation[3] = rotation[3];
    }

}  // namespace

namespace mesh_builder {

    bool GridIndex::operator==(const GridIndex &other) const {
        return indices[0] == other.indices[0] && indices[1] == other.indices[1] &&
               indices[2] == other.indices[2];
    }

    void MeshBuilderApp::onPointCloudAvailable(const TangoPointCloud *point_cloud) {
        if (!t3dr_is_running_)
            return;

        TangoMatrixTransformData matrix_transform;
        TangoSupport_getMatrixTransformAtTime(
                point_cloud->timestamp, TANGO_COORDINATE_FRAME_START_OF_SERVICE,
                TANGO_COORDINATE_FRAME_CAMERA_DEPTH, TANGO_SUPPORT_ENGINE_OPENGL,
                TANGO_SUPPORT_ENGINE_TANGO, ROTATION_0, &matrix_transform);
        if (matrix_transform.status_code != TANGO_POSE_VALID)
            return;

        binder_mutex_.lock();
        point_cloud_matrix_ = glm::make_mat4(matrix_transform.matrix);
        TangoSupport_updatePointCloud(point_cloud_manager_, point_cloud);
        point_cloud_available_ = true;
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::onFrameAvailable(TangoCameraId id, const TangoImageBuffer *buffer) {
        if (id != TANGO_CAMERA_COLOR || !t3dr_is_running_)
            return;

        binder_mutex_.lock();
        if (!point_cloud_available_) {
            binder_mutex_.unlock();
            return;
        }

        // Get the camera color transform to OpenGL world frame in OpenGL convention.
        TangoMatrixTransformData matrix_transform;
        TangoSupport_getMatrixTransformAtTime(
                buffer->timestamp, TANGO_COORDINATE_FRAME_START_OF_SERVICE,
                TANGO_COORDINATE_FRAME_CAMERA_COLOR, TANGO_SUPPORT_ENGINE_OPENGL,
                TANGO_SUPPORT_ENGINE_TANGO, ROTATION_0, &matrix_transform);
        if (matrix_transform.status_code != TANGO_POSE_VALID) {
            binder_mutex_.unlock();
            return;
        }

        if(!process_mutex_.try_lock()) {
            binder_mutex_.unlock();
            return;
        }
        glm::mat4 image_matrix = glm::make_mat4(matrix_transform.matrix);
        Tango3DR_ImageBuffer t3dr_image;
        t3dr_image.width = buffer->width;
        t3dr_image.height = buffer->height;
        t3dr_image.stride = buffer->stride;
        t3dr_image.timestamp = buffer->timestamp;
        t3dr_image.format = static_cast<Tango3DR_ImageFormatType>(buffer->format);
        t3dr_image.data = buffer->data;

        Tango3DR_Pose t3dr_image_pose;
        extract3DRPose(image_matrix, &t3dr_image_pose);

        Tango3DR_PointCloud t3dr_depth;
        TangoSupport_getLatestPointCloud(point_cloud_manager_, &front_cloud_);
        t3dr_depth.timestamp = front_cloud_->timestamp;
        t3dr_depth.num_points = front_cloud_->num_points;
        t3dr_depth.points = front_cloud_->points;

        Tango3DR_Pose t3dr_depth_pose;
        extract3DRPose(point_cloud_matrix_, &t3dr_depth_pose);

        Tango3DR_GridIndexArray *t3dr_updated;
        Tango3DR_Status t3dr_err =
                Tango3DR_update(t3dr_context_, &t3dr_depth, &t3dr_depth_pose, &t3dr_image,
                                &t3dr_image_pose, &t3dr_updated);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            return;

        updated_indices_binder_thread_.resize(t3dr_updated->num_indices);
        std::copy(&t3dr_updated->indices[0][0], &t3dr_updated->indices[t3dr_updated->num_indices][0],
                  reinterpret_cast<uint32_t *>(updated_indices_binder_thread_.data()));

        Tango3DR_GridIndexArray_destroy(t3dr_updated);
        point_cloud_available_ = false;

        app = this;
        for(long i = 0; i < THREAD_COUNT; i++) {
            threadDone[i] = false;
            struct thread_info *info = (struct thread_info *) i;
            pthread_create(&threadId[i], NULL, Process, info);
        }
        bool done = false;
        while(!done) {
            done = true;
            for(int i = 0; i < THREAD_COUNT; i++) {
                threadMutex[i].lock();
                if(!threadDone[i])
                  done = false;
                threadMutex[i].unlock();
            }
            usleep(10);
        }
        process_mutex_.unlock();
        binder_mutex_.unlock();
    }

    MeshBuilderApp::MeshBuilderApp() {
        zoom = 0;
    }

    MeshBuilderApp::~MeshBuilderApp() {
        if (tango_config_ != nullptr) {
            TangoConfig_free(tango_config_);
            tango_config_ = nullptr;
        }
        if (point_cloud_manager_ != nullptr) {
            TangoSupport_freePointCloudManager(point_cloud_manager_);
            point_cloud_manager_ = nullptr;
        }
    }

    void MeshBuilderApp::ActivityCtor(bool t3dr_is_running) {
        t3dr_is_running_ = t3dr_is_running;
    }

    void MeshBuilderApp::OnCreate(JNIEnv *env, jobject activity) {
        int version;
        TangoErrorType err = TangoSupport_GetTangoVersion(env, activity, &version);
        if (err != TANGO_SUCCESS || version < kTangoCoreMinimumVersion)
            std::exit(EXIT_SUCCESS);
    }

    void MeshBuilderApp::OnTangoServiceConnected(JNIEnv *env, jobject binder) {
        TangoService_setBinder(env, binder);
        TangoSetupConfig();
        TangoConnectCallbacks();
        TangoConnect();
        TangoSetup3DR(0.03, 0.5, 3);
    }

    void MeshBuilderApp::TangoSetupConfig() {
        tango_config_ = TangoService_getConfig(TANGO_CONFIG_DEFAULT);

        // This enables basic motion tracking capabilities.
        if (tango_config_ == nullptr)
            std::exit(EXIT_SUCCESS);

        // Set auto-recovery for motion tracking as requested by the user.
        int ret = TangoConfig_setBool(tango_config_, "config_enable_auto_recovery", true);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Enable depth.
        ret = TangoConfig_setBool(tango_config_, "config_enable_depth", true);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Enable learning.
        ret = TangoConfig_setBool(tango_config_, "config_enable_learning_mode", true);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Need to specify the depth_mode as XYZC.
        ret = TangoConfig_setInt32(tango_config_, "config_depth_mode", TANGO_POINTCLOUD_XYZC);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Enable color camera.
        ret = TangoConfig_setBool(tango_config_, "config_enable_color_camera", true);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        if (point_cloud_manager_ == nullptr) {
            int32_t max_point_cloud;
            ret = TangoConfig_getInt32(tango_config_, "max_point_cloud_elements", &max_point_cloud);
            if (ret != TANGO_SUCCESS)
                std::exit(EXIT_SUCCESS);

            size_t max_point_cloud_elements = (size_t) max_point_cloud;
            ret = TangoSupport_createPointCloudManager(max_point_cloud_elements, &point_cloud_manager_);
            if (ret != TANGO_SUCCESS)
                std::exit(EXIT_SUCCESS);
        }
    }

    void MeshBuilderApp::TangoSetup3DR(double res, double dmin, double dmax) {
        binder_mutex_.lock();
        if(t3dr_context_ != nullptr)
            Tango3DR_destroy(t3dr_context_);
        Tango3DR_ConfigH t3dr_config = Tango3DR_Config_create(TANGO_3DR_CONFIG_CONTEXT);
        Tango3DR_Status t3dr_err;
        t3dr_err = Tango3DR_Config_setDouble(t3dr_config, "resolution", res);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setDouble(t3dr_config, "min_depth", dmin);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setDouble(t3dr_config, "max_depth", dmax);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setBool(t3dr_config, "generate_color", true);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setBool(t3dr_config, "use_parallel_integration", true);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_context_ = Tango3DR_create(t3dr_config);
        if (t3dr_context_ == nullptr)
            std::exit(EXIT_SUCCESS);

        Tango3DR_setColorCalibration(t3dr_context_, &t3dr_intrinsics_);
        Tango3DR_Config_destroy(t3dr_config);
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::TangoConnectCallbacks() {
        TangoErrorType ret = TangoService_connectOnPointCloudAvailable(onPointCloudAvailableRouter);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        ret = TangoService_connectOnFrameAvailable(TANGO_CAMERA_COLOR, this, onFrameAvailableRouter);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
    }

    void MeshBuilderApp::TangoConnect() {
        TangoErrorType err = TangoService_connect(this, tango_config_);
        if (err != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Initialize TangoSupport context.
        TangoSupport_initializeLibrary();

        // Update the camera intrinsics too.
        TangoCameraIntrinsics intrinsics;
        err = TangoService_getCameraIntrinsics(TANGO_CAMERA_COLOR, &intrinsics);
        if (err != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        t3dr_intrinsics_.calibration_type =
                static_cast<Tango3DR_TangoCalibrationType>(intrinsics.calibration_type);
        t3dr_intrinsics_.width = intrinsics.width;
        t3dr_intrinsics_.height = intrinsics.height;
        t3dr_intrinsics_.fx = intrinsics.fx;
        t3dr_intrinsics_.fy = intrinsics.fy;
        t3dr_intrinsics_.cx = intrinsics.cx;
        t3dr_intrinsics_.cy = intrinsics.cy;
        std::copy(std::begin(intrinsics.distortion), std::end(intrinsics.distortion),
                  std::begin(t3dr_intrinsics_.distortion));
    }

    void MeshBuilderApp::TangoDisconnect() {
        TangoConfig_free(tango_config_);
        tango_config_ = nullptr;
        TangoService_disconnect();
    }

    void MeshBuilderApp::OnPause() {
        TangoDisconnect();
        DeleteResources();

        // Since motion tracking is lost when disconnected from Tango, any
        // existing 3D reconstruction state no longer is lined up with the
        // real world. Best we can do is clear the state.
        OnClearButtonClicked();
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

        TangoMatrixTransformData matrix_transform;
        TangoSupport_getMatrixTransformAtTime(
                0, TANGO_COORDINATE_FRAME_START_OF_SERVICE, TANGO_COORDINATE_FRAME_DEVICE,
                TANGO_SUPPORT_ENGINE_OPENGL, TANGO_SUPPORT_ENGINE_OPENGL, ROTATION_0,
                &matrix_transform);
        if (matrix_transform.status_code == TANGO_POSE_VALID)
            start_service_T_device_ = glm::make_mat4(matrix_transform.matrix);

        render_mutex_.lock();
        main_scene_.camera_->SetTransformationMatrix(start_service_T_device_);
        main_scene_.UpdateFrustum(main_scene_.camera_->GetPosition(), zoom);
        //zoom
        glm::vec4 move = main_scene_.camera_->GetTransformationMatrix() * glm::vec4(0, 0, zoom, 0);
        main_scene_.camera_->Translate(glm::vec3(move.x, move.y, move.z));
        //render
        main_scene_.Render();
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
        Tango3DR_clear(t3dr_context_);
        meshes_.clear();
        main_scene_.ClearDynamicMeshes();
        binder_mutex_.unlock();
    }


    void* MeshBuilderApp::Process(void *ptr)
    {
        long thread = (long)ptr;
        app->threadMutex[thread].lock();
        unsigned long it = app->updated_indices_binder_thread_.size() * thread / THREAD_COUNT;
        unsigned long end = app->updated_indices_binder_thread_.size() * (thread + 1) / THREAD_COUNT;
        while (it < end) {
            GridIndex updated_index = app->updated_indices_binder_thread_[it];

            // Build a dynamic mesh and add it to the scene.
            app->add_mutex_.lock();
            std::shared_ptr<SingleDynamicMesh> &dynamic_mesh = app->meshes_[updated_index];
            if (dynamic_mesh == nullptr) {
                dynamic_mesh = std::make_shared<SingleDynamicMesh>();
                dynamic_mesh->mesh.render_mode = GL_TRIANGLES;
                dynamic_mesh->mesh.vertices.resize(kInitialVertexCount);
                dynamic_mesh->mesh.colors.resize(kInitialVertexCount);
                dynamic_mesh->mesh.uv.resize(kInitialVertexCount);
                dynamic_mesh->mesh.indices.resize(kInitialIndexCount);
                app->render_mutex_.lock();
                app->main_scene_.AddDynamicMesh(dynamic_mesh.get());
                app->render_mutex_.unlock();
            }
            dynamic_mesh->mutex.lock();
            app->add_mutex_.unlock();

            dynamic_mesh->mesh.vertices.resize(dynamic_mesh->mesh.vertices.capacity());
            dynamic_mesh->mesh.colors.resize(dynamic_mesh->mesh.colors.capacity());
            dynamic_mesh->mesh.uv.resize(dynamic_mesh->mesh.uv.capacity());
            dynamic_mesh->mesh.indices.resize(dynamic_mesh->mesh.indices.capacity());

            Tango3DR_Mesh tango_mesh = {
                    /* timestamp */ 0.0,
                    /* num_vertices */ 0u,
                    /* num_faces */ 0u,
                    /* num_textures */ 0u,
                    /* max_num_vertices */ static_cast<uint32_t>(dynamic_mesh->mesh.vertices.capacity()),
                    /* max_num_faces */static_cast<uint32_t>(dynamic_mesh->mesh.indices.capacity() / 3),
                    /* max_num_textures */ 0u,
                    /* vertices */ reinterpret_cast<Tango3DR_Vector3 *>(dynamic_mesh->mesh.vertices.data()),
                    /* faces */ reinterpret_cast<Tango3DR_Face *>(dynamic_mesh->mesh.indices.data()),
                    /* normals */ nullptr,
                    /* colors */ reinterpret_cast<Tango3DR_Color *>(dynamic_mesh->mesh.colors.data()),
                    /* texture_coords */ reinterpret_cast<Tango3DR_TexCoord *>(dynamic_mesh->mesh.uv.data()),
                    /* texture_ids */ nullptr,
                    /* textures */ nullptr};

            Tango3DR_Status err = Tango3DR_extractPreallocatedMeshSegment(
                    app->t3dr_context_, updated_index.indices, &tango_mesh);
            if (err == TANGO_3DR_INSUFFICIENT_SPACE) {
                unsigned long new_vertex_size = dynamic_mesh->mesh.vertices.capacity() * kGrowthFactor;
                unsigned long new_index_size = dynamic_mesh->mesh.indices.capacity() * kGrowthFactor;
                new_index_size -= new_index_size % 3;
                dynamic_mesh->mesh.vertices.resize(new_vertex_size);
                dynamic_mesh->mesh.colors.resize(new_vertex_size);
                dynamic_mesh->mesh.uv.resize(new_vertex_size);
                dynamic_mesh->mesh.indices.resize(new_index_size);
            } else
                ++it;
            dynamic_mesh->mesh.vertices.resize(tango_mesh.num_vertices);
            dynamic_mesh->mesh.colors.resize(tango_mesh.num_vertices);
            dynamic_mesh->mesh.uv.resize(tango_mesh.num_vertices);
            dynamic_mesh->mesh.indices.resize(tango_mesh.num_faces * 3);
            dynamic_mesh->mutex.unlock();
        }
        app->threadMutex[thread].unlock();
        app->threadDone[thread] = true;
        pthread_detach(app->threadId[thread]);
        return 0;
    }

    void MeshBuilderApp::Save(std::string filename)
    {
        binder_mutex_.lock();
        process_mutex_.lock();
        LOGI("Writing into %s", filename.c_str());
        //count vertices and faces
        int vertexCount = 0;
        int faceCount = 0;
        for(unsigned int i = 0; i < main_scene_.dynamic_meshes_.size(); i++) {
            vertexCount += main_scene_.dynamic_meshes_[i]->mesh.vertices.size();
            faceCount += main_scene_.dynamic_meshes_[i]->mesh.indices.size() / 3;
        }

        //file header
        FILE* file = fopen(filename.c_str(), "w");
        fprintf(file, "ply\nformat ascii 1.0\ncomment ---\n");
        fprintf(file, "element vertex %d\n", vertexCount);
        fprintf(file, "property float x\n");
        fprintf(file, "property float y\n");
        fprintf(file, "property float z\n");
        fprintf(file, "property uchar red\n");
        fprintf(file, "property uchar green\n");
        fprintf(file, "property uchar blue\n");
        fprintf(file, "element face %d\n", faceCount);
        fprintf(file, "property list uchar uint vertex_indices\n");
        fprintf(file, "end_header\n");
        //write vertices
        for(unsigned int i = 0; i < main_scene_.dynamic_meshes_.size(); i++) {
            for(unsigned int j = 0; j < main_scene_.dynamic_meshes_[i]->mesh.vertices.size(); j++) {
                glm::vec3 v = main_scene_.dynamic_meshes_[i]->mesh.vertices[j];
                unsigned int c = main_scene_.dynamic_meshes_[i]->mesh.colors[j];
                unsigned int r = (c & 0x000000FF);
                unsigned int g = (c & 0x0000FF00) >> 8;
                unsigned int b = (c & 0x00FF0000) >> 16;
                fprintf(file, "%f %f %f %d %d %d\n", -v.x, v.z, v.y, r, g, b);
            }
        }
        //write faces
        int offset = 0;
        for(unsigned int i = 0; i < main_scene_.dynamic_meshes_.size(); i++) {
            for (unsigned int j = 0; j < main_scene_.dynamic_meshes_[i]->mesh.indices.size(); j+=3) {
                unsigned int a = main_scene_.dynamic_meshes_[i]->mesh.indices[j + 0] + offset;
                unsigned int b = main_scene_.dynamic_meshes_[i]->mesh.indices[j + 1] + offset;
                unsigned int c = main_scene_.dynamic_meshes_[i]->mesh.indices[j + 2] + offset;
                fprintf(file, "3 %d %d %d\n", a, b, c);
            }
            offset += main_scene_.dynamic_meshes_[i]->mesh.vertices.size();
        }
        fclose(file);
        process_mutex_.unlock();
        binder_mutex_.unlock();
    }
}  // namespace mesh_builder
