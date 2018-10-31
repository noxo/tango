#include <sstream>
#include "app.h"

//#define EXPORT_DEPTHMAP

oc::App *appInstance = 0;

namespace {
    const int kSubdivisionSize = 20000;

    glm::mat4 convertToMat(double* translation, double* orientation) {
        glm::quat rotation;
        rotation.x = orientation[0];
        rotation.y = orientation[1];
        rotation.z = orientation[2];
        rotation.w = orientation[3];
        glm::mat4 output = glm::mat4_cast(rotation);
        output[3][0] = translation[0];
        output[3][1] = translation[1];
        output[3][2] = translation[2];
        return output;
    }

    void* process(void*) {

        //check if pose is vlaid
        std::vector<TangoMatrixTransformData> transform = appInstance->tango.Pose(appInstance->t3dr_image.timestamp, 0);
        if (transform[COLOR_CAMERA].status_code != TANGO_POSE_VALID) {
            pthread_detach(appInstance->threadId);
            appInstance->binder_mutex_.unlock();
            return 0;
        }

        //check if point cloud is valid and capturing is enabled
        if (!appInstance->point_cloud_available_ || !appInstance->t3dr_is_running_) {
            pthread_detach(appInstance->threadId);
            appInstance->binder_mutex_.unlock();
            return 0;
        }

        //sharp photos filtering
        Tango3DR_Pose t3dr_image_pose = oc::TangoService::Extract3DRPose(appInstance->tango.Convert(transform)[COLOR_CAMERA]);
        if (appInstance->sharp) {
            glm::vec3 pos = glm::vec3((float) t3dr_image_pose.translation[0],
                                      (float) t3dr_image_pose.translation[1],
                                      (float) t3dr_image_pose.translation[2]);
            glm::quat rot = glm::quat((float) t3dr_image_pose.orientation[0],
                                      (float) t3dr_image_pose.orientation[1],
                                      (float) t3dr_image_pose.orientation[2],
                                      (float) t3dr_image_pose.orientation[3]);
            float value = oc::GLCamera::Diff(pos, appInstance->image_position, rot, appInstance->image_rotation);
            float diff = value > appInstance->last_diff ? value : 0.95f * appInstance->last_diff + 0.05f * value;
            appInstance->last_diff = diff;
            appInstance->image_position = pos;
            appInstance->image_rotation = rot;
            if (diff > 1) {
                pthread_detach(appInstance->threadId);
                appInstance->binder_mutex_.unlock();
                return 0;
            }
        }

        //get point cloud
        Tango3DR_PointCloud t3dr_depth;
        Tango3DR_Pose t3dr_depth_pose = oc::TangoService::Extract3DRPose(appInstance->point_cloud_matrix_);
        TangoSupport_getLatestPointCloud(appInstance->tango.Pointcloud(), &appInstance->front_cloud_);
        t3dr_depth.timestamp = appInstance->front_cloud_->timestamp;
        t3dr_depth.num_points = appInstance->front_cloud_->num_points;
        t3dr_depth.points = appInstance->front_cloud_->points;

        if (appInstance->photomode) {
            appInstance->StoreDataset(t3dr_depth, t3dr_depth_pose, t3dr_image_pose);

            //get depth sensor pose
            int count, w, h;
            float cx, cy, fx, fy;
            appInstance->tango.Dataset().GetCalibration(cx, cy, fx, fy);
            appInstance->tango.Dataset().GetState(count, w, h);
            int poseIndex = count - 1;
            double translation[3];
            double orientation[4];
            appInstance->tango.Dataset().GetPose(poseIndex, DEPTH_CAMERA, translation, orientation);
            glm::mat4 sensor2world = convertToMat(translation, orientation);

            //get camera pose
            appInstance->tango.Dataset().GetPose(poseIndex, COLOR_CAMERA, translation, orientation);
            glm::mat4 world2uv = glm::inverse(convertToMat(translation, orientation));

            //get raw pointcloud
            std::vector<oc::Mesh> mesh;
            oc::Image jpg(appInstance->tango.Dataset().GetFileName(poseIndex, ".jpg"));
            oc::File3d pcl(appInstance->tango.Dataset().GetFileName(poseIndex, ".pcl"), false);
            pcl.ReadModel(-1, mesh);

            //process pointcloud
            int mapScale = 12;
            int stride = jpg.GetWidth() / mapScale;
            int height = jpg.GetHeight() / mapScale;
            int map[stride * height];
            for (int i = 0; i < stride * height; i++)
                map[i] = -1;
            std::vector<glm::vec3> points;
            std::vector<float> depth;
            std::vector<unsigned int> colors;
            for (unsigned int i = 0; i < mesh[0].vertices.size(); i++) {
                glm::vec3 v = mesh[0].vertices[i];
                if (fabs(v.z) > 2)
                    continue;
                glm::vec4 w = sensor2world * glm::vec4(v, 1.0f);
                w /= glm::abs(w.w);
                glm::vec4 t = world2uv * glm::vec4(w.x, w.y, w.z, 1.0f);
                t.x /= glm::abs(t.z * t.w);
                t.y /= glm::abs(t.z * t.w);
                t.x *= fx / (float)jpg.GetWidth();
                t.y *= fy / (float)jpg.GetHeight();
                t.x += cx / (float)jpg.GetWidth();
                t.y += cy / (float)jpg.GetHeight();
                int x = (int)(t.x * jpg.GetWidth());
                int y = (int)(t.y * jpg.GetHeight());
                int index = (y * jpg.GetWidth() + x) * 4;
                if ((x >= 0) && (x < jpg.GetWidth()) && (y >= 0) && (y < jpg.GetHeight())) {
                    map[((int)(y / mapScale)) * stride + (int)(x / mapScale)] = points.size();
                    glm::ivec3 color;
                    color.r = jpg.GetData()[index + 0];
                    color.g = jpg.GetData()[index + 1];
                    color.b = jpg.GetData()[index + 2];
                    colors.push_back(oc::File3d::CodeColor(color));
                    depth.push_back(v.z);
                    points.push_back(glm::vec3(w.x, w.y, w.z));
                }
            }

            //generate geometry
            float aspect = 0.075f;
            int margin = 4;
            oc::Mesh m;
            for (int x = 1 + margin; x < stride - margin; x++) {
                for (int y = 1 + margin; y < height - margin; y++) {
                    int a = map[(y - 1) * stride + x - 1];
                    int b = map[(y - 1) * stride + x];
                    int c = map[y * stride + x - 1];
                    int d = map[y * stride + x];
                    if ((a >= 0) && (b >= 0) && (c >= 0) && (a != b) && (b != c) && (c != a)) {
                        float avrg = aspect * (depth[c] + depth[b] + depth[a]) / 3.0f;
                        float len1 = fabs(depth[a] - depth[b]);
                        float len2 = fabs(depth[a] - depth[c]);
                        float len3 = fabs(depth[b] - depth[c]);
                        if ((len1 < avrg) && (len2 < avrg) && (len3 < avrg)) {
                            m.vertices.push_back(points[c]);
                            m.vertices.push_back(points[b]);
                            m.vertices.push_back(points[a]);
                            m.colors.push_back(colors[c]);
                            m.colors.push_back(colors[b]);
                            m.colors.push_back(colors[a]);
                        }
                    }
                    if ((d >= 0) && (b >= 0) && (c >= 0) && (d != b) && (b != c) && (c != d)) {
                        float avrg = aspect * (depth[b] + depth[c] + depth[d]) / 3.0f;
                        float len1 = fabs(depth[d] - depth[b]);
                        float len2 = fabs(depth[d] - depth[c]);
                        float len3 = fabs(depth[b] - depth[c]);
                        if ((len1 < avrg) && (len2 < avrg) && (len3 < avrg)) {
                            m.vertices.push_back(points[b]);
                            m.vertices.push_back(points[c]);
                            m.vertices.push_back(points[d]);
                            m.colors.push_back(colors[b]);
                            m.colors.push_back(colors[c]);
                            m.colors.push_back(colors[d]);
                        }
                    }
                }
            }

            //merge
            appInstance->render_mutex_.lock();
            appInstance->scene.static_meshes_.push_back(m);
            appInstance->render_mutex_.unlock();
        } else {

            //update 3DR
            Tango3DR_GridIndexArray t3dr_updated;
            Tango3DR_Status ret;
            ret = Tango3DR_updateFromPointCloud(appInstance->tango.Context(), &t3dr_depth, &t3dr_depth_pose,
                                                &appInstance->t3dr_image, &t3dr_image_pose, &t3dr_updated);
            if (ret != TANGO_3DR_SUCCESS) {
                pthread_detach(appInstance->threadId);
                appInstance->binder_mutex_.unlock();
                return 0;
            }

            //update dataset and update GL scene
            appInstance->StoreDataset(t3dr_depth, t3dr_depth_pose, t3dr_image_pose);
            std::vector<std::pair<oc::GridIndex, Tango3DR_Mesh*> > added;
            added = appInstance->scan.Process(appInstance->tango.Context(), &t3dr_updated);
            Tango3DR_GridIndexArray_destroy(&t3dr_updated);
            appInstance->render_mutex_.lock();
            appInstance->scan.Merge(added);
            appInstance->render_mutex_.unlock();
        }

        //cleanup and finish
        appInstance->point_cloud_available_ = false;
        if (appInstance->photomode)
            appInstance->t3dr_is_running_ = false;
        pthread_detach(appInstance->threadId);
        appInstance->binder_mutex_.unlock();
        return 0;
    }

