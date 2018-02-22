#include <sstream>
#include "app.h"

#define EXPORT_POINTCLOUD

namespace {
    const int kSubdivisionSize = 20000;

    void onPointCloudAvailableRouter(void *context, const TangoPointCloud *point_cloud) {
        oc::App *app = static_cast<oc::App *>(context);
        app->onPointCloudAvailable((TangoPointCloud*)point_cloud);
    }

    void onFrameAvailableRouter(void *context, TangoCameraId id, const TangoImageBuffer *buffer) {
        oc::App *app = static_cast<oc::App *>(context);
        app->onFrameAvailable(id, buffer);
    }

    void onTangoEventRouter(void *context, const TangoEvent *event) {
        oc::App *app = static_cast<oc::App *>(context);
        app->onTangoEvent(event);
    }
}

namespace oc {

    void App::onTangoEvent(const TangoEvent *event) {
        event_mutex_.lock();
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

    void App::StorePointCloud(Tango3DR_PointCloud t3dr_depth) {
#ifdef EXPORT_POINTCLOUD
        std::vector<Mesh> pcl;
        Mesh m;
        glm::vec4 v;
        for (int i = 0; i < t3dr_depth.num_points; i++) {
            v = glm::vec4 (t3dr_depth.points[i][0], t3dr_depth.points[i][1], t3dr_depth.points[i][2], 1);
            v = point_cloud_matrix_ * v;
            v /= fabs(v.w);
            m.vertices.push_back(glm::vec3(v.x, v.y, -v.z));
        }
        pcl.push_back(m);
        File3d file(texturize.GetFileName(texturize.GetLatestIndex(tango.Dataset()), tango.Dataset(), ".ply").c_str(), true);
        file.WriteModel(pcl);
#endif
    }

    void App::onPointCloudAvailable(TangoPointCloud *pc) {
        if (!t3dr_is_running_)
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

    void App::onFrameAvailable(TangoCameraId id, const TangoImageBuffer *im) {
        if (id != TANGO_CAMERA_COLOR || (!t3dr_is_running_ && !scene.IsFullScreenActive()))
            return;

        std::vector<TangoMatrixTransformData> transform = tango.Pose(im->timestamp, 0);
        if (transform[COLOR_CAMERA].status_code != TANGO_POSE_VALID)
            return;

        binder_mutex_.lock();
        if (!point_cloud_available_ && !scene.IsFullScreenActive()) {
            binder_mutex_.unlock();
            return;
        }

        Tango3DR_ImageBuffer t3dr_image;
        t3dr_image.width = im->width;
        t3dr_image.height = im->height;
        t3dr_image.stride = im->stride;
        t3dr_image.timestamp = im->timestamp;
        t3dr_image.format = static_cast<Tango3DR_ImageFormatType>(im->format);
        t3dr_image.data = im->data;

        if (scene.IsFullScreenActive()) {
            Image* img = new Image(t3dr_image.data, t3dr_image.width, t3dr_image.height, 1);
            render_mutex_.lock();
            scene.SetPreview(img);
            float match = 100.0f * (float)scene.CountMatching();
            char res[8];
            sprintf(res, "%.1f", match);
            event_mutex_.lock();
            event_ = "Matching: current=" + std::string(res) + "%";
            sprintf(res, "%.1f", best_match);
            event_ += " best=" + std::string(res) + "%";
            if (best_match < match) {
                best_match = match;
                std::vector<glm::mat4> toZero = tango.Convert(transform);
                for (unsigned int i = 0; i < toZero.size(); i++)
                    toZero[i] = glm::inverse(toZero[i]);
                tango.SetupTransform(texturize.GetLatestPose(tango.Dataset()), toZero);
            }
            if (best_match > 75)
                event_ += "\nPress the record icon to continue";
            else
                event_ += "\nFind position matching this photo";
            event_mutex_.unlock();
            render_mutex_.unlock();
            binder_mutex_.unlock();
            return;
        }

        Tango3DR_Pose t3dr_image_pose = TangoService::Extract3DRPose(tango.Convert(transform)[COLOR_CAMERA]);
        if (sharp) {
            glm::vec3 pos = glm::vec3((float) t3dr_image_pose.translation[0],
                                      (float) t3dr_image_pose.translation[1],
                                      (float) t3dr_image_pose.translation[2]);
            glm::quat rot = glm::quat((float) t3dr_image_pose.orientation[0],
                                      (float) t3dr_image_pose.orientation[1],
                                      (float) t3dr_image_pose.orientation[2],
                                      (float) t3dr_image_pose.orientation[3]);
            float value = GLCamera::Diff(pos, image_position, rot, image_rotation);
            float diff = value > last_diff ? value : 0.95f * last_diff + 0.05f * value;
            last_diff = diff;
            image_position = pos;
            image_rotation = rot;
            if (diff > 1) {
                binder_mutex_.unlock();
                return;
            }
        }

        Tango3DR_PointCloud t3dr_depth;
        TangoSupport_getLatestPointCloud(tango.Pointcloud(), &front_cloud_);
        t3dr_depth.timestamp = front_cloud_->timestamp;
        t3dr_depth.num_points = front_cloud_->num_points;
        t3dr_depth.points = front_cloud_->points;

        Tango3DR_Pose t3dr_depth_pose = TangoService::Extract3DRPose(point_cloud_matrix_);
        Tango3DR_GridIndexArray t3dr_updated;
        Tango3DR_Status ret;
        ret = Tango3DR_updateFromPointCloud(tango.Context(), &t3dr_depth, &t3dr_depth_pose,
                                            &t3dr_image, &t3dr_image_pose, &t3dr_updated);
        if (ret != TANGO_3DR_SUCCESS) {
            binder_mutex_.unlock();
            return;
        }

        texturize.Add(t3dr_image, tango.Convert(transform), tango.Dataset());
        StorePointCloud(t3dr_depth);
        std::vector<std::pair<GridIndex, Tango3DR_Mesh*> > added;
        added = scan.Process(tango.Context(), &t3dr_updated);
        render_mutex_.lock();
        scan.Merge(added);
        render_mutex_.unlock();

        Tango3DR_GridIndexArray_destroy(&t3dr_updated);
        point_cloud_available_ = false;
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
                  point_cloud_available_(false) {}

    void App::OnTangoServiceConnected(JNIEnv *env, jobject binder, double res, double dmin,
                                      double dmax, int noise, bool land, bool sharpPhotos,
                                      std::string dataset) {
        landscape = land;
        sharp = sharpPhotos;

        TangoService_setBinder(env, binder);
        tango.SetupConfig(dataset);
        texturize.SetResolution((float) res);

        TangoErrorType ret = TangoService_connectOnPointCloudAvailable(onPointCloudAvailableRouter);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = TangoService_connectOnFrameAvailable(TANGO_CAMERA_COLOR, this, onFrameAvailableRouter);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = TangoService_connectOnTangoEvent(onTangoEventRouter);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        binder_mutex_.lock();
        tango.Connect(this);
        tango.Setup3DR(res, dmin, dmax, noise);
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
        if (t3dr_is_running && scene.IsFullScreenActive()) {
            render_mutex_.lock();
            scene.SetFullScreen(0);
            scene.SetPreview(0);
            tango.ApplyTransform();
            render_mutex_.unlock();
        }
        binder_mutex_.unlock();
    }

    void App::OnClearButtonClicked() {
        binder_mutex_.lock();
        render_mutex_.lock();
        scan.Clear();
        tango.Clear();
        texturize.Clear(tango.Dataset());
        for (unsigned int i = 0; i < scene.static_meshes_.size(); i++)
            scene.static_meshes_[i].Destroy();
        scene.static_meshes_.clear();
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

    void App::Save(std::string filename) {
        binder_mutex_.lock();
        render_mutex_.lock();

        if (texturize.Init(tango.Context(), tango.Camera())) {
            texturize.Process(filename);

            //merge with previous OBJ
            scan.Clear();
            tango.Clear();
            File3d(filename, false).ReadModel(kSubdivisionSize, scene.static_meshes_);
        }
        File3d(filename, true).WriteModel(scene.static_meshes_);
        render_mutex_.unlock();
        binder_mutex_.unlock();
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

    void App::OnResumeScanning() {
        best_match = 0;
        scene.SetFullScreen(texturize.GetLatestImage(tango.Dataset()));
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
          jboolean sharp, jstring d) {
  app.OnTangoServiceConnected(env, iBinder, res, dmin, dmax, noise, land, sharp, jstring2string(env, d));
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
Java_com_lvonasek_openconstructor_main_JNI_onResumeScanning(JNIEnv*, jobject) {
    app.OnResumeScanning();
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_load(JNIEnv* env, jobject, jstring name) {
  app.Load(jstring2string(env, name));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_main_JNI_save(JNIEnv* env, jobject, jstring name) {
    app.Save(jstring2string(env, name));
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
