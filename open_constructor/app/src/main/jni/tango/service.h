#ifndef TANGO_SERVICE_H
#define TANGO_SERVICE_H

#include <tango_3d_reconstruction_api.h>
#include <tango_client_api.h>
#include <tango_support_api.h>

namespace oc {
    class TangoService {
    public:
        ~TangoService();
        void Clear();
        void Connect(void* app);
        void Disconnect();
        void SetupConfig(std::string datapath);
        void Setup3DR(double res, double dmin, double dmax, int noise);

        std::string Dataset() { return dataset; }
        Tango3DR_CameraCalibration Camera() { return camera; }
        Tango3DR_ReconstructionContext Context() { return context; }
        TangoSupportPointCloudManager* Pointcloud() { return pointcloud; }

    private:
        std::string dataset;
        TangoConfig config;
        Tango3DR_CameraCalibration camera;
        Tango3DR_ReconstructionContext context;
        TangoSupportPointCloudManager* pointcloud;

        double res_;
        double dmin_;
        double dmax_;
        int noise_;
    };
}
#endif