    void onFrameAvailableRouter(void *context, TangoCameraId id, const TangoImageBuffer *im) {
        if (id != TANGO_CAMERA_COLOR)
            return;

        appInstance = static_cast<oc::App *>(context);
        if (!appInstance->running)
            return;

        if (appInstance->binder_mutex_.try_lock()) {
            //copy metadata
            appInstance->t3dr_image.width = im->width;
            appInstance->t3dr_image.height = im->height;
            appInstance->t3dr_image.stride = im->stride;
            appInstance->t3dr_image.timestamp = im->timestamp;

            //copy image data
            unsigned int size = im->width * im->height * 3 / 2;
            if (!appInstance->t3dr_image.data)
                appInstance->t3dr_image.data = new uint8_t[size];
            memcpy(appInstance->t3dr_image.data, im->data, size);

            //start processing
            struct thread_info *tinfo = 0;
            pthread_create(&appInstance->threadId, NULL, process, tinfo);
        }
    }

    void onPointCloudAvailableRouter(void *context, const TangoPointCloud *point_cloud) {
        oc::App *app = static_cast<oc::App *>(context);
        app->onPointCloudAvailable((TangoPointCloud*)point_cloud);
    }

    void onTangoEventRouter(void *context, const TangoEvent *event) {
        oc::App *app = static_cast<oc::App *>(context);
        app->onTangoEvent(event);
    }

