#ifndef DATA_DATASET_H
#define DATA_DATASET_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace oc {
    enum Pose { COLOR_CAMERA, DEPTH_CAMERA, OPENGL_CAMERA, MAX_CAMERA };

    class Dataset {
    public:

        Dataset(std::string path);
        bool AddSession();
        void ClearSessions();
        void GetCalibration(float& cx, float& cy, float& fx, float& fy);
        std::string GetFileName(int index, std::string extension);
        std::string GetPath() { return dataset; }
        void GetPose(int index, int pose, double* translation, double* orientation);
        double GetPoseTime(int index, int pose);
        std::vector<int> GetSessions();
        void GetState(int& count, int& width, int& height);
        void WriteCalibration(double cx, double cy, double fx, double fy);
        void WritePose(int index, double* imageTranslation, double* imageOrientation,
                       double* depthTranslation, double* depthOrientation,
                       double imageTimestamp, double depthTimestamp);
        void WriteState(int count, int width, int height);
    private:
        std::string dataset;
    };
}
#endif
