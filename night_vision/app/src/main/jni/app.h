#ifndef APP_H
#define APP_H

#include <jni.h>
#include <mutex>
#include <string>
#include "gl/glsl.h"
#include "tango/service.h"

namespace oc {

    class App {
    public:
        void OnTangoServiceConnected(JNIEnv *env, jobject binder, bool updown);
        void onPointCloudAvailable(TangoPointCloud *pc);
        void OnSurfaceChanged(int w, int h);
        void OnDrawFrame();
        void SetParams(float c);
        void SetParams(float n, float f);
        void SetParams(int count, float sx, float sy, float dst, float off);
    private:
        GLSL *shader_program_;
        GLuint attribute_vertices_;
        std::mutex binder_mutex_;
        std::mutex render_mutex_;
        TangoService tango;
        TangoPointCloud* front_cloud_;
        std::vector<glm::vec4> points;
        bool swap;
        int width, height, viewportCount;
        float colors, near, far, scaleX, scaleY, eyeDistance, offset;
    };
}

#endif
