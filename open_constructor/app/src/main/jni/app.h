#ifndef APP_H
#define APP_H

#include <jni.h>
#include <mutex>
#include <string>

#include "tango/scan.h"
#include "tango/service.h"
#include "tango/texturize.h"
#include "scene.h"

namespace oc {

    class App {
    public:
        App();
        void OnCreate(JNIEnv *env, jobject caller_activity);
        void OnTangoServiceConnected(JNIEnv *env, jobject binder, double res, double dmin, double dmax,
                                     int noise, bool land, std::string dataset);
        void onPointCloudAvailable(TangoPointCloud *point_cloud);
        void onFrameAvailable(TangoCameraId id, const TangoImageBuffer *buffer);
        void onTangoEvent(const TangoEvent *event);
        void OnSurfaceCreated();
        void OnSurfaceChanged(int width, int height);
        void OnDrawFrame();
        void OnToggleButtonClicked(bool t3dr_is_running);
        void OnClearButtonClicked();

        void Load(std::string filename);
        void Save(std::string filename, std::string dataset);
        void Texturize(std::string filename, std::string dataset);

        void SetView(float p, float y, float mx, float my, bool g) { pitch = p; yaw = y; gyro = g;
                                                                            movex = mx; movey = my;}
        void SetZoom(float value) { zoom = value; }
        std::string GetEvent();

    private:
        bool t3dr_is_running_;
        bool point_cloud_available_;
        TangoPointCloud* front_cloud_;
        glm::mat4 point_cloud_matrix_;
        glm::mat4 image_matrix;
        glm::quat image_rotation;
        std::mutex binder_mutex_;
        std::mutex render_mutex_;
        std::mutex event_mutex_;
        std::string event_;

        Scene scene;
        TangoScan scan;
        TangoService tango;
        TangoTexturize texturize;

        bool gyro;
        bool landscape;
        float movex;
        float movey;
        float pitch;
        float yaw;
        float zoom;
    };
}

#endif
