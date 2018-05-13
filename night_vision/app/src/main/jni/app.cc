#include <sstream>
#include "app.h"

namespace {
    std::string kVertexShader = "attribute vec4 vertex;\n"\
    "varying float v_depth;\n"\
    "void main() {\n"\
    "  gl_Position = vec4(vertex.x * 2.0, vertex.y, vertex.z * 0.01, 1.0);\n"\
    "  gl_Position.xy /= vertex.z;     //perspective correction\n"\
    "  gl_Position.xy *= 1.5;          //bigger picture\n"\
    "  gl_PointSize = vertex.z * 25.0; //more distant points bigger\n"\
    "  v_depth = vertex.z;\n"\
    "}";

    std::string kFragmentShader = "varying float v_depth;\n"\
    "void main() {\n"\
    "  gl_FragColor = vec4(0.0, v_depth * 0.2, v_depth * 0.05, 1.0);\n"\
    "}";

    void onPointCloudAvailableRouter(void *context, const TangoPointCloud *point_cloud) {
        oc::App *app = static_cast<oc::App *>(context);
        app->onPointCloudAvailable((TangoPointCloud*)point_cloud);
    }
}

namespace oc {

    void App::onPointCloudAvailable(TangoPointCloud *pc) {
        binder_mutex_.lock();
        if (TangoSupport_updatePointCloud(tango.Pointcloud(), pc) == TANGO_SUCCESS) {
            if (TangoSupport_getLatestPointCloud(tango.Pointcloud(), &front_cloud_) == TANGO_SUCCESS) {
                Tango3DR_PointCloud t3dr_depth;
                t3dr_depth.timestamp = front_cloud_->timestamp;
                t3dr_depth.num_points = front_cloud_->num_points;
                t3dr_depth.points = front_cloud_->points;

                render_mutex_.lock();
                points.clear();
                for (int i = 0; i < t3dr_depth.num_points; i++) {
                    glm::vec4 v;
                    v.x = t3dr_depth.points[i][1] *  (swap ? -1 : 1);
                    v.y = t3dr_depth.points[i][0] *  (swap ? -1 : 1);
                    v.z = t3dr_depth.points[i][2];
                    v.w = t3dr_depth.points[i][3];
                    points.push_back(v);
                }
                render_mutex_.unlock();
            }
        }
        binder_mutex_.unlock();
    }

    void App::OnTangoServiceConnected(JNIEnv *env, jobject binder, bool updown) {
        binder_mutex_.lock();
        TangoService_setBinder(env, binder);
        tango.SetupConfig("");
        swap = updown;
        if (TangoService_connectOnPointCloudAvailable(onPointCloudAvailableRouter) != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        tango.Connect(this);
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
        glClearColor(0.0f, 0.1f, 0.2f, 1.0f);
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
Java_com_lvonasek_nightvision_JNI_onTangoServiceConnected(JNIEnv* env, jobject, jobject iBinder, jboolean updown) {
  app.OnTangoServiceConnected(env, iBinder, updown);
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
