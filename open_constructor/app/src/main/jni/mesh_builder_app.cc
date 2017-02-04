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

#include <glm/gtx/transform.hpp>
#include <tango-gl/conversions.h>
#include <tango-gl/util.h>
#include <map>
#include <sstream>

#include "mesh_builder_app.h"
#include "vertex_processor.h"

namespace {
    const int kSubdivisionSize = 5000;
    const int kInitialVertexCount = 30;
    const int kInitialIndexCount = 99;
    const int kGrowthFactor = 2;
    constexpr int kTangoCoreMinimumVersion = 9377;

    void onPointCloudAvailableRouter(void *context, const TangoPointCloud *point_cloud) {
        mesh_builder::MeshBuilderApp *app = static_cast<mesh_builder::MeshBuilderApp *>(context);
        app->onPointCloudAvailable((TangoPointCloud*)point_cloud);
    }

    void onFrameAvailableRouter(void *context, TangoCameraId id, const TangoImageBuffer *buffer) {
        mesh_builder::MeshBuilderApp *app = static_cast<mesh_builder::MeshBuilderApp *>(context);
        app->onFrameAvailable(id, buffer);
    }

    Tango3DR_Pose extract3DRPose(const glm::mat4 &mat) {
        Tango3DR_Pose pose;
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
        tango_gl::util::DecomposeMatrix(mat, translation, rotation, scale);
        pose.translation[0] = translation[0];
        pose.translation[1] = translation[1];
        pose.translation[2] = translation[2];
        pose.orientation[0] = rotation[0];
        pose.orientation[1] = rotation[1];
        pose.orientation[2] = rotation[2];
        pose.orientation[3] = rotation[3];
        return pose;
    }
}  // namespace

namespace mesh_builder {

    bool GridIndex::operator==(const GridIndex &other) const {
        return indices[0] == other.indices[0] && indices[1] == other.indices[1] &&
               indices[2] == other.indices[2];
    }

