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

#include "mask_processor.h"
#include "math_utils.h"
#include "mesh_builder_app.h"

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
                point_cloud->timestamp, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION,
                TANGO_COORDINATE_FRAME_CAMERA_DEPTH, TANGO_SUPPORT_ENGINE_OPENGL,
                TANGO_SUPPORT_ENGINE_TANGO, ROTATION_0, &matrix_transform);
        if (matrix_transform.status_code != TANGO_POSE_VALID)
            return;

        binder_mutex_.lock();
        point_cloud_matrix_ = glm::make_mat4(matrix_transform.matrix);
        point_cloud_matrix_[3][0] *= scale;
        point_cloud_matrix_[3][1] *= scale;
        point_cloud_matrix_[3][2] *= scale;
        if (glm::abs(1 - scale) > 0.005f) {
            for (unsigned int i = 0; i < point_cloud->num_points; i++) {
                point_cloud->points[i][0] *= scale;
                point_cloud->points[i][1] *= scale;
                point_cloud->points[i][2] *= scale;
            }
        }
        TangoSupport_updatePointCloud(point_cloud_manager_, point_cloud);
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
        image_matrix[3][0] *= scale;
        image_matrix[3][1] *= scale;
        image_matrix[3][2] *= scale;

        Tango3DR_ImageBuffer t3dr_image;
        t3dr_image.width = buffer->width;
        t3dr_image.height = buffer->height;
        t3dr_image.stride = buffer->stride;
        t3dr_image.timestamp = buffer->timestamp;
        t3dr_image.format = static_cast<Tango3DR_ImageFormatType>(buffer->format);
        t3dr_image.data = buffer->data;
        Tango3DR_Pose t3dr_image_pose = Math::extract3DRPose(image_matrix);
        if(!photoMode) {
            glm::quat rot = glm::quat((float) t3dr_image_pose.orientation[0],
                                      (float) t3dr_image_pose.orientation[1],
                                      (float) t3dr_image_pose.orientation[2],
                                      (float) t3dr_image_pose.orientation[3]);
            float diff = Math::diff(rot, image_rotation);
            image_rotation = rot;
            int limit = textured ? 1 : 5;
            if (diff > limit) {
                binder_mutex_.unlock();
                return;
            }
        }

        Tango3DR_PointCloud t3dr_depth;
        TangoSupport_getLatestPointCloud(point_cloud_manager_, &front_cloud_);
        t3dr_depth.timestamp = front_cloud_->timestamp;
        t3dr_depth.num_points = front_cloud_->num_points;
        t3dr_depth.points = front_cloud_->points;

        Tango3DR_Pose t3dr_depth_pose = Math::extract3DRPose(point_cloud_matrix_);
        Tango3DR_GridIndexArray* t3dr_updated;
        Tango3DR_Status t3dr_err =
                Tango3DR_update(t3dr_context_, &t3dr_depth, &t3dr_depth_pose, &t3dr_image,
                                &t3dr_image_pose, &t3dr_updated);
        if (t3dr_err != TANGO_3DR_SUCCESS)
        {
            binder_mutex_.unlock();
            return;
        }
        if (textured) {
            RGBImage frame(t3dr_image, 4);
            std::ostringstream ss;
            ss << dataset_.c_str();
            ss << "/";
            ss << poses_.size();
            ss << ".png";
            frame.Write(ss.str().c_str());
            poses_.push_back(image_matrix);
            timestamps_.push_back(t3dr_image.timestamp);
        }

        MeshUpdate(t3dr_image, t3dr_updated);
        Tango3DR_GridIndexArray_destroy(t3dr_updated);
        point_cloud_available_ = false;
        binder_mutex_.unlock();
    }


    MeshBuilderApp::MeshBuilderApp() :  t3dr_is_running_(false),
                                        gyro(false),
                                        landscape(false),
                                        photoFinished(false),
                                        photoMode(false),
                                        point_cloud_available_(false),
                                        textured(false),
                                        scale(1),
                                        zoom(0)
    {
        textureProcessor = new TextureProcessor();
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

    void MeshBuilderApp::OnCreate(JNIEnv *env, jobject activity) {
        int version;
        TangoErrorType err = TangoSupport_GetTangoVersion(env, activity, &version);
        if (err != TANGO_SUCCESS || version < kTangoCoreMinimumVersion)
            std::exit(EXIT_SUCCESS);
    }

    void MeshBuilderApp::OnTangoServiceConnected(JNIEnv *env, jobject binder, double res,
               double dmin, double dmax, int noise, bool land, bool photo, bool textures,
               std::string dataset) {
        dataset_ = dataset;
        landscape = land;
        photoFinished = false;
        photoMode = photo;
        textured = textures;
        if (res < 0.00999)
            scale = 10;
        else
            scale = 1;

        TangoService_setBinder(env, binder);
        TangoSetupConfig();
        TangoConnectCallbacks();
        TangoConnect();
        binder_mutex_.lock();
        t3dr_context_ = TangoSetup3DR(res, dmin, dmax, noise);
        binder_mutex_.unlock();
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

        // Disable learning.
        ret = TangoConfig_setBool(tango_config_, "config_enable_learning_mode", false);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Enable drift correction.
        ret = TangoConfig_setBool(tango_config_, "config_enable_drift_correction", true);
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

        // Set datasets
        ret = TangoConfig_setString(tango_config_, "config_datasets_path", dataset_.c_str());
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = TangoConfig_setBool(tango_config_, "config_enable_dataset_recording", true);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = TangoConfig_setInt32(tango_config_, "config_dataset_recording_mode", TANGO_RECORDING_MODE_MOTION_TRACKING);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        if (point_cloud_manager_ == nullptr) {
            int32_t max_point_cloud_elements;
            ret = TangoConfig_getInt32(tango_config_, "max_point_cloud_elements",
                                       &max_point_cloud_elements);
            if (ret != TANGO_SUCCESS) {
                LOGE("Failed to query maximum number of point cloud elements.");
                std::exit(EXIT_SUCCESS);
            }

            ret = TangoSupport_createPointCloudManager((size_t) max_point_cloud_elements,
                                                       &point_cloud_manager_);
            if (ret != TANGO_SUCCESS)
                std::exit(EXIT_SUCCESS);
        }
    }

    Tango3DR_Context MeshBuilderApp::TangoSetup3DR(double res, double dmin, double dmax, int noise) {
        Tango3DR_ConfigH t3dr_config = Tango3DR_Config_create(TANGO_3DR_CONFIG_CONTEXT);
        Tango3DR_Status t3dr_err;
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

        Tango3DR_Context output = Tango3DR_create(t3dr_config);
        if (output == nullptr)
            std::exit(EXIT_SUCCESS);
        Tango3DR_Config_destroy(t3dr_config);

        Tango3DR_setColorCalibration(output, &t3dr_intrinsics_);
        Tango3DR_setDepthCalibration(output, &t3dr_intrinsics_depth);
        return output;
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
                    0, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION, TANGO_COORDINATE_FRAME_DEVICE,
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
        if (textureProcessor->UpdateGL())
          main_scene_.textureMap = textureProcessor->TextureMap();
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
        photoFinished = false;
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::OnClearButtonClicked() {
        binder_mutex_.lock();
        render_mutex_.lock();
        Tango3DR_clear(t3dr_context_);
        meshes_.clear();
        poses_.clear();
        timestamps_.clear();
        main_scene_.ClearDynamicMeshes();
        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::Load(std::string filename) {
        binder_mutex_.lock();
        render_mutex_.lock();
        ModelIO io(filename, false);
        textureProcessor->Add(io.ReadModel(kSubdivisionSize, main_scene_.static_meshes_));
        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::Save(std::string filename, std::string dataset) {
        binder_mutex_.lock();
        render_mutex_.lock();
        if (textured) {
            //get mesh to texture
            Tango3DR_Mesh* mesh = 0;
            Tango3DR_Status ret;
            ret = Tango3DR_extractFullMesh(t3dr_context_, &mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);

            //get texturing context
            Tango3DR_ConfigH textureConfig;
            textureConfig = Tango3DR_Config_create(TANGO_3DR_CONFIG_TEXTURING);
            ret = Tango3DR_Config_setDouble(textureConfig, "min_resolution", 0.01);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            Tango3DR_Context context;
            context = Tango3DR_createTexturingContext(textureConfig, dataset.c_str(), mesh);
            if (context == nullptr)
                std::exit(EXIT_SUCCESS);
            Tango3DR_Config_destroy(textureConfig);

            //texturize mesh
            ret = Tango3DR_Mesh_destroy(mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            ret = Tango3DR_getTexturedMesh(context, &mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);

            //save and cleanup
            ret = Tango3DR_destroyTexturingContext(context);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            ret = Tango3DR_Mesh_saveToObj(mesh, filename.c_str());
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            ret = Tango3DR_Mesh_destroy(mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
        } else {
            ModelIO io(filename, true);
            io.WriteModel(main_scene_.dynamic_meshes_);
        }
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

    void MeshBuilderApp::MeshUpdate(Tango3DR_ImageBuffer t3dr_image, Tango3DR_GridIndexArray *t3dr_updated) {
        if (photoMode)
            t3dr_is_running_ = false;

        for (unsigned long it = 0; it < t3dr_updated->num_indices; ++it) {
            GridIndex updated_index;
            updated_index.indices[0] = t3dr_updated->indices[it][0];
            updated_index.indices[1] = t3dr_updated->indices[it][1];
            updated_index.indices[2] = t3dr_updated->indices[it][2];

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
                Tango3DR_Status ret;
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
        if (photoMode)
            photoFinished = true;
    }
}  // namespace mesh_builder
