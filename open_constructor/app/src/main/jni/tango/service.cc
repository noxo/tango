#include <cstdlib>
#include <iterator>
#include "tango/service.h"

namespace oc {

    TangoService::~TangoService() {
        if (config != nullptr) {
            TangoConfig_free(config);
            config = nullptr;
        }
        if (pointcloud != nullptr) {
            TangoSupport_freePointCloudManager(pointcloud);
            pointcloud = nullptr;
        }
        if (context != nullptr) {
            Tango3DR_ReconstructionContext_destroy(context);
            context = nullptr;
        }
    }

    void TangoService::Clear() {
        Tango3DR_ReconstructionContext_destroy(context);
        Setup3DR(res_, dmin_, dmax_, noise_);
    }

    void TangoService::Connect(void* app) {
        TangoErrorType err = TangoService_connect(app, config);
        if (err != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Initialize TangoSupport context.
        TangoSupport_initializeLibrary();

        // Update the camera intrinsics too.
        TangoCameraIntrinsics intrinsics;
        err = TangoService_getCameraIntrinsics(TANGO_CAMERA_COLOR, &intrinsics);
        if (err != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        camera.calibration_type = static_cast<Tango3DR_TangoCalibrationType>(intrinsics.calibration_type);
        camera.width = intrinsics.width;
        camera.height = intrinsics.height;
        camera.fx = intrinsics.fx;
        camera.fy = intrinsics.fy;
        camera.cx = intrinsics.cx;
        camera.cy = intrinsics.cy;
        std::copy(std::begin(intrinsics.distortion), std::end(intrinsics.distortion),
                  std::begin(camera.distortion));
    }

    void TangoService::Disconnect() {
        TangoConfig_free(config);
        config = nullptr;
        TangoService_disconnect();
    }

    void TangoService::Setup3DR(double res, double dmin, double dmax, int noise) {
        Tango3DR_Config t3dr_config = Tango3DR_Config_create(TANGO_3DR_CONFIG_RECONSTRUCTION);
        Tango3DR_Status t3dr_err;
        t3dr_err = Tango3DR_Config_setDouble(t3dr_config, "resolution", res);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setDouble(t3dr_config, "min_depth", dmin);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setDouble(t3dr_config, "max_depth", dmax);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setBool(t3dr_config, "generate_color", true);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        t3dr_err = Tango3DR_Config_setBool(t3dr_config, "use_parallel_integration", true);
        if (t3dr_err != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        Tango3DR_Config_setInt32(t3dr_config, "min_num_vertices", noise);

        context = Tango3DR_ReconstructionContext_create(t3dr_config);
        if (context == nullptr)
            std::exit(EXIT_SUCCESS);
        Tango3DR_Config_destroy(t3dr_config);

        Tango3DR_setColorCalibration(context, &camera);

        res_ = res;
        dmin_ = dmin;
        dmax_ = dmax;
        noise_ = noise;
    }

    void TangoService::SetupConfig(std::string datapath) {
        dataset = datapath;
        config = TangoService_getConfig(TANGO_CONFIG_DEFAULT);
        if (config == nullptr)
            std::exit(EXIT_SUCCESS);

        // Set auto-recovery for motion tracking as requested by the user.
        int ret = TangoConfig_setBool(config, "config_enable_auto_recovery", true);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Enable depth.
        ret = TangoConfig_setBool(config, "config_enable_depth", true);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Disable learning.
        ret = TangoConfig_setBool(config, "config_enable_learning_mode", false);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Enable drift correction.
        ret = TangoConfig_setBool(config, "config_enable_drift_correction", true);//false in Tango Constructor
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Need to specify the depth_mode as XYZC.
        ret = TangoConfig_setInt32(config, "config_depth_mode", TANGO_POINTCLOUD_XYZC);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Enable color camera.
        ret = TangoConfig_setBool(config, "config_enable_color_camera", true);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Set datasets
        ret = TangoConfig_setString(config, "config_datasets_path", dataset.c_str());
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = TangoConfig_setBool(config, "config_enable_dataset_recording", true);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = TangoConfig_setInt32(config, "config_dataset_recording_mode", TANGO_RECORDING_MODE_MOTION_TRACKING);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = TangoConfig_setBool(config, "config_smooth_pose", false);
        if (ret != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        if (pointcloud == nullptr) {
            int32_t max_point_cloud_elements;
            ret = TangoConfig_getInt32(config, "max_point_cloud_elements",
                                       &max_point_cloud_elements);
            if (ret != TANGO_SUCCESS)
                std::exit(EXIT_SUCCESS);

            ret = TangoSupport_createPointCloudManager((size_t) max_point_cloud_elements, &pointcloud);
            if (ret != TANGO_SUCCESS)
                std::exit(EXIT_SUCCESS);
        }
    }
}
