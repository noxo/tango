#include <sstream>
#include "app.h"

namespace {
    std::string kVertexShader = "attribute vec4 vertex;\n"\
    "varying float v_color;\n"\
    "void main() {\n"\
    "  gl_PointSize = 50.0;\n"\
    "  gl_Position = vec4(vertex.xyz, 1.0);\n"\
    "  v_color = gl_Position.z * 10.0;\n"\
    "}";

    std::string kFragmentShader = "varying float v_color;\n"\
    "void main() {\n"\
    "  gl_FragColor = vec4(v_color, v_color * 4.0, v_color * 2.0, 1.0);\n"\
    "}";

    void onPointCloudAvailableRouter(void *context, const TangoPointCloud *point_cloud) {
        oc::App *app = static_cast<oc::App *>(context);
        app->onPointCloudAvailable((TangoPointCloud*)point_cloud);
    }
}

namespace oc {

    void App::onPointCloudAvailable(TangoPointCloud *pc) {

        binder_mutex_.lock();
        TangoSupport_updatePointCloud(tango.Pointcloud(), pc);

        Tango3DR_PointCloud t3dr_depth;
        TangoSupport_getLatestPointCloud(tango.Pointcloud(), &front_cloud_);
        t3dr_depth.timestamp = front_cloud_->timestamp;
        t3dr_depth.num_points = front_cloud_->num_points;
        t3dr_depth.points = front_cloud_->points;

        render_mutex_.lock();
        points.clear();
        for (int i = 0; i < t3dr_depth.num_points; i++) {
            glm::vec4 v;
            v.x = 2.0f * t3dr_depth.points[i][1] *  (swap ? -1 : 1);
            v.y = t3dr_depth.points[i][0] *  (swap ? -1 : 1);
            v.z = t3dr_depth.points[i][2] * 0.01f;
            v.w = 1.0f;
            points.push_back(v);
        }
        render_mutex_.unlock();
        binder_mutex_.unlock();
    }

    void App::OnTangoServiceConnected(JNIEnv *env, jobject binder, double res, double dmin, double dmax, int noise, bool updown) {
        TangoService_setBinder(env, binder);
        tango.SetupConfig("");
        swap = updown;

        TangoErrorType ret = TangoService_connectOnPointCloudAvailable(onPointCloudAvailableRouter);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        binder_mutex_.lock();
        tango.Connect(this);
        tango.Setup3DR(res, dmin, dmax, noise, false);
        binder_mutex_.unlock();
    }

    void App::OnSurfaceChanged(int width, int height) {
        render_mutex_.lock();
        glViewport(0, 0, width, height);
        shader_program_ = new GLSL(kVertexShader, kFragmentShader);
        attribute_vertices_ = (GLuint) glGetAttribLocation(shader_program_->GetId(), "vertex");
        render_mutex_.unlock();
    }

    void App::OnDrawFrame() {
        render_mutex_.lock();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!points.empty()) {
            shader_program_->Bind();
            glEnable(GL_DEPTH_TEST);
            glEnableVertexAttribArray(attribute_vertices_);
            glVertexAttribPointer(attribute_vertices_, 4, GL_FLOAT, GL_FALSE, 0, points.data());
            glDrawArrays(GL_POINTS, 0, points.size());
            shader_program_->Unbind();
        }
        render_mutex_.unlock();
    }
}


static oc::App app;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL
Java_com_lvonasek_nightvision_JNI_onTangoServiceConnected(JNIEnv* env, jobject,
          jobject iBinder, jdouble res, jdouble dmin, jdouble dmax, jint noise, jboolean updown) {
  app.OnTangoServiceConnected(env, iBinder, res, dmin, dmax, noise, updown);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_nightvision_JNI_onGlSurfaceChanged(
    JNIEnv*, jobject, jint width, jint height) {
  app.OnSurfaceChanged(width, height);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_nightvision_JNI_onGlSurfaceDrawFrame(JNIEnv*, jobject) {
  app.OnDrawFrame();
}

#ifdef __cplusplus
}
#endif
