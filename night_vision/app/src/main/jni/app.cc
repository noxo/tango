#include <sstream>
#include "app.h"

namespace {
    std::string kVertexShader = "attribute vec4 vertex;\n"\
    "uniform float u_colors;\n"\
    "uniform float u_scaleX;\n"\
    "uniform float u_scaleY;\n"\
    "uniform float u_far;\n"\
    "uniform float u_near;\n"\
    "varying vec3 v_color;\n"\
    "void main() {\n"\
    "  gl_Position = vec4(vertex.x * u_scaleX, vertex.y * u_scaleY, vertex.z * 0.01, 1.0);\n"\
    "  gl_Position.xy /= vertex.z;\n"\
    "  gl_PointSize = u_near + vertex.z * (u_far - u_near) * 0.1;\n"\
    "  if (u_colors < 0.5)       //night vision\n"\
    "    v_color = vec3(0.0, vertex.z * 0.15, vertex.z * 0.05);\n"\
    "  else if (u_colors < 1.5)  //spectrum\n"\
    "    v_color = vec3(abs(sin(vertex.z * 0.25)), abs(sin(vertex.z * 0.5)), abs(sin(vertex.z)));\n"\
    "  else if (u_colors < 2.5)  //ghost light\n"\
    "    v_color = vec3(1.0 - 0.25 * vertex.z, 1.0 - 0.15 * vertex.z, 1.0 - 0.15 * vertex.z);\n"\
    "  else if (u_colors < 3.5)  //irda\n"\
    "    v_color = vec3(1.0, 1.0, 1.0) * vertex.w;\n"\
    "  else if (u_colors < 4.5)  //ir spectrum\n"\
    "    v_color = vec3(abs(sin(vertex.z)), abs(sin(vertex.z * 0.25)), abs(sin(vertex.z * 0.5))) * vertex.w;\n"\
    "}";

    std::string kFragmentShader = "varying vec3 v_color;\n"\
    "void main() {\n"\
    "    gl_FragColor = vec4(v_color, 1.0);\n"\
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
                    v.x = t3dr_depth.points[i][0] * (swap ? 1 : -1);
                    v.y = t3dr_depth.points[i][1] * (swap ? -1 : 1);
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

    void App::OnSurfaceChanged(int w, int h) {
        render_mutex_.lock();
        width = w;
        height = h;
        shader_program_ = new GLSL(kVertexShader, kFragmentShader);
        attribute_vertices_ = (GLuint) glGetAttribLocation(shader_program_->GetId(), "vertex");
        render_mutex_.unlock();
    }

    void App::OnDrawFrame() {
        render_mutex_.lock();
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (int i = 0; i < viewportCount; i++)  {
            int offset = (int)(width / viewportCount * eyeDistance);
            if (i % 2 == 1)
                offset = -offset;
            glViewport(i * width / viewportCount + offset, -abs(offset), width / viewportCount, height);
            if (!points.empty()) {
                shader_program_->Bind();
                shader_program_->UniformFloat("u_colors", colors);
                shader_program_->UniformFloat("u_far", far);
                shader_program_->UniformFloat("u_near", near);
                shader_program_->UniformFloat("u_scaleX", scaleX);
                shader_program_->UniformFloat("u_scaleY", scaleY);
                glEnable(GL_DEPTH_TEST);
                glEnableVertexAttribArray(attribute_vertices_);
                glVertexAttribPointer(attribute_vertices_, 4, GL_FLOAT, GL_FALSE, 0, points.data());
                glDrawArrays(GL_POINTS, 0, points.size());
                shader_program_->Unbind();
            }
        }
        render_mutex_.unlock();
    }


    void App::SetParams(float c) {
        colors = c;
    }

    void App::SetParams(float n, float f) {
        near = n;
        far = f;
    }

    void App::SetParams(int count, float sx, float sy, float dst) {
        scaleX = sx;
        scaleY = sy;
        eyeDistance = dst;
        viewportCount = count;
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

JNIEXPORT void JNICALL
Java_com_lvonasek_nightvision_JNI_setColorParams(
        JNIEnv*, jobject, jfloat colors) {
    app.SetParams(colors);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_nightvision_JNI_setPointParams(
        JNIEnv*, jobject, jfloat near, jfloat far) {
    app.SetParams(near, far);
}

JNIEXPORT void JNICALL
Java_com_lvonasek_nightvision_JNI_setViewParams(
        JNIEnv*, jobject, jint count, jfloat scaleX, jfloat scaleY, jfloat eyeDistance) {
    app.SetParams(count, scaleX, scaleY, eyeDistance);
}

#ifdef __cplusplus
}
#endif
