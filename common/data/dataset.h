#ifndef DATA_DATASET_H
#define DATA_DATASET_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace oc {
    class Dataset {
    public:

        Dataset(std::string path);
        void GetCalibration(float& cx, float& cy, float& fx, float& fy);
        std::string GetFileName(int index, std::string extension);
        std::string GetPath() { return dataset; }
        std::vector<glm::mat4> GetPose(int index);
        double GetPoseTime(int index);
        void GetState(int& count, int& width, int& height);
        void WriteCalibration(double cx, double cy, double fx, double fy);
        void WritePose(int index, std::vector<glm::mat4> pose, double timestamp);
        void WriteState(int count, int width, int height);
    private:
        std::string dataset;
    };
}
#endif
