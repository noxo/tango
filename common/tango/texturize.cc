#include <sstream>
#include "data/image.h"
#include "gl/camera.h"
#include "tango/scan.h"
#include "tango/service.h"
#include "tango/texturize.h"

oc::TangoTexturize* instanceTexturize;
void callbackTexturize(int progress, void*)
{
    instanceTexturize->Callback(progress);
}

namespace oc {

    TangoTexturize::TangoTexturize() : poses(0) {}

    void TangoTexturize::Add(Tango3DR_ImageBuffer t3dr_image, std::vector<glm::mat4> matrix, Dataset dataset, double depthTimestamp) {
        if (poses == 0)
            dataset.GetState(poses, width, height);

        //save frame
        width = t3dr_image.width;
        height = t3dr_image.height;
        Image::YUV2JPG(t3dr_image.data, width, height, dataset.GetFileName(poses, ".jpg"), false);

        //save transform
        dataset.WritePose(poses, matrix, t3dr_image.timestamp, depthTimestamp);
        dataset.WriteState(++poses, width, height);
    }

    void TangoTexturize::ApplyFrames(Dataset dataset) {
        dataset.GetState(poses, width, height);
        Tango3DR_ImageBuffer image;
        image.width = (uint32_t) width;
        image.height = (uint32_t) height;
        image.stride = (uint32_t) width;
        image.format = TANGO_3DR_HAL_PIXEL_FORMAT_YCrCb_420_SP;
        image.data = new unsigned char[width * height * 3];

        for (int i = 0; i < poses; i++) {
            std::ostringstream ss;
            ss << "IMAGE ";
            ss << 100 * i / poses;
            ss << "%";
            event = ss.str();

            image.timestamp = dataset.GetPoseTime(i, COLOR_CAMERA);
            Image::JPG2YUV(dataset.GetFileName(i, ".jpg"), image.data, width, height);
            Tango3DR_Pose t3dr_image_pose = TangoScan::LoadPose(dataset, i, COLOR_CAMERA);
            if (Tango3DR_updateTexture(context, &image, &t3dr_image_pose) != TANGO_3DR_SUCCESS)
                exit(EXIT_SUCCESS);
        }

        delete[] image.data;
    }

    void TangoTexturize::Callback(int progress) {
        std::ostringstream ss;
        ss << "TRAJECTORY ";
        ss << progress;
        ss << "%";
        SetEvent(ss.str());
    }

    void TangoTexturize::Clear(Dataset dataset) {
        poses = 0;
        dataset.WriteState(poses, width, height);
    }

    Tango3DR_Trajectory TangoTexturize::GetTrajectory(Dataset dataset) {
        SetEvent("TRAJECTORY");
        instanceTexturize = this;
        Tango3DR_Status ret;

        //stop dataset recording
        TangoConfig config = TangoService_getConfig(TANGO_CONFIG_RUNTIME);
        TangoConfig_setInt32(config, "config_runtime_recording_control", TANGO_RUNTIME_RECORDING_STOP);
        TangoService_setRuntimeConfig(config);

        //get area description
        TangoUUID uuid;
        std::string tangoDataset = dataset.GetPath() + "/";
        TangoService_Experimental_getCurrentDatasetUUID(&uuid);
        tangoDataset += uuid;
        Tango3DR_AreaDescription area_description;
        std::string loop_closure = dataset.GetPath() + "/config";
        ret = Tango3DR_AreaDescription_createFromDataset(tangoDataset.c_str(), loop_closure.c_str(),
                                                         &area_description, callbackTexturize, 0);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);

        //get trajectory
        Tango3DR_Trajectory trajectory;
        ret = Tango3DR_Trajectory_createFromAreaDescription(area_description, &trajectory);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        TangoService_Experimental_deleteDataset(uuid);
        return trajectory;
    }

    bool TangoTexturize::Init(std::string filename, Tango3DR_CameraCalibration* camera) {
        event = "MERGE";
        Tango3DR_Mesh mesh;
        Tango3DR_Status ret;
        ret = Tango3DR_Mesh_loadFromObj(filename.c_str(), &mesh);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);

        //prevent crash on saving empty model
        if (mesh.num_faces == 0) {
            ret = Tango3DR_Mesh_destroy(&mesh);
            if (ret != TANGO_3DR_SUCCESS)
                exit(EXIT_SUCCESS);
            event = "";
            return false;
        }

        //create texturing context
        CreateContext(true, &mesh, camera);
        ret = Tango3DR_Mesh_destroy(&mesh);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        return true;
    }

    bool TangoTexturize::Init(Tango3DR_ReconstructionContext context, Tango3DR_CameraCalibration* camera) {
        event = "PROCESS";
        Tango3DR_Mesh mesh;
        Tango3DR_Status ret;
        ret = Tango3DR_extractFullMesh(context, &mesh);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);

        //prevent crash on saving empty model
        if (mesh.num_faces == 0) {
            ret = Tango3DR_Mesh_destroy(&mesh);
            if (ret != TANGO_3DR_SUCCESS)
                exit(EXIT_SUCCESS);
            event = "";
            return false;
        }

        //create texturing context
        CreateContext(false, &mesh, camera);
        ret = Tango3DR_Mesh_destroy(&mesh);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        return true;
    }

    void TangoTexturize::Process(std::string filename) {
        //texturize mesh
        event = "UNWRAP";
        Tango3DR_Mesh mesh;
        Tango3DR_Status ret;
        ret = Tango3DR_getTexturedMesh(context, &mesh);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);

        //save
        event = "CONVERT";
        ret = Tango3DR_Mesh_saveToObj(&mesh, filename.c_str());
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);

        //cleanup
        ret = Tango3DR_Mesh_destroy(&mesh);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        ret = Tango3DR_TexturingContext_destroy(context);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        event = "";
    }

    bool TangoTexturize::Test(Tango3DR_ReconstructionContext context, Tango3DR_CameraCalibration* camera) {
        Tango3DR_Mesh mesh;
        Tango3DR_Status ret;
        ret = Tango3DR_extractFullMesh(context, &mesh);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        bool notempty = mesh.num_faces != 0;

        ret = Tango3DR_Mesh_destroy(&mesh);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        return notempty;
    }

    void TangoTexturize::CreateContext(bool finalize, Tango3DR_Mesh* mesh, Tango3DR_CameraCalibration* camera) {
        event = "SIMPLIFY";
        Tango3DR_Config textureConfig = Tango3DR_Config_create(TANGO_3DR_CONFIG_TEXTURING);
        Tango3DR_Status ret;
        ret = Tango3DR_Config_setDouble(textureConfig, "min_resolution", finalize ? 0.005 : 0.01);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        ret = Tango3DR_Config_setInt32(textureConfig, "max_num_textures", 8);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        ret = Tango3DR_Config_setInt32(textureConfig, "mesh_simplification_factor", 1);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        ret = Tango3DR_Config_setInt32(textureConfig, "texture_size", 2048);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        ret = Tango3DR_Config_setInt32(textureConfig, "texturing_backend", TANGO_3DR_CPU_TEXTURING);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);

        context = Tango3DR_TexturingContext_create(textureConfig, mesh);
        if (context == nullptr)
            exit(EXIT_SUCCESS);
        Tango3DR_Config_destroy(textureConfig);

        Tango3DR_TexturingContext_setColorCalibration(context, camera);
    }
}
