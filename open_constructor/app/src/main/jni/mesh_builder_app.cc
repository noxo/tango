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
#include <unistd.h>

#include "mesh_builder/mesh_builder_app.h"

mesh_builder::MeshBuilderApp* app = 0;

namespace {
    const int kSubdivisionSize = 5000;
    const int kInitialVertexCount = 100;
    const int kInitialIndexCount = 99;
    const int kGrowthFactor = 5;
    constexpr int kTangoCoreMinimumVersion = 9377;

    void onPointCloudAvailableRouter(void *context, const TangoPointCloud *point_cloud) {
        mesh_builder::MeshBuilderApp *app = static_cast<mesh_builder::MeshBuilderApp *>(context);
        app->onPointCloudAvailable((TangoPointCloud*)point_cloud);
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

    glm::quat toQuaternion(double pitch, double roll, double yaw) {
        glm::quat q;
        double t0 = std::cos(yaw * 0.5f);
        double t1 = std::sin(yaw * 0.5f);
        double t2 = std::cos(roll * 0.5f);
        double t3 = std::sin(roll * 0.5f);
        double t4 = std::cos(pitch * 0.5f);
        double t5 = std::sin(pitch * 0.5f);

        q.w = t0 * t2 * t4 + t1 * t3 * t5;
        q.x = t0 * t3 * t4 - t1 * t2 * t5;
        q.y = t0 * t2 * t5 + t1 * t3 * t4;
        q.z = t1 * t2 * t4 - t0 * t3 * t5;
        return q;
    }

    unsigned int scanDec(char *line, int offset)
    {
        unsigned int number = 0;
        for (int i = offset; i < 1024; i++) {
            char c = line[i];
            if (c != '\n')
                number = number * 10 + c - '0';
            else
                return number;
        }
        return number;
    }

    bool startsWith(std::string s, std::string e)
    {
        if (s.size() >= e.size())
            if (s.substr(0, e.size()).compare(e) == 0)
                return true;
        return false;
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
        if (t3dr_image.timestamp == 0) {
            binder_mutex_.unlock();
            return;
        }
        glm::mat4 point_cloud_matrix_ = glm::make_mat4(matrix_transform.matrix);
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

        Tango3DR_PointCloud t3dr_depth;
        t3dr_depth.num_points = point_cloud->num_points;
        t3dr_depth.points = point_cloud->points;
        //For Tango3DR_update process there must be swapped timestamps(but the same gap!)
        t3dr_depth.timestamp = t3dr_image.timestamp;
        t3dr_image.timestamp = point_cloud->timestamp;

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
        if (photoMode)
            t3dr_is_running_ = false;

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
        if (photoMode)
            photoFinished = true;
        process_mutex_.unlock();
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::onFrameAvailable(TangoCameraId id, const TangoImageBuffer *buffer) {
        if (id != TANGO_CAMERA_COLOR)
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
        glm::mat4 image_matrix = glm::make_mat4(matrix_transform.matrix);
        image_matrix[3][0] *= scale;
        image_matrix[3][1] *= scale;
        image_matrix[3][2] *= scale;
        t3dr_image.width = buffer->width;
        t3dr_image.height = buffer->height;
        t3dr_image.stride = buffer->stride;
        t3dr_image.timestamp = buffer->timestamp;
        t3dr_image.format = static_cast<Tango3DR_ImageFormatType>(buffer->format);
        t3dr_image.data = buffer->data;
        extract3DRPose(image_matrix, &t3dr_image_pose);
        binder_mutex_.unlock();
    }

    MeshBuilderApp::MeshBuilderApp() {
        t3dr_image.timestamp = 0;
        gyro = true;
        landscape = false;
        photoFinished = false;
        photoMode = false;
        scale = 1;
        zoom = 0;
    }

    MeshBuilderApp::~MeshBuilderApp() {
        if (tango_config_ != nullptr) {
            TangoConfig_free(tango_config_);
            tango_config_ = nullptr;
        }
    }

    void MeshBuilderApp::ActivityCtor(bool t3dr_is_running) {
        t3dr_is_running_ = t3dr_is_running;
    }

    void MeshBuilderApp::SetPhotoMode(bool on) {
        binder_mutex_.lock();
        photoFinished = false;
        photoMode = on;
        binder_mutex_.unlock();
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
    }

    void MeshBuilderApp::TangoSetup3DR(double res, double dmin, double dmax) {
        binder_mutex_.lock();
        if(t3dr_context_ != nullptr)
            Tango3DR_destroy(t3dr_context_);
        Tango3DR_ConfigH t3dr_config = Tango3DR_Config_create(TANGO_3DR_CONFIG_CONTEXT);
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

        Tango3DR_Config_setInt32(t3dr_config, "update_method", TANGO_3DR_PROJECTIVE_UPDATE);

        TangoConfig_setBool(t3dr_config, "config_color_mode_auto", false);
        TangoConfig_setInt32(t3dr_config, "config_color_iso", 800);
        TangoConfig_setInt32(t3dr_config, "config_color_exp", (int32_t) floor(11.1 * 2.0));

        t3dr_context_ = Tango3DR_create(t3dr_config);
        if (t3dr_context_ == nullptr)
            std::exit(EXIT_SUCCESS);

        Tango3DR_setColorCalibration(t3dr_context_, &t3dr_intrinsics_);
        Tango3DR_setDepthCalibration(t3dr_context_, &t3dr_intrinsics_depth);
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

        TangoMatrixTransformData matrix_transform;
        TangoSupport_getMatrixTransformAtTime(
                0, TANGO_COORDINATE_FRAME_START_OF_SERVICE, TANGO_COORDINATE_FRAME_DEVICE,
                TANGO_SUPPORT_ENGINE_OPENGL, TANGO_SUPPORT_ENGINE_OPENGL, ROTATION_0,
                &matrix_transform);
        if (matrix_transform.status_code == TANGO_POSE_VALID)
            start_service_T_device_ = glm::make_mat4(matrix_transform.matrix);
        start_service_T_device_[3][0] *= scale;
        start_service_T_device_[3][1] *= scale;
        start_service_T_device_[3][2] *= scale;

        render_mutex_.lock();
        //camera transformation
        if (!gyro) {
            main_scene_.camera_->SetPosition(glm::vec3(movex, 0, movey));
            main_scene_.camera_->SetRotation(toQuaternion(pitch, yaw, 0));
            main_scene_.camera_->SetScale(glm::vec3(1, 1, 1));
        } else {
            if (landscape) {
                float radian = (float) (-90 * M_PI / 180);
                glm::mat4x4 rotation(
                        cosf(radian),sinf(radian),0,0,
                        -sinf(radian),cosf(radian),0,0,
                        0,0,1,0,
                        0,0,0,1
                );
                start_service_T_device_ *= rotation;
            }
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
                dynamic_mesh->mesh.indices.resize(kInitialIndexCount);
                app->render_mutex_.lock();
                app->main_scene_.AddDynamicMesh(dynamic_mesh.get());
                app->render_mutex_.unlock();
            }
            dynamic_mesh->mutex.lock();
            app->add_mutex_.unlock();

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
                    /* texture_coords */ nullptr,
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
                dynamic_mesh->mesh.indices.resize(new_index_size);
            } else {
                ++it;
                dynamic_mesh->size = tango_mesh.num_faces * 3;
            }
            dynamic_mesh->mutex.unlock();
        }
        app->threadMutex[thread].unlock();
        app->threadDone[thread] = true;
        pthread_detach(app->threadId[thread]);
        return 0;
    }