    std::vector<std::string> separateByComma(std::string text) {
        std::vector<std::string> items;
        std::string temp;
        for(unsigned int i = 0; i < text.size(); i++)
        {
            char c = text[i];
            if(c == ',')
            {
                items.push_back(temp);
                temp.clear();
            }
            else
                temp.push_back(c);
        }
        if(!temp.empty())
            items.push_back(temp);
        return items;
    }
}

namespace oc {

    void App::onTangoEvent(const TangoEvent *event) {
        event_mutex_.lock();
        if (t3dr_is_running_)
        {
            if (strcmp(event->event_key, "ColorOverExposed") == 0)
            {
                event_ = "TOO_BRIGHT";
            }
            if (strcmp(event->event_key, "FisheyeOverExposed") == 0)
            {
                event_ = "TOO_BRIGHT";
            }
            if (strcmp(event->event_key, "ColorUnderExposed") == 0)
            {
                event_ = "TOO_DARK";
            }
            if (strcmp(event->event_key, "FisheyeUnderExposed") == 0)
            {
                event_ = "TOO_DARK";
            }
            if (strcmp(event->event_key, "TooFewFeaturesTracked") == 0)
            {
                event_ = "FEW_FEATURES";
            }
        }
        if (strcmp(event->event_key, "AreaDescriptionSaveProgress") == 0)
        {
            event_ = "AREADESCRIPTION";
        }
        event_mutex_.unlock();
    }

    std::string App::GetEvent() {
        event_mutex_.lock();
        std::string output = event_;
        if (!texturize.GetEvent().empty())
          output = texturize.GetEvent();
        event_ = "";
        event_mutex_.unlock();
        return output;
    }

