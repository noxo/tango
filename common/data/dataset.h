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
        std::vector<glm::mat4> GetPose(int index);
        double GetPoseTime(int index, Pose pose);
        std::vector<int> GetSessions();
        void GetState(int& count, int& width, int& height);
        void WriteCalibration(double cx, double cy, double fx, double fy);
        void WritePose(int index, std::vector<glm::mat4> pose, double imageTimestamp, double depthTimestamp);
        void WriteState(int count, int width, int height);
    private:
        std::string dataset;
    };
}
#endif
