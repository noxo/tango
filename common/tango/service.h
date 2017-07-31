#ifndef TANGO_SERVICE_H
#define TANGO_SERVICE_H

#include <tango_3d_reconstruction_api.h>
#include <tango_client_api.h>
#include <tango_support.h>
#include <vector>
#include "gl/opengl.h"

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
        void SetupConfig(std::string datapath);
        void Setup3DR(double res, double dmin, double dmax, int noise);
        void SetupTransform(std::vector<glm::mat4> area, std::vector<glm::mat4> zero);

        static void DecomposeMatrix(const glm::mat4& matrix, glm::vec3* translation, glm::quat* rotation, glm::vec3* scale);
        static Tango3DR_Pose Extract3DRPose(const glm::mat4 &mat);

        std::vector<glm::mat4> Convert(std::vector<TangoSupport_MatrixTransformData> m);
        std::string Dataset() { return dataset; }
        Tango3DR_CameraCalibration* Camera() { return &camera; }
        Tango3DR_ReconstructionContext Context() { return context; }
        TangoSupport_PointCloudManager* Pointcloud() { return pointcloud; }
        std::vector<TangoSupport_MatrixTransformData> Pose(double timestamp, bool land);
        std::vector<glm::mat4> ZeroPose();

    private:
        std::string dataset;
        TangoConfig config;
        Tango3DR_CameraCalibration camera;
        Tango3DR_CameraCalibration depth;
        Tango3DR_ReconstructionContext context;
        TangoSupport_PointCloudManager* pointcloud;
        std::vector<glm::mat4> toArea, toAreaTemp;
        std::vector<glm::mat4> toZero, toZeroTemp;

        double res_;
        double dmin_;
        double dmax_;
        int noise_;
    };
}
#endif