    void App::StoreDataset(Tango3DR_PointCloud t3dr_depth, Tango3DR_Pose t3dr_depth_pose, Tango3DR_Pose t3dr_image_pose) {

        //export color camera frame and pose
        int pose = texturize.Add(t3dr_image, tango.Dataset());
        tango.Dataset().WritePose(pose, t3dr_image_pose.translation, t3dr_image_pose.orientation,
                                  t3dr_depth_pose.translation, t3dr_depth_pose.orientation,
                                  t3dr_image.timestamp, t3dr_depth.timestamp);

        //export calibration
        Tango3DR_CameraCalibration* c = tango.Camera();
        tango.Dataset().WriteCalibration(c->cx, c->cy, c->fx, c->fy);

        //get pose index
        int count, width, height, poseIndex;
        tango.Dataset().GetState(count, width, height);
        poseIndex = count - 1;

        //export point cloud
        scan.SavePointCloud(tango.Dataset(), poseIndex, t3dr_depth);

#ifdef EXPORT_DEPTHMAP
        bool compact = true;
        int w = compact ? t3dr_image.width / 10 : t3dr_image.width;
        int h = compact ? t3dr_image.height / 10 : t3dr_image.height;
        glm::mat4 pose = tango.Dataset().GetPose(poseIndex)[0];
        glm::mat4 world2uv = glm::inverse(pose);
        glm::vec4 v;
        oc::Image png(w, h);
        memset(png.GetData(), 0, w * h * 4);
        for (int i = 0; i < t3dr_depth.num_points; i++) {
            v = glm::vec4(t3dr_depth.points[i][0], t3dr_depth.points[i][1], t3dr_depth.points[i][2], 1);
            v = point_cloud_matrix_ * v;
            v /= fabs(v.w);
            v.w = 1.0f;
            glm::vec4 t = world2uv * v;
            t.x /= glm::abs(t.z * t.w);
            t.y /= glm::abs(t.z * t.w);
            t.x *= c->fx / (float)c->width;
            t.y *= c->fy / (float)c->height;
            t.x += c->cx / (float)c->width;
            t.y += c->cy / (float)c->height;
            int x = (int)(t.x * w);
            int y = (int)(t.y * h);
            if ((x >= 0) && (y >= 0) && (x < w) && (y < h)) {
                int dst = 100 * glm::length(glm::vec3(v.x, v.y, v.z) - glm::vec3(pose[3][0], pose[3][1], pose[3][2]));
                int index = (y * w + x) * 4;
                png.GetData()[index + 0] = glm::clamp(dst, 0, 255);
                png.GetData()[index + 1] = glm::clamp(dst - 255, 0, 255);
                png.GetData()[index + 2] = glm::clamp(dst - 510, 0, 255);
                png.GetData()[index + 3] = 255;
            }
        }
        png.Blur(compact ? 1 : 4);
        png.Write(tango.Dataset().GetFileName(poseIndex, ".png"));
#endif
    }

    void App::onPointCloudAvailable(TangoPointCloud *pc) {
        if (!running)
            return;
        std::vector<TangoMatrixTransformData> transform = tango.Pose(pc->timestamp, 0);
        if (transform[DEPTH_CAMERA].status_code != TANGO_POSE_VALID)
            return;

        binder_mutex_.lock();
        point_cloud_matrix_ = tango.Convert(transform)[DEPTH_CAMERA];
        TangoSupport_updatePointCloud(tango.Pointcloud(), pc);
        point_cloud_available_ = true;
        binder_mutex_.unlock();
    }

    App::App() :  t3dr_is_running_(false),
                  gyro(true),
                  landscape(false),
                  last_diff(2),
                  lastMovex(0),
                  lastMovey(0),
                  lastMovez(0),
                  lastPitch(0),
                  lastYaw(0),
                  point_cloud_available_(false),
                  running(true) {
        t3dr_image.data = 0;
        t3dr_image.format = TANGO_3DR_HAL_PIXEL_FORMAT_YCrCb_420_SP;
    }

    void App::OnTangoServiceConnected(JNIEnv *env, jobject binder, double res, double dmin,
                                      double dmax, int noise, bool land, bool sharpPhotos,
                                      bool fixHoles, bool clearing, bool correct, bool asus,
                                      std::string dataset) {
        correction = correct;
        landscape = land;
        photomode = res > 0.055;
        poisson = fixHoles;
        sharp = sharpPhotos;

        TangoService_setBinder(env, binder);
        tango.SetupConfig(dataset);
        texturize.SetResolution((float) res);
        bool newScan = tango.Dataset().AddSession();

        TangoErrorType ret = TangoService_connectOnPointCloudAvailable(onPointCloudAvailableRouter);
        if (ret != TANGO_SUCCESS)
            exit(EXIT_SUCCESS);
        ret = TangoService_connectOnFrameAvailable(TANGO_CAMERA_COLOR, this, onFrameAvailableRouter);
        if (ret != TANGO_SUCCESS)
            exit(EXIT_SUCCESS);
        ret = TangoService_connectOnTangoEvent(onTangoEventRouter);
        if (ret != TANGO_SUCCESS)
            exit(EXIT_SUCCESS);

        binder_mutex_.lock();
        scan.SetBuggyDevice(asus);
        tango.Connect(this);
        tango.Setup3DR(res, dmin, dmax, noise, clearing);

        if (newScan) {
            char* uuids = 0;
            TangoService_getAreaDescriptionUUIDList(&uuids);
            for (std::string uuid : separateByComma(uuids)) {
                LOGI("Deleting area: %s", uuid.c_str());
                TangoService_deleteAreaDescription(uuid.c_str());
            }
        }
        binder_mutex_.unlock();
    }

