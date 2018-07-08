#ifndef TANGO_SERVICE_H
#define TANGO_SERVICE_H

#include <tango_3d_reconstruction_api.h>
#include <tango_client_api.h>
#include <tango_support_api.h>
#include <vector>
#include "data/dataset.h"
#include "gl/opengl.h"

//#define GENERATE_FLOORPLAN

namespace oc {
    enum Pose { COLOR_CAMERA, DEPTH_CAMERA, OPENGL_CAMERA, MAX_CAMERA };

    class TangoService {
    public:
        TangoService();
        ~TangoService();
        void ApplyTransform();
        void Clear();
        void Connect(void* app);
        void Disconnect();
        void SavePointCloud(std::string filename);
        void SetupConfig(std::string datapath);
        void Setup3DR(double res, double dmin, double dmax, int noise, bool clearing);
        void SetupTransform(std::vector<glm::mat4> area, std::vector<glm::mat4> zero);

        static void DecomposeMatrix(const glm::mat4& matrix, glm::vec3* translation, glm::quat* rotation, glm::vec3* scale);
        static Tango3DR_Pose Extract3DRPose(const glm::mat4 &mat);

        std::vector<glm::mat4> Convert(std::vector<TangoMatrixTransformData> m);
        oc::Dataset Dataset() { return *dataset; }
        Tango3DR_CameraCalibration* Camera() { return &camera; }
        Tango3DR_CameraCalibration* Depth() { return &depth; }
        Tango3DR_ReconstructionContext Context() { return context; }
        TangoSupportPointCloudManager* Pointcloud() { return pointcloud; }
        std::vector<TangoMatrixTransformData> Pose(double timestamp, bool land);
        std::vector<glm::mat4> ZeroPose();

    private:
        oc::Dataset* dataset;
        TangoConfig config;
        Tango3DR_CameraCalibration camera;
        Tango3DR_CameraCalibration depth;
        Tango3DR_ReconstructionContext context;
        TangoSupportPointCloudManager* pointcloud;
        std::vector<glm::mat4> toArea, toAreaTemp;
        std::vector<glm::mat4> toZero, toZeroTemp;

        bool clearing_;
        double res_;
        double dmin_;
        double dmax_;
        int noise_;
    };
}
#endif
