#include "data/dataset.h"
#include <sstream>

namespace oc {
    Dataset::Dataset(std::string path) {
        dataset = path;
    }

    bool Dataset::AddSession() {
        int count, width, height;
        GetState(count, width, height);
        FILE* file = fopen((GetPath() + "/session.txt").c_str(), "a");
        fprintf(file, "%d\n", count);
        fclose(file);
        return count == 0;
    }

    void Dataset::ClearSessions() {
        FILE* file = fopen((GetPath() + "/session.txt").c_str(), "w");
        fprintf(file, "0\n");
        fclose(file);
    }

    void Dataset::GetCalibration(float &cx, float &cy, float &fx, float &fy) {
        FILE* file = fopen((dataset + "/calibration.txt").c_str(), "r");
        fscanf(file, "%f %f %f %f", &cx, &cy, &fx, &fy);
        fclose(file);
    }

    std::string Dataset::GetFileName(int index, std::string extension) {
        std::ostringstream ss;
        ss << index;
        std::string number = ss.str();
        while(number.size() < 8)
            number = "0" + number;
        return dataset + "/" + number + extension;
    }

    void Dataset::GetPose(int index, int pose, double* translation, double* orientation) {
        int count = 0;
        glm::mat4 mat;
        std::vector<glm::mat4> output;
        FILE* file = fopen(GetFileName(index, ".pos").c_str(), "r");
        for (int i = 0; i <= pose; i++) {
            fscanf(file, "%lf %lf %lf %lf %lf %lf %lf\n", &translation[0], &translation[1], &translation[2],
                   &orientation[0], &orientation[1], &orientation[2], &orientation[3]);
        }
        fclose(file);
    }

    double Dataset::GetPoseTime(int index, int pose) {
        double timestamp;
        FILE* file = fopen(GetFileName(index, ".pos").c_str(), "r");
        for (int i = 0; i < 2; i++)
            fscanf(file, "%lf %lf %lf %lf %lf %lf %lf\n", &timestamp, &timestamp, &timestamp,
                   &timestamp, &timestamp, &timestamp, &timestamp);
        for (int i = 0; i <= pose; i++)
            fscanf(file, "%lf\n", &timestamp);
        fclose(file);
        return timestamp;
    }

    std::vector<int> Dataset::GetSessions() {
        int session = 0;
        std::vector<int> sessions;
        {
            FILE* file = fopen((GetPath() + "/session.txt").c_str(), "r");
            while (!feof(file)) {
                fscanf(file, "%d\n", &session);
                sessions.push_back(session);
            }
            fclose(file);
        }
        return sessions;
    }

    void Dataset::GetState(int &count, int &width, int &height) {
        FILE* file = fopen((dataset + "/state.txt").c_str(), "r");
        if (file) {
            fscanf(file, "%d %d %d\n", &count, &width, &height);
            fclose(file);
        } else {
            count = 0;
        }
    }

    void Dataset::WriteCalibration(double cx, double cy, double fx, double fy) {
        FILE* file = fopen((dataset + "/calibration.txt").c_str(), "w");
        fprintf(file, "%f %f %f %f", cx, cy, fx, fy);
        fclose(file);
    }

    void Dataset::WritePose(int index, double* imageTranslation, double* imageOrientation,
                            double* depthTranslation, double* depthOrientation,
                            double imageTimestamp, double depthTimestamp) {
        FILE* file = fopen(GetFileName(index, ".pos").c_str(), "w");
        fprintf(file, "%lf %lf %lf %lf %lf %lf %lf\n",
                imageTranslation[0], imageTranslation[1], imageTranslation[2],
                imageOrientation[0], imageOrientation[1], imageOrientation[2], imageOrientation[3]);
        fprintf(file, "%lf %lf %lf %lf %lf %lf %lf\n",
                depthTranslation[0], depthTranslation[1], depthTranslation[2],
                depthOrientation[0], depthOrientation[1], depthOrientation[2], depthOrientation[3]);

        fprintf(file, "%lf\n", imageTimestamp);
        fprintf(file, "%lf\n", depthTimestamp);
        fclose(file);
    }

    void Dataset::WriteState(int count, int width, int height) {
        FILE* file = fopen((dataset + "/state.txt").c_str(), "w");
        fprintf(file, "%d %d %d\n", count, width, height);
        fclose(file);
    }
}