    void App::OnSurfaceChanged(int width, int height) {
        render_mutex_.lock();
        scene.SetupViewPort(width, height);
        selector.Init(width, height);
        render_mutex_.unlock();
    }

    void App::OnDrawFrame() {
        render_mutex_.lock();
        //camera transformation
        if (!gyro) {
            lastMovex = lastMovex * 0.9f + movex * 0.1f;
            lastMovey = lastMovey * 0.9f + movey * 0.1f;
            lastMovez = lastMovez * 0.9f + movez * 0.1f;
            lastPitch = lastPitch * 0.95f + pitch * 0.05f;
            lastYaw   = lastYaw   * 0.95f + yaw   * 0.05f;
            scene.renderer->camera.position = glm::vec3(lastMovex, lastMovez, lastMovey);
            scene.renderer->camera.rotation = glm::quat(glm::vec3(lastYaw, lastPitch, 0));
            scene.renderer->camera.scale    = glm::vec3(1, 1, 1);
        } else {
            std::vector<TangoMatrixTransformData> transform = tango.Pose(0, landscape);
            glm::mat4 matrix = tango.Convert(transform)[OPENGL_CAMERA];
            if (transform[OPENGL_CAMERA].status_code != TANGO_POSE_VALID)
                matrix = glm::mat4(1);
            scene.renderer->camera.SetTransformation(matrix);
            scene.UpdateFrustum(scene.renderer->camera.position, movez);
            glm::vec4 move = scene.renderer->camera.GetTransformation() * glm::vec4(0, 0, movez, 0);
            scene.renderer->camera.position += glm::vec3(move.x, move.y, move.z);
        }
        //render
        scene.Render(gyro);
        for (std::pair<GridIndex, Tango3DR_Mesh*> s : scan.Data()) {
            scene.renderer->Render(&s.second->vertices[0][0], 0, 0,
                                   (unsigned int*)&s.second->colors[0][0],
                                   s.second->num_faces * 3, &s.second->faces[0][0]);
        }
        render_mutex_.unlock();
    }

    void App::OnToggleButtonClicked(bool t3dr_is_running) {
        binder_mutex_.lock();
        t3dr_is_running_ = t3dr_is_running;
        binder_mutex_.unlock();
    }

