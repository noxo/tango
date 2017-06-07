#include "app.h"

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
        event_ = "Tango: " + std::string(event->event_key) + " " + std::string(event->event_value);
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

    void App::onPointCloudAvailable(TangoPointCloud *point_cloud) {
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

    void App::onFrameAvailable(TangoCameraId id, const TangoImageBuffer *buffer) {
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

        texturize.Add(t3dr_image, image_matrix, tango.Dataset());
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
                  gyro(false),
                  landscape(false),
                  lastMovex(0),
                  lastMovey(0),
                  lastMovez(0),
                  lastPitch(0),
                  lastYaw(0),
                  point_cloud_available_(false) {}

    void App::OnTangoServiceConnected(JNIEnv *env, jobject binder, double res,
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
            TangoMatrixTransformData transform;
            TangoSupport_getMatrixTransformAtTime(
                    0, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION, TANGO_COORDINATE_FRAME_DEVICE,
                    TANGO_SUPPORT_ENGINE_OPENGL, TANGO_SUPPORT_ENGINE_OPENGL,
                    landscape ? ROTATION_90 : ROTATION_0, &transform);
            if (transform.status_code == TANGO_POSE_VALID) {
                scene.renderer->camera.SetTransformation(glm::make_mat4(transform.matrix));
                scene.UpdateFrustum(scene.renderer->camera.position, movez);
                glm::vec4 move = scene.renderer->camera.GetTransformation() * glm::vec4(0, 0, movez, 0);
                scene.renderer->camera.position += glm::vec3(move.x, move.y, move.z);
            }
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
        scan.Clear();
        tango.Clear();
        texturize.Clear();
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

    void App::Texturize(std::string filename) {
        binder_mutex_.lock();
        render_mutex_.lock();

        //check if texturizing is valid
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
        texturize.Clear();

        //reload the model
        for (unsigned int i = 0; i < scene.static_meshes_.size(); i++)
            scene.static_meshes_[i].Destroy();
        scene.static_meshes_.clear();
        File3d io(filename, false);
        io.ReadModel(kSubdivisionSize, scene.static_meshes_);

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

    void App::ApplyEffect(Effector::Effect effect, float value) {
        render_mutex_.lock();
        editor.ApplyEffect(scene.static_meshes_, effect, value);
        scene.vertex = scene.TexturedVertexShader();
        scene.fragment = scene.TexturedFragmentShader();
        render_mutex_.unlock();
    }

    void App::PreviewEffect(Effector::Effect effect, float value) {
        render_mutex_.lock();
        std::string vs = scene.TexturedVertexShader();
        std::string fs = scene.TexturedFragmentShader();
        editor.PreviewEffect(vs, fs, effect);
        scene.vertex = vs;
        scene.fragment = fs;
        scene.uniform = value / 255.0f;
        render_mutex_.unlock();
    }

    void App::ApplySelection(float x, float y) {
        render_mutex_.lock();
        glm::mat4 matrix = scene.renderer->camera.projection * scene.renderer->camera.GetView();
        selector.ApplySelection(scene.static_meshes_, matrix, x, y);
        render_mutex_.unlock();
    }

    void App::CompleteSelection(bool inverse) {
        render_mutex_.lock();
        selector.CompleteSelection(scene.static_meshes_, inverse);
        render_mutex_.unlock();
    }

    void App::MultSelection(bool increase) {
        render_mutex_.lock();
        if (increase)
            selector.IncreaseSelection(scene.static_meshes_);
        else
            selector.DecreaseSelection(scene.static_meshes_);
        render_mutex_.unlock();
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
Java_com_lvonasek_openconstructor_TangoJNINative_onTangoServiceConnected(JNIEnv* env, jobject,
          jobject iBinder, jdouble res, jdouble dmin, jdouble dmax, jint noise, jboolean land,
                                                                              jstring dataset) {
  app.OnTangoServiceConnected(env, iBinder, res, dmin, dmax, noise, land, jstring2string(env, dataset));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onGlSurfaceChanged(
    JNIEnv*, jobject, jint width, jint height) {
  app.OnSurfaceChanged(width, height);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onGlSurfaceDrawFrame(JNIEnv*, jobject) {
  app.OnDrawFrame();
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onToggleButtonClicked(
    JNIEnv*, jobject, jboolean t3dr_is_running) {
  app.OnToggleButtonClicked(t3dr_is_running);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_onClearButtonClicked(JNIEnv*, jobject) {
  app.OnClearButtonClicked();
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_load(JNIEnv* env, jobject, jstring name) {
  app.Load(jstring2string(env, name));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_save(JNIEnv* env, jobject, jstring name) {
  app.Save(jstring2string(env, name));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_texturize(JNIEnv* env, jobject, jstring name) {
  app.Texturize(jstring2string(env, name));
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_setView(JNIEnv*, jobject, jfloat pitch, jfloat yaw,
                                                         jfloat x, jfloat y, jfloat z, jboolean gyro) {
  app.SetView(pitch, yaw, x, y, z, gyro);
}

JNIEXPORT jfloat JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_getFloorLevel(JNIEnv*, jobject, jfloat x, jfloat y, jfloat z) {
    return app.GetFloorLevel(x, y, z);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_applyEffect(JNIEnv*, jobject, jint effect, jfloat value) {
    app.ApplyEffect((oc::Effector::Effect) effect, value);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_previewEffect(JNIEnv*, jobject, jint effect, jfloat value) {
    app.PreviewEffect((oc::Effector::Effect) effect, value);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_applySelect(JNIEnv*, jobject, jfloat x, jfloat y) {
    app.ApplySelection(x, y);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_completeSelection(JNIEnv*, jobject, jboolean inverse) {
    app.CompleteSelection(inverse);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_multSelection(JNIEnv*, jobject, jboolean increase) {
    app.MultSelection(increase);
}

JNIEXPORT jbyteArray JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_getEvent(JNIEnv* env, jobject) {
  std::string message = app.GetEvent();
  int byteCount = (int) message.length();
  const jbyte* pNativeMessage = reinterpret_cast<const jbyte*>(message.c_str());
  jbyteArray bytes = env->NewByteArray(byteCount);
  env->SetByteArrayRegion(bytes, 0, byteCount, pNativeMessage);
  return bytes;
}

#ifndef NDEBUG
JNIEXPORT jbyteArray JNICALL
Java_com_lvonasek_openconstructor_TangoJNINative_clientSecret(JNIEnv* env, jobject) {
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
Java_com_lvonasek_openconstructor_TangoJNINative_clientSecret(JNIEnv* env, jobject) {
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