    void MeshBuilderApp::Load(std::string filename) {
        binder_mutex_.lock();
        process_mutex_.lock();
        render_mutex_.lock();
        LOGI("Loading from %s", filename.c_str());
        FILE *file = fopen(filename.c_str(), "r");

        //prepare objects
        char buffer[1024];
        unsigned int vertexCount = 0;
        unsigned int faceCount = 0;
        std::vector<std::pair<glm::vec3, unsigned int> > vertices;

        //read header
        while (true) {
            if (!fgets(buffer, 1024, file))
                break;
            if (startsWith(buffer, "element vertex"))
                vertexCount = scanDec(buffer, 15);
            else if (startsWith(buffer, "element face"))
                faceCount = scanDec(buffer, 13);
            else if (startsWith(buffer, "end_header"))
                break;
        }
        //read vertices
        unsigned int t, a, b, c;
        std::pair<glm::vec3, unsigned int> vec;
        for (int i = 0; i < vertexCount; i++) {
            fscanf(file, "%f %f %f %d %d %d", &vec.first.x, &vec.first.z, &vec.first.y, &a, &b, &c);
            vec.first.x *= -1.0f;
            vec.second = a + (b << 8) + (c << 16);
            vertices.push_back(vec);
        }
        //parse faces
        int parts = faceCount / kSubdivisionSize;
        if(faceCount % kSubdivisionSize > 0)
            parts++;
        for (int j = 0; j < parts; j++)  {
            tango_gl::StaticMesh static_mesh;
            static_mesh.render_mode = GL_TRIANGLES;
            int count = kSubdivisionSize;
            if (j == parts - 1)
                count = faceCount % kSubdivisionSize;
            for (int i = 0; i < count; i++)  {
                fscanf(file, "%d %d %d %d", &t, &a, &b, &c);
                //unsupported format
                if (t != 3)
                    continue;
                //broken topology ignored
                if ((a == b) || (a == c) || (b == c))
                    continue;
                static_mesh.vertices.push_back(vertices[a].first);
                static_mesh.colors.push_back(vertices[a].second);
                static_mesh.vertices.push_back(vertices[b].first);
                static_mesh.colors.push_back(vertices[b].second);
                static_mesh.vertices.push_back(vertices[c].first);
                static_mesh.colors.push_back(vertices[c].second);
            }
            main_scene_.static_meshes_.push_back(static_mesh);
        }
        fclose(file);
        render_mutex_.unlock();
        process_mutex_.unlock();
        binder_mutex_.unlock();
    }