    void App::OnClearButtonClicked() {
        binder_mutex_.lock();
        render_mutex_.lock();
        if (photomode) {
            if (!scene.static_meshes_.empty()) {
                scene.static_meshes_.erase(scene.static_meshes_.begin() + scene.static_meshes_.size() - 1);
                texturize.DeleteLast(tango.Dataset());
            }
        } else {
            scan.Clear();
            tango.Clear();
            texturize.Clear(tango.Dataset());
            tango.Dataset().ClearSessions();
            for (unsigned int i = 0; i < scene.static_meshes_.size(); i++)
                scene.static_meshes_[i].Destroy();
            scene.static_meshes_.clear();
        }
        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    void App::Load(std::string filename) {
        binder_mutex_.lock();
        render_mutex_.lock();
        File3d io(filename, false);
        io.ReadModel(kSubdivisionSize, scene.static_meshes_);
        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    bool App::Save(std::string filename) {
        running = false;
        OnToggleButtonClicked(false);

        binder_mutex_.lock();
        render_mutex_.lock();

        if (texturize.Test(tango.Context(), tango.Camera())) {

            //save area
            TangoUUID uuid;
            texturize.SetEvent("AREADESCRIPTION");
            if (TangoService_saveAreaDescription(&uuid) == TANGO_SUCCESS) {
                FILE *file = fopen((tango.Dataset().GetPath() + "/uuid.txt").c_str(), "w");
                fprintf(file, "%s", uuid);
                fclose(file);
            }

            if (correction && !photomode) {
                //pose correction
                Tango3DR_Trajectory trajectory = texturize.GetTrajectory(tango.Dataset());
                scan.CorrectPoses(tango.Dataset(), trajectory);
                Tango3DR_Trajectory_destroy(trajectory);
                tango.Disconnect();

                //clear scene
                scan.Clear();
                tango.Clear();
                texturize.SetEvent("RECONSTRUCTION");

                //prepare image cache
                int count, width, height;
                tango.Dataset().GetState(count, width, height);
                Tango3DR_ImageBuffer image;
                image.width = (uint32_t) width;
                image.height = (uint32_t) height;
                image.stride = (uint32_t) width;
                image.format = TANGO_3DR_HAL_PIXEL_FORMAT_YCrCb_420_SP;
                image.data = new unsigned char[width * height * 3];

                //mesh reconstruction
                std::vector<int> sessions = tango.Dataset().GetSessions();
                int offset = sessions[sessions.size() - 1];
                for (int i = offset; i < count; i++) {
                    std::ostringstream ss;
                    ss << "RECONSTRUCTION ";
                    ss << 100 * (i - offset) / (count - offset);
                    ss << "%";
                    texturize.SetEvent(ss.str());

                    Tango3DR_Pose t3dr_depth_pose, t3dr_image_pose;
                    tango.Dataset().GetPose(i, DEPTH_CAMERA, t3dr_depth_pose.translation, t3dr_depth_pose.orientation);
                    tango.Dataset().GetPose(i, COLOR_CAMERA, t3dr_image_pose.translation, t3dr_image_pose.orientation);
                    Tango3DR_PointCloud t3dr_depth = scan.LoadPointCloud(tango.Dataset(), i);

                    Tango3DR_Status ret;
                    Tango3DR_GridIndexArray t3dr_updated;
                    t3dr_depth.timestamp = tango.Dataset().GetPoseTime(i, DEPTH_CAMERA);
                    image.timestamp = tango.Dataset().GetPoseTime(i, COLOR_CAMERA);
                    Image::JPG2YUV(tango.Dataset().GetFileName(i, ".jpg"), image.data,
                                   width, height);
                    ret = Tango3DR_updateFromPointCloud(tango.Context(), &t3dr_depth,
                                                        &t3dr_depth_pose,
                                                        &image, &t3dr_image_pose,
                                                        &t3dr_updated);
                    if (ret == TANGO_3DR_SUCCESS) {
                        Tango3DR_GridIndexArray_destroy(&t3dr_updated);
                    }
                    Tango3DR_PointCloud_destroy(&t3dr_depth);
                }
                delete[] image.data;
            }

            //save mesh
            if (texturize.Init(tango.Context(), tango.Camera())) {
                texturize.Process(filename);
                scan.Clear();
                tango.Clear();
                File3d(filename, false).ReadModel(kSubdivisionSize, scene.static_meshes_);
            }
        }
        texturize.SetEvent("MERGE");
        if (photomode) {
            for (Mesh& m : scene.static_meshes_)
                m.GenerateNormals();
        }
        File3d(filename, true).WriteModel(scene.static_meshes_);
        int count = 0;
        for (Mesh& m : scene.static_meshes_)
            count += m.vertices.size();
        render_mutex_.unlock();
        binder_mutex_.unlock();
        return count != 0;
    }

    void App::SaveWithTextures(std::string filename) {
        binder_mutex_.lock();
        render_mutex_.lock();

        int index = 0;
        std::vector<std::string> names;
        for (Mesh& m : scene.static_meshes_) {
            if (m.imageOwner) {
                std::ostringstream ss;
                ss << filename.substr(0, filename.size() - 4).c_str();
                ss << "_";
                ss << index++;
                ss << ".png";
                names.push_back(m.image->GetName());
                m.image->SetName(ss.str());
                m.image->Write(ss.str());
            }
        }
        index = 0;
        File3d(filename, true).WriteModel(scene.static_meshes_);
        for (Mesh& m : scene.static_meshes_)
            if (m.imageOwner)
                m.image->SetName(names[index++]);
        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    void App::Texturize(std::string filename) {
        binder_mutex_.lock();
        render_mutex_.lock();
        tango.Disconnect();

        //poisson reconstruction
        if (poisson) {
            texturize.SetEvent("POISSON");
            Poisson().Process(filename);
            texturize.SetEvent("");
        }

        //check if texturize is valid
        if (!texturize.Init(filename, tango.Camera())) {
            render_mutex_.unlock();
            binder_mutex_.unlock();
            return;
        }

        //texturize
        scan.Clear();
        tango.Clear();
        texturize.ApplyFrames(tango.Dataset());
        texturize.Process(filename);
        texturize.Clear(tango.Dataset());

        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    float App::GetFloorLevel(float x, float y, float z) {
        binder_mutex_.lock();
        render_mutex_.lock();
        float output = INT_MAX;
        glm::vec3 p = glm::vec3(x, z, y);
        for (unsigned int i = 0; i < scene.static_meshes_.size(); i++) {
            float value = scene.static_meshes_[i].GetFloorLevel(p);
            if (value < INT_MIN + 1000)
                continue;
            if (output > value)
                output = value;
        }
        if (output > INT_MAX - 1000)
            output = INT_MIN;
        render_mutex_.unlock();
        binder_mutex_.unlock();
        return output;
    }

    void App::ApplyEffect(Effector::Effect effect, float value, int axis) {
        render_mutex_.lock();
        editor.ApplyEffect(scene.static_meshes_, effect, value, axis);
        scene.vertex = scene.TexturedVertexShader();
        scene.fragment = scene.TexturedFragmentShader();
        render_mutex_.unlock();
    }

    void App::PreviewEffect(Effector::Effect effect, float value, int axis) {
        render_mutex_.lock();
        std::string vs = scene.TexturedVertexShader();
        std::string fs = scene.TexturedFragmentShader();
        editor.PreviewEffect(vs, fs, effect, axis);
        scene.vertex = vs;
        scene.fragment = fs;
        scene.uniform = value / 255.0f;
        render_mutex_.unlock();
    }

    void App::ApplySelection(float x, float y, bool triangle) {
        render_mutex_.lock();
        glm::mat4 matrix = scene.renderer->camera.projection * scene.renderer->camera.GetView();
        if (triangle)
          selector.SelectTriangle(scene.static_meshes_, matrix, x, y);
        else
          selector.SelectObject(scene.static_meshes_, matrix, x, y);
        glm::vec3 center = selector.GetCenter(scene.static_meshes_);
        editor.SetCenter(center);
        scene.uniformPos = center;
        render_mutex_.unlock();
    }

    void App::CompleteSelection(bool inverse) {
        render_mutex_.lock();
        selector.CompleteSelection(scene.static_meshes_, inverse);
        glm::vec3 center = selector.GetCenter(scene.static_meshes_);
        editor.SetCenter(center);
        scene.uniformPos = center;
        render_mutex_.unlock();
    }

    void App::MultSelection(bool increase) {
        render_mutex_.lock();
        if (increase)
            selector.IncreaseSelection(scene.static_meshes_);
        else
            selector.DecreaseSelection(scene.static_meshes_);
        glm::vec3 center = selector.GetCenter(scene.static_meshes_);
        editor.SetCenter(center);
        scene.uniformPos = center;
        render_mutex_.unlock();
    }

    void App::RectSelection(float x1, float y1, float x2, float y2) {
        render_mutex_.lock();
        glm::mat4 matrix = scene.renderer->camera.projection * scene.renderer->camera.GetView();
        selector.SelectRect(scene.static_meshes_, matrix, x1, y1, x2, y2);
        glm::vec3 center = selector.GetCenter(scene.static_meshes_);
        editor.SetCenter(center);
        scene.uniformPos = center;
        render_mutex_.unlock();
    }

    void App::SetView(float p, float y, float mx, float my, float mz, bool g) {
        pitch = p;
        yaw = y;
        gyro = g;
        movex = mx;
        movey = my;
        movez = mz;
        editor.SetPitch(pitch);
        scene.uniformPitch = pitch;
    }

    bool App::AnimFinished() {
        render_mutex_.lock();
        bool output = (fabs(lastPitch - pitch) < 0.01f) && (fabs(lastYaw - yaw) < 0.01f);
        render_mutex_.unlock();
        return output;
    }
}


static oc::App app;

std::string jstring2string(JNIEnv* env, jstring name)
{
  const char *s = env->GetStringUTFChars(name,NULL);
  std::string str( s );
  env->ReleaseStringUTFChars(name,s);
  return str;
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_onTangoServiceConnected(JNIEnv* env, jobject,
          jobject iBinder, jdouble res, jdouble dmin, jdouble dmax, jint noise, jboolean land,
          jboolean sharp, jboolean fixholes, jboolean clearing, jboolean correction, jboolean asus,
                                                                   jstring d) {
  app.OnTangoServiceConnected(env, iBinder, res, dmin, dmax, noise, land, sharp, fixholes, clearing,
                              correction, asus, jstring2string(env, d));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_onGlSurfaceChanged(
    JNIEnv*, jobject, jint width, jint height) {
  app.OnSurfaceChanged(width, height);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_onGlSurfaceDrawFrame(JNIEnv*, jobject) {
  app.OnDrawFrame();
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_onToggleButtonClicked(
    JNIEnv*, jobject, jboolean t3dr_is_running) {
  app.OnToggleButtonClicked(t3dr_is_running);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_onClearButtonClicked(JNIEnv*, jobject) {
    app.OnClearButtonClicked();
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_load(JNIEnv* env, jobject, jstring name) {
  app.Load(jstring2string(env, name));
}

JNIEXPORT jboolean JNICALL
Java_com_lvonasek_openconstructor_main_JNI_save(JNIEnv* env, jobject, jstring name) {
    return app.Save(jstring2string(env, name));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_saveWithTextures(JNIEnv* env, jobject, jstring name) {
    app.SaveWithTextures(jstring2string(env, name));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_texturize(JNIEnv* env, jobject, jstring name) {
  app.Texturize(jstring2string(env, name));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_setView(JNIEnv*, jobject, jfloat pitch, jfloat yaw,
                                                         jfloat x, jfloat y, jfloat z, jboolean gyro) {
  app.SetView(pitch, yaw, x, y, z, gyro);
}

JNIEXPORT jfloat JNICALL
Java_com_lvonasek_openconstructor_main_JNI_getFloorLevel(JNIEnv*, jobject, jfloat x, jfloat y, jfloat z) {
    return app.GetFloorLevel(x, y, z);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_applyEffect(JNIEnv*, jobject, jint effect, jfloat value, jint axis) {
    app.ApplyEffect((oc::Effector::Effect) effect, value, axis);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_previewEffect(JNIEnv*, jobject, jint effect, jfloat value, jint axis) {
    app.PreviewEffect((oc::Effector::Effect) effect, value, axis);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_applySelect(JNIEnv*, jobject, jfloat x, jfloat y, jboolean triangle) {
    app.ApplySelection(x, y, triangle);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_completeSelection(JNIEnv*, jobject, jboolean inverse) {
    app.CompleteSelection(inverse);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_multSelection(JNIEnv*, jobject, jboolean increase) {
    app.MultSelection(increase);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_rectSelection(JNIEnv*, jobject, jfloat x1, jfloat y1,
                                                               jfloat x2, jfloat y2) {
    app.RectSelection(x1, y1, x2, y2);
}

JNIEXPORT jboolean JNICALL
Java_com_lvonasek_openconstructor_main_JNI_animFinished(JNIEnv*, jobject) {
    return (jboolean) app.AnimFinished();
}

JNIEXPORT jbyteArray JNICALL
Java_com_lvonasek_openconstructor_main_JNI_getEvent(JNIEnv* env, jobject) {
  std::string message = app.GetEvent();
  int byteCount = (int) message.length();
  const jbyte* pNativeMessage = reinterpret_cast<const jbyte*>(message.c_str());
  jbyteArray bytes = env->NewByteArray(byteCount);
  env->SetByteArrayRegion(bytes, 0, byteCount, pNativeMessage);
  return bytes;
}

#ifndef NDEBUG
JNIEXPORT jbyteArray JNICALL
Java_com_lvonasek_openconstructor_main_JNI_clientSecret(JNIEnv* env, jobject) {
  std::string message = "NO SECRET";
  int byteCount = (int) message.length();
  const jbyte* pNativeMessage = reinterpret_cast<const jbyte*>(message.c_str());
  jbyteArray bytes = env->NewByteArray(byteCount);
  env->SetByteArrayRegion(bytes, 0, byteCount, pNativeMessage);
  return bytes;
}
#else
#include "secret.h"
JNIEXPORT jbyteArray JNICALL
Java_com_lvonasek_openconstructor_main_JNI_clientSecret(JNIEnv* env, jobject) {
  std::string message = secret();
  int byteCount = message.length();
  const jbyte* pNativeMessage = reinterpret_cast<const jbyte*>(message.c_str());
  jbyteArray bytes = env->NewByteArray(byteCount);
  env->SetByteArrayRegion(bytes, 0, byteCount, pNativeMessage);
  return bytes;
}
#endif

#ifdef __cplusplus
}
#endif
