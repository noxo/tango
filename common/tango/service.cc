#include <cstdlib>
#include <iterator>
#include "tango/service.h"

namespace oc {

    TangoService::TangoService() : toArea(ZeroPose()), toAreaTemp(ZeroPose()),
                                   toZero(ZeroPose()), toZeroTemp(ZeroPose()) {}

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

    void TangoService::ApplyTransform() {
        toArea = toAreaTemp;
        toZero = toZeroTemp;
    }

    void TangoService::Clear() {
        Tango3DR_clear(context);
        Tango3DR_ReconstructionContext_destroy(context);
        Setup3DR(res_, dmin_, dmax_, noise_);
    }

    void TangoService::Connect(void* app) {
        TangoErrorType err = TangoService_connect(app, config);
        if (err != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);

        // Initialize TangoSupport context.
        TangoSupport_initialize(TangoService_getPoseAtTime, TangoService_getCameraIntrinsics);

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

        // Update the depth intrinsics too.
        err = TangoService_getCameraIntrinsics(TANGO_CAMERA_DEPTH, &intrinsics);
        if (err != TANGO_SUCCESS)
            std::exit(EXIT_SUCCESS);
        depth.calibration_type = static_cast<Tango3DR_TangoCalibrationType>(intrinsics.calibration_type);
        depth.width = intrinsics.width;
        depth.height = intrinsics.height;
        depth.fx = intrinsics.fx;
        depth.fy = intrinsics.fy;
        depth.cx = intrinsics.cx;
        depth.cy = intrinsics.cy;
        std::copy(std::begin(intrinsics.distortion), std::end(intrinsics.distortion),
                  std::begin(depth.distortion));
    }

    void TangoService::Disconnect() {
        TangoConfig_free(config);
        config = nullptr;
        TangoService_disconnect();
    }

    void TangoService::DecomposeMatrix(const glm::mat4& matrix, glm::vec3* translation,
                                       glm::quat* rotation, glm::vec3* scale) {
        translation->x = matrix[3][0];
        translation->y = matrix[3][1];
        translation->z = matrix[3][2];
        *rotation = glm::quat_cast(matrix);
        scale->x = glm::length(glm::vec3(matrix[0][0], matrix[1][0], matrix[2][0]));
        scale->y = glm::length(glm::vec3(matrix[0][1], matrix[1][1], matrix[2][1]));
        scale->z = glm::length(glm::vec3(matrix[0][2], matrix[1][2], matrix[2][2]));
        if (glm::determinant(matrix) < 0.0)
            scale->x = -scale->x;
    }

    Tango3DR_Pose TangoService::Extract3DRPose(const glm::mat4 &mat) {
        Tango3DR_Pose pose;
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
        DecomposeMatrix(mat, &translation, &rotation, &scale);
        pose.translation[0] = translation[0];
        pose.translation[1] = translation[1];
        pose.translation[2] = translation[2];
        pose.orientation[0] = rotation[0];
        pose.orientation[1] = rotation[1];
        pose.orientation[2] = rotation[2];
        pose.orientation[3] = rotation[3];
        return pose;
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
        Tango3DR_Config_setInt32(t3dr_config, "update_method", TANGO_3DR_PROJECTIVE_UPDATE);

        context = Tango3DR_ReconstructionContext_create(t3dr_config);
        if (context == nullptr)
            std::exit(EXIT_SUCCESS);
        Tango3DR_Config_destroy(t3dr_config);

        Tango3DR_ReconstructionContext_setColorCalibration(context, &camera);
        Tango3DR_ReconstructionContext_setDepthCalibration(context, &depth);

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
        ret = TangoConfig_setBool(config, "config_enable_drift_correction", true);
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

        if (pointcloud == nullptr) {
            int32_t max_point_cloud_elements;
            ret = TangoConfig_getInt32(config, "max_point_cloud_elements", &max_point_cloud_elements);
            if (ret != TANGO_SUCCESS)
                std::exit(EXIT_SUCCESS);

            ret = TangoSupport_createPointCloudManager((size_t) max_point_cloud_elements, &pointcloud);
            if (ret != TANGO_SUCCESS)
                std::exit(EXIT_SUCCESS);
        }
    }

    void TangoService::SetupTransform(std::vector<glm::mat4> area, std::vector<glm::mat4> zero) {
        toAreaTemp = area;
        toZeroTemp = zero;
    }

    std::vector<TangoMatrixTransformData> TangoService::Pose(double timestamp, bool land) {
        //init objects
        std::vector<TangoMatrixTransformData> output;
        TangoMatrixTransformData matrix_transform;

        //get color camera transform
        TangoSupport_getMatrixTransformAtTime(
                timestamp, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION,
                TANGO_COORDINATE_FRAME_CAMERA_COLOR, TANGO_SUPPORT_ENGINE_OPENGL,
                TANGO_SUPPORT_ENGINE_TANGO, ROTATION_0, &matrix_transform);
        output.push_back(matrix_transform);

        //get depth camera transform
        TangoSupport_getMatrixTransformAtTime(
                timestamp, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION,
                TANGO_COORDINATE_FRAME_CAMERA_DEPTH, TANGO_SUPPORT_ENGINE_OPENGL,
                TANGO_SUPPORT_ENGINE_TANGO, ROTATION_0, &matrix_transform);
        output.push_back(matrix_transform);

        //get OpenGL camera transform
        TangoSupport_getMatrixTransformAtTime(
                timestamp, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION, TANGO_COORDINATE_FRAME_DEVICE,
                TANGO_SUPPORT_ENGINE_OPENGL, TANGO_SUPPORT_ENGINE_OPENGL,
                land ? ROTATION_90 : ROTATION_0, &matrix_transform);
        output.push_back(matrix_transform);

        assert(output.size() == MAX_CAMERA);
        return output;
    }

    std::vector<glm::mat4> TangoService::Convert(std::vector<TangoMatrixTransformData> m) {
        std::vector<glm::mat4> output;
        for (int i = 0; i < m.size(); i++)
            output.push_back(toArea[i] * toZero[i] * glm::make_mat4(m[i].matrix));
        return output;
    }

    void TangoService::SavePointCloud(std::string filename) {
        //save complete model to obj
        Tango3DR_Mesh mesh;
        Tango3DR_Status ret = Tango3DR_extractFullMesh(context, &mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = Tango3DR_Mesh_saveToObj(&mesh, filename.c_str());
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = Tango3DR_Mesh_destroy(&mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
    }

    std::vector<glm::mat4> TangoService::ZeroPose() {
        std::vector<glm::mat4> output;
        for (int i = 0; i < MAX_CAMERA; i++)
            output.push_back(glm::mat4(1));
        return output;
    }
}