    void MeshBuilderApp::onPointCloudAvailable(TangoPointCloud *point_cloud) {
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
        if (!hasNewFrame) {
            binder_mutex_.unlock();
            return;
        }
        glm::mat4 point_cloud_matrix_;
        point_cloud_matrix_ = glm::make_mat4(matrix_transform.matrix);
        point_cloud_matrix_[3][0] *= scale;
        point_cloud_matrix_[3][1] *= scale;
        point_cloud_matrix_[3][2] *= scale;
        if (fabs(1 - scale) > 0.005f) {
            for (unsigned int i = 0; i < point_cloud->num_points; i++) {
                point_cloud->points[i][0] *= scale;
                point_cloud->points[i][1] *= scale;
                point_cloud->points[i][2] *= scale;
            }
        }
        Tango3DR_Pose t3dr_depth_pose;
        Tango3DR_PointCloud t3dr_depth;
        t3dr_depth.timestamp = point_cloud->timestamp;
        t3dr_depth.num_points = point_cloud->num_points;
        t3dr_depth.points = point_cloud->points;
        t3dr_depth_pose = extract3DRPose(point_cloud_matrix_);
        if(textured) {
            SaveFrame();
            Tango3DR_clear(t3dr_context_);
        }
        Tango3DR_Pose t3dr_image_pose = extract3DRPose(image_matrix);
        Tango3DR_update(t3dr_context_, &t3dr_depth, &t3dr_depth_pose, &t3dr_image, &t3dr_image_pose,
                        &t3dr_updated);
        MeshUpdate();
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::onFrameAvailable(TangoCameraId id, const TangoImageBuffer *buffer) {
        if (id != TANGO_CAMERA_COLOR || !t3dr_is_running_)
            return;

        // Get the camera color transform to OpenGL world frame in OpenGL convention.
        TangoMatrixTransformData matrix_transform;
        TangoSupport_getMatrixTransformAtTime(
                        buffer->timestamp, TANGO_COORDINATE_FRAME_START_OF_SERVICE,
                        TANGO_COORDINATE_FRAME_CAMERA_COLOR, TANGO_SUPPORT_ENGINE_OPENGL,
                        TANGO_SUPPORT_ENGINE_TANGO, ROTATION_0, &matrix_transform);
        if (matrix_transform.status_code != TANGO_POSE_VALID)
            return;

        binder_mutex_.lock();
        if (hasNewFrame) {
            binder_mutex_.unlock();
            return;
        }

        image_matrix = glm::make_mat4(matrix_transform.matrix);
        image_matrix[3][0] *= scale;
        image_matrix[3][1] *= scale;
        image_matrix[3][2] *= scale;
        t3dr_image.width = buffer->width;
        t3dr_image.height = buffer->height;
        t3dr_image.stride = buffer->stride;
        t3dr_image.timestamp = buffer->timestamp;
        t3dr_image.format = static_cast<Tango3DR_ImageFormatType>(buffer->format);
        t3dr_image.data = buffer->data;
        hasNewFrame = true;
        binder_mutex_.unlock();
    }

    MeshBuilderApp::MeshBuilderApp() {
        t3dr_is_running_ = false;
        dataset_ = "";
        gyro = false;
        hasNewFrame = false;
        landscape = false;
        photoFinished = false;
        photoMode = false;
        textured = false;
        textureId = 0;
        scale = 1;
        zoom = 0;
    }

    MeshBuilderApp::~MeshBuilderApp() {
        if (tango_config_ != nullptr) {
            TangoConfig_free(tango_config_);
            tango_config_ = nullptr;
        }
    }

    void MeshBuilderApp::OnCreate(JNIEnv *env, jobject activity) {
        int version;
        TangoErrorType err = TangoSupport_GetTangoVersion(env, activity, &version);
        if (err != TANGO_SUCCESS || version < kTangoCoreMinimumVersion)
            std::exit(EXIT_SUCCESS);
    }

    void MeshBuilderApp::OnTangoServiceConnected(JNIEnv *env, jobject binder, double res,
                              double dmin, double dmax, int noise, bool land, bool photo,
                                                    bool textures, std::string dataset) {
        dataset_ = dataset;
        landscape = land;
        photoFinished = false;
        photoMode = photo;
        textured = textures;
        TangoService_setBinder(env, binder);
        TangoSetupConfig();
        TangoConnectCallbacks();
        TangoConnect();
        TangoSetup3DR(res, dmin, dmax, noise);
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

        /*TangoConfig_setBool(tango_config_, "config_color_mode_auto", false);
        TangoConfig_setInt32(tango_config_, "config_color_iso", 800);
        TangoConfig_setInt32(tango_config_, "config_color_exp", (int32_t) floor(11.1 * 2.0));*/
    }

    void MeshBuilderApp::TangoSetup3DR(double res, double dmin, double dmax, int noise) {
        binder_mutex_.lock();
        Tango3DR_ConfigH t3dr_config;
        t3dr_config = Tango3DR_Config_create(TANGO_3DR_CONFIG_CONTEXT);
        Tango3DR_Status t3dr_err;
        if (res < 0.00999)
            scale = 10;
        else
            scale = 1;
        t3dr_err = Tango3DR_Config_setDouble(t3dr_config, "resolution", res * scale);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setDouble(t3dr_config, "min_depth", dmin);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setDouble(t3dr_config, "max_depth", dmax * scale);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setBool(t3dr_config, "generate_color", true);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setBool(t3dr_config, "use_parallel_integration", true);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        Tango3DR_Config_setInt32(t3dr_config, "min_num_vertices", noise);
        Tango3DR_Config_setInt32(t3dr_config, "update_method", TANGO_3DR_PROJECTIVE_UPDATE);

        t3dr_context_ = Tango3DR_create(t3dr_config);
        if (t3dr_context_ == nullptr)
            std::exit(EXIT_SUCCESS);
        Tango3DR_Config_destroy(t3dr_config);

        Tango3DR_setColorCalibration(t3dr_context_, &t3dr_intrinsics_);
        Tango3DR_setDepthCalibration(t3dr_context_, &t3dr_intrinsics_depth);
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

        // Update the depth intrinsics too.
        err = TangoService_getCameraIntrinsics(TANGO_CAMERA_DEPTH, &intrinsics);
        if (err != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        t3dr_intrinsics_depth.calibration_type =
                static_cast<Tango3DR_TangoCalibrationType>(intrinsics.calibration_type);
        t3dr_intrinsics_depth.width = intrinsics.width;
        t3dr_intrinsics_depth.height = intrinsics.height;
        t3dr_intrinsics_depth.fx = intrinsics.fx;
        t3dr_intrinsics_depth.fy = intrinsics.fy;
        t3dr_intrinsics_depth.cx = intrinsics.cx;
        t3dr_intrinsics_depth.cy = intrinsics.cy;
        std::copy(std::begin(intrinsics.distortion), std::end(intrinsics.distortion),
                  std::begin(t3dr_intrinsics_depth.distortion));
    }

    void MeshBuilderApp::TangoDisconnect() {
        TangoConfig_free(tango_config_);
        tango_config_ = nullptr;
        TangoService_disconnect();
    }

    void MeshBuilderApp::OnPause() {
        TangoDisconnect();
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
            main_scene_.camera_->SetPosition(glm::vec3(movex, 0, movey));
            main_scene_.camera_->SetRotation(glm::quat(glm::vec3(yaw, pitch, 0)));
            main_scene_.camera_->SetScale(glm::vec3(1, 1, 1));
        } else {
            TangoMatrixTransformData matrix_transform;
            TangoSupport_getMatrixTransformAtTime(
                    0, TANGO_COORDINATE_FRAME_START_OF_SERVICE, TANGO_COORDINATE_FRAME_DEVICE,
                    TANGO_SUPPORT_ENGINE_OPENGL, TANGO_SUPPORT_ENGINE_OPENGL,
                    landscape ? ROTATION_90 : ROTATION_0, &matrix_transform);
            glm::mat4 start_service_T_device_;
            if (matrix_transform.status_code == TANGO_POSE_VALID)
                start_service_T_device_ = glm::make_mat4(matrix_transform.matrix);
            start_service_T_device_[3][0] *= scale;
            start_service_T_device_[3][1] *= scale;
            start_service_T_device_[3][2] *= scale;
            main_scene_.camera_->SetTransformationMatrix(start_service_T_device_);
            main_scene_.UpdateFrustum(main_scene_.camera_->GetPosition(), zoom);
        }
        //zoom
        glm::vec4 move = main_scene_.camera_->GetTransformationMatrix() * glm::vec4(0, 0, zoom, 0);
        main_scene_.camera_->Translate(glm::vec3(move.x, move.y, move.z));
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
        if (t3dr_is_running)
            hasNewFrame = false;
        t3dr_is_running_ = t3dr_is_running;
        photoFinished = false;
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::OnClearButtonClicked() {
        binder_mutex_.lock();
        Tango3DR_clear(t3dr_context_);
        meshes_.clear();
        main_scene_.ClearDynamicMeshes();
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::Load(std::string filename) {
        binder_mutex_.lock();
        render_mutex_.lock();

        ModelIO io(filename, false);
        main_scene_.toLoad = io.readModel(kSubdivisionSize, main_scene_.static_meshes_);

        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::Save(std::string filename)
    {
        binder_mutex_.lock();
        render_mutex_.lock();
        ModelIO io(filename, true);
        io.setDataset(dataset_);
        io.writeModel(main_scene_.dynamic_meshes_);
        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    float MeshBuilderApp::CenterOfStaticModel(bool horizontal) {
        float min = 99999999;
        float max = -99999999;
        for (tango_gl::StaticMesh mesh : main_scene_.static_meshes_) {
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

    void MeshBuilderApp::MeshUpdate() {
        if (photoMode)
            t3dr_is_running_ = false;

        std::vector<GridIndex> indices;
        indices.resize(t3dr_updated->num_indices);
        std::copy(&t3dr_updated->indices[0][0], &t3dr_updated->indices[t3dr_updated->num_indices][0],
                  reinterpret_cast<uint32_t *>(indices.data()));

        Tango3DR_Status ret;
        if (textured) {
            //extract textured mesh
            glm::mat4 world2uv = glm::inverse(image_matrix);
            tango_gl::StaticMesh debug;
            SingleDynamicMesh* dynamic_mesh = new SingleDynamicMesh();
            for (unsigned long it = 0; it < indices.size(); ++it) {
                GridIndex updated_index = indices[it];
                VertexProcessor vp(t3dr_context_, updated_index.indices);
                vp.getMeshWithUV(world2uv, t3dr_intrinsics_, dynamic_mesh);
                vp.getDebugMesh(&debug);
            }

            if (!dynamic_mesh->mesh.indices.empty()) {
                dynamic_mesh->size = (int) dynamic_mesh->mesh.indices.size();
                dynamic_mesh->mesh.texture = textureId;
                textureId++;
                render_mutex_.lock();
                main_scene_.AddDynamicMesh(dynamic_mesh);
                main_scene_.debug_meshes_.push_back(debug);
                render_mutex_.unlock();
            } else
                delete dynamic_mesh;
        } else {
            for (unsigned long it = 0; it < indices.size(); ++it) {
                GridIndex updated_index = indices[it];

                // Build a dynamic mesh and add it to the scene.
                SingleDynamicMesh* dynamic_mesh = meshes_[updated_index];
                if (dynamic_mesh == nullptr) {
                    dynamic_mesh = new SingleDynamicMesh();
                    dynamic_mesh->mesh.render_mode = GL_TRIANGLES;
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
                    ret = Tango3DR_extractPreallocatedMeshSegment(t3dr_context_, updated_index.indices,
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
        Tango3DR_GridIndexArray_destroy(t3dr_updated);

        if (photoMode)
            photoFinished = true;
        hasNewFrame = false;
    }

    void MeshBuilderApp::SaveFrame() {
        if (t3dr_image.format != TANGO_3DR_HAL_PIXEL_FORMAT_YCrCb_420_SP)
            std::exit(EXIT_SUCCESS);
        int scale = 4;
        unsigned char* rgb = new unsigned char[(t3dr_image.width / scale) * (t3dr_image.height / scale) * 3];
        std::ostringstream ostr;
        ostr << dataset_.c_str();
        ostr << "/frame_";
        ostr << textureId;
        ostr << ".png";
        int index = 0;
        int frameSize = t3dr_image.width * t3dr_image.height;
        for (int y = t3dr_image.height - 1; y >= 0; y-=scale) {
            for (int x = 0; x < t3dr_image.width; x+=scale) {
                int xby2 = x/2;
                int yby2 = y/2;
                int UVIndex = frameSize + 2*xby2 + yby2*t3dr_image.width;
                int Y = t3dr_image.data[y*t3dr_image.width + x] & 0xff;
                float U = (float)(t3dr_image.data[UVIndex + 0] & 0xff) - 128.0f;
                float V = (float)(t3dr_image.data[UVIndex + 1] & 0xff) - 128.0f;

                // Do the YUV -> RGB conversion
                float Yf = 1.164f*((float)Y) - 16.0f;
                int R = (int)(Yf + 1.596f*V);
                int G = (int)(Yf - 0.813f*V - 0.391f*U);
                int B = (int)(Yf            + 2.018f*U);

                // Clip rgb values to 0-255
                R = R < 0 ? 0 : R > 255 ? 255 : R;
                G = G < 0 ? 0 : G > 255 ? 255 : G;
                B = B < 0 ? 0 : B > 255 ? 255 : B;

                // Put that pixel in the buffer
                rgb[index++] = (unsigned char) B;
                rgb[index++] = (unsigned char) G;
                rgb[index++] = (unsigned char) R;
            }
        }
        TextureToLoad t;
        t.width = t3dr_image.width / scale;
        t.height = t3dr_image.height / scale;
        t.data = rgb;
        WritePNG(ostr.str().c_str(), (u_int32_t) t.width, (u_int32_t) t.height, t.data);
        render_mutex_.lock();
        main_scene_.toLoad.push_back(t);
        render_mutex_.unlock();
    }

    void MeshBuilderApp::WritePNG(const char* filename, int width, int height, unsigned char *buffer) {
        // Open file for writing (binary mode)
        FILE *fp = fopen(filename, "wb");

        // init PNG library
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info_ptr = png_create_info_struct(png_ptr);
        setjmp(png_jmpbuf(png_ptr));
        png_init_io(png_ptr, fp);
        png_set_IHDR(png_ptr, info_ptr, width, height,
                     8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(png_ptr, info_ptr);

        // write image data
        png_bytep row = (png_bytep) malloc(3 * width * sizeof(png_byte));
        for (int y = 0; y < height; y++) {
            for (int x=0; x < width; x++) {
                row[x * 3 + 0] = buffer[(y * width + x) * 3 + 0];
                row[x * 3 + 1] = buffer[(y * width + x) * 3 + 1];
                row[x * 3 + 2] = buffer[(y * width + x) * 3 + 2];
            }
            png_write_row(png_ptr, row);
        }
        png_write_end(png_ptr, NULL);

        /// close all
        if (fp != NULL) fclose(fp);
        if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
        if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        if (row != NULL) free(row);
    }
}  // namespace mesh_builder