    void MeshBuilderApp::Save(std::string filename)
    {
        binder_mutex_.lock();
        process_mutex_.lock();
        LOGI("Writing into %s", filename.c_str());

        //count vertices and faces
        unsigned int vertexCount = 0;
        unsigned int faceCount = 0;
        for(unsigned int i = 0; i < main_scene_.dynamic_meshes_.size(); i++) {
            vertexCount += main_scene_.dynamic_meshes_[i]->mesh.vertices.size();
            faceCount += main_scene_.dynamic_meshes_[i]->size / 3;
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
            for (unsigned int j = 0; j < main_scene_.dynamic_meshes_[i]->size; j+=3) {
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

    void MeshBuilderApp::Filter(std::string oldname, std::string newname, int passes) {
        LOGI("Loading from %s", oldname.c_str());
        FILE *file = fopen(oldname.c_str(), "r");

        //prepare objects
        char buffer[1024];
        unsigned int vertexCount = 0;
        unsigned int faceCount = 0;
        std::vector<glm::ivec3> indices;
        std::pair<glm::vec3, glm::ivec3> vec;

        //read header
        while (true) {
            if (!fgets(buffer, 1024, file))
                break;
            if (startsWith(buffer, "element vertex"))
                vertexCount = scanDec(buffer, 15);
            else if (startsWith(buffer, "element face"))
                faceCount = scanDec(buffer, 13);
            else if (startsWith(buffer, "end_header"))
                break;
        }

        {
            std::map<unsigned int, unsigned int> index2index;
            std::string key;
            {
                //prepare reindexing of vertices
                std::map<std::string, unsigned int> key2index;
                for (unsigned int i = 0; i < vertexCount; i++) {
                    if (!fgets(buffer, 1024, file))
                        break;
                    sscanf(buffer, "%f %f %f %d %d %d", &vec.first.x, &vec.first.y, &vec.first.z,
                           &vec.second.r, &vec.second.g, &vec.second.b);
                    sprintf(buffer, "%.3f,%.3f,%.3f", vec.first.x, vec.first.y, vec.first.z);
                    key = std::string(buffer);
                    if (key2index.find(key) == key2index.end())
                        key2index[key] = i;
                    else
                        index2index[i] = key2index[key];
                }
            }

            //parse indices
            unsigned int t, a, b, c;
            std::vector<int> nodeLevel;
            for (unsigned int i = 0; i < vertexCount; i++)
              nodeLevel.push_back(0);
            for (unsigned int i = 0; i < faceCount; i++)  {
                fscanf(file, "%d %d %d %d", &t, &a, &b, &c);
                //unsupported format
                if (t != 3)
                    continue;
                //broken topology ignored
                if ((a == b) || (a == c) || (b == c))
                    continue;
                //reindex
                if (index2index.find(a) != index2index.end())
                    a = index2index[a];
                if (index2index.find(b) != index2index.end())
                    b = index2index[b];
                if (index2index.find(c) != index2index.end())
                    c = index2index[c];
                //get node levels
                nodeLevel[a]++;
                nodeLevel[b]++;
                nodeLevel[c]++;
                //store indices
                indices.push_back(glm::ivec3(a, b, c));
            }

            //filter indices
            glm::ivec3 ci;
            std::vector<glm::ivec3> decrease;
            for (int pass = 0; pass < passes; pass++) {
                LOGI("Processing noise filter pass %d/%d", pass + 1, passes);
                for (long i = indices.size() - 1; i >= 0; i--) {
                    ci = indices[i];
                    if ((nodeLevel[ci.x] < 3) || (nodeLevel[ci.y] < 3) || (nodeLevel[ci.z] < 3)) {
                        indices.erase(indices.begin() + i);
                        decrease.push_back(ci);
                    }
                }
                for (glm::ivec3 i : decrease) {
                    nodeLevel[i.x]--;
                    nodeLevel[i.y]--;
                    nodeLevel[i.z]--;
                }
                decrease.clear();
            }
            faceCount = indices.size();
            fclose(file);
        }

        //reload vertices into memory
        LOGI("Reloading vertices from %s", oldname.c_str());
        file = fopen(oldname.c_str(), "r");
        while (true) {
            if (!fgets(buffer, 1024, file))
                break;
            else if (startsWith(buffer, "end_header"))
                break;
        }
        std::vector<std::pair<glm::vec3, glm::ivec3> > vertices;
        for (unsigned int i = 0; i < vertexCount; i++) {
            if (!fgets(buffer, 1024, file))
                break;
            sscanf(buffer, "%f %f %f %d %d %d", &vec.first.x, &vec.first.y, &vec.first.z,
                   &vec.second.r, &vec.second.g, &vec.second.b);
            vertices.push_back(vec);
        }
        fclose(file);

        //file header
        LOGI("Writing into %s", newname.c_str());
        file = fopen(newname.c_str(), "w");
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
        glm::vec3 vi;
        glm::ivec3 ci;
        for (unsigned int i = 0; i < vertices.size(); i++) {
            vi = vertices[i].first;
            ci = vertices[i].second;
            fprintf(file, "%f %f %f %d %d %d\n", vi.x, vi.y, vi.z, ci.r, ci.g, ci.b);
        }

        //write faces
        for (glm::ivec3 i : indices)
            fprintf(file, "3 %d %d %d\n", i.x, i.y, i.z);
        fclose(file);
    }
}  // namespace mesh_builder
