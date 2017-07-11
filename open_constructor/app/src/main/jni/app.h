#ifndef APP_H
#define APP_H

#include <jni.h>
#include <mutex>
#include <string>

#include "editor/effector.h"
#include "editor/selector.h"
#include "tango/scan.h"
#include "tango/service.h"
#include "tango/texturize.h"
#include "scene.h"

namespace oc {

    class App {
    public:
        App();
        void OnTangoServiceConnected(JNIEnv *env, jobject binder, double res, double dmin, double dmax,
                                     int noise, bool land, std::string dataset);
        void onPointCloudAvailable(TangoPointCloud *point_cloud);
        void onEyeFrameAvailable(TangoCameraId id, const TangoImageBuffer *buffer);
        void onFrameAvailable(TangoCameraId id, const TangoImageBuffer *buffer);
        void onTangoEvent(const TangoEvent *event);
        void OnSurfaceChanged(int width, int height);
        void OnDrawFrame();
        void OnToggleButtonClicked(bool t3dr_is_running);
        void OnClearButtonClicked();

        void Load(std::string filename);
        void Save(std::string filename);
        void SaveWithTextures(std::string filename);
        void Texturize(std::string filename);

        float GetFloorLevel(float x, float y, float z);
        void SetView(float p, float y, float mx, float my, float mz, bool g);
        std::string GetEvent();

        void ApplyEffect(Effector::Effect effect, float value, int axis);
        void PreviewEffect(Effector::Effect effect, float value, int axis);

        void ApplySelection(float x, float y, bool triangle);
        void CompleteSelection(bool inverse);
        void MultSelection(bool increase);
        void RectSelection(float x1, float y1, float x2, float y2);

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

        Effector editor;
        Scene scene;
        Selector selector;
        TangoScan scan;
        TangoService tango;
        TangoTexturize texturize;

        bool gyro;
        bool landscape;
        float movex, lastMovex;
        float movey, lastMovey;
        float movez, lastMovez;
        float pitch, lastPitch;
        float yaw, lastYaw;
    };
}

#endif
