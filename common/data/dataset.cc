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

    std::vector<glm::mat4> Dataset::GetPose(int index) {
        int count = 0;
        glm::mat4 mat;
        std::vector<glm::mat4> output;
        FILE* file = fopen(GetFileName(index, ".txt").c_str(), "r");
        fscanf(file, "%d\n", &count);
        for (int i = 0; i < count; i++) {
            for (int j = 0; j < 4; j++)
                fscanf(file, "%f %f %f %f\n", &mat[j][0], &mat[j][1], &mat[j][2], &mat[j][3]);
            output.push_back(mat);
        }
        fclose(file);
        return output;
    }

    double Dataset::GetPoseTime(int index, Pose pose) {
        int count = 0;
        glm::mat4 mat;
        double timestamp;
        FILE* file = fopen(GetFileName(index, ".txt").c_str(), "r");
        fscanf(file, "%d\n", &count);
        for (int i = 0; i < count; i++) {
            for (int j = 0; j < 4; j++)
                fscanf(file, "%f %f %f %f\n", &mat[j][0], &mat[j][1], &mat[j][2], &mat[j][3]);
        }
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

    void Dataset::WritePose(int index, std::vector<glm::mat4> pose, double imageTimestamp, double depthTimestamp) {
        FILE* file = fopen(GetFileName(index, ".txt").c_str(), "w");
        fprintf(file, "%d\n", (int) pose.size());
        for (int k = 0; k < pose.size(); k++)
            for (int i = 0; i < 4; i++)
                fprintf(file, "%f %f %f %f\n", pose[k][i][0], pose[k][i][1],
                                               pose[k][i][2], pose[k][i][3]);
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
