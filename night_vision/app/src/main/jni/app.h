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
        void OnTangoServiceConnected(JNIEnv *env, jobject binder, double res, double dmin, double dmax, int noise, bool updown);
        void onPointCloudAvailable(TangoPointCloud *pc);
        void OnSurfaceChanged(int width, int height);
        void OnDrawFrame();
    private:
        GLSL *shader_program_;
        GLuint attribute_vertices_;
        std::mutex binder_mutex_;
        std::mutex render_mutex_;
        TangoService tango;
        TangoPointCloud* front_cloud_;
        std::vector<glm::vec4> points;
        bool swap;
    };
}

#endif
