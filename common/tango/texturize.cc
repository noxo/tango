#include <sstream>
#include "data/image.h"
#include "gl/camera.h"
#include "tango/service.h"
#include "tango/texturize.h"

oc::TangoTexturize* instance;
void callback(int progress, void*)
{
    instance->Callback(progress);
}

namespace oc {

    TangoTexturize::TangoTexturize() : poses(0) {}

    void TangoTexturize::Add(Tango3DR_ImageBuffer t3dr_image, std::vector<glm::mat4> matrix, Dataset dataset) {
        if (poses == 0)
            dataset.GetState(poses, width, height);

        //save frame
        width = t3dr_image.width;
        height = t3dr_image.height;
        Image::YUV2JPG(t3dr_image.data, t3dr_image.width, t3dr_image.height, dataset.GetFileName(poses, ".jpg"), false);

        //save transform
        dataset.WritePose(poses, matrix, t3dr_image.timestamp);
        dataset.WriteState(++poses, width, height);
    }

    void TangoTexturize::ApplyFrames(Dataset dataset, std::string tangoDataset) {
        dataset.GetState(poses, width, height);
        Tango3DR_ImageBuffer image;
        image.width = (uint32_t) width;
        image.height = (uint32_t) height;
        image.stride = (uint32_t) width;
        image.format = TANGO_3DR_HAL_PIXEL_FORMAT_YCrCb_420_SP;
        image.data = new unsigned char[width * height * 3];

        Tango3DR_Trajectory trajectory = nullptr;
        if (tangoDataset.size() > 10) {
            SetEvent("TRAJECTORY");
            instance = this;
            Tango3DR_Status ret;
            Tango3DR_AreaDescription area_description;
            std::string loop_closure = tangoDataset + "/../config";
            ret = Tango3DR_AreaDescription_createFromDataset(tangoDataset.c_str(), loop_closure.c_str(), &area_description, callback, 0);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            ret = Tango3DR_Trajectory_createFromAreaDescription(area_description, &trajectory);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
        }

        for (unsigned int i = 0; i < poses; i++) {
            std::ostringstream ss;
            ss << "IMAGE ";
            ss << i + 1;
            ss << "/";
            ss << poses;
            event = ss.str();

            double timestamp = dataset.GetPoseTime(i);
            std::vector<glm::mat4> output = dataset.GetPose(i);

            Tango3DR_Status ret;
            image.timestamp = timestamp;
            Image::JPG2YUV(dataset.GetFileName(i, ".jpg"), image.data, width, height);
            Tango3DR_Pose t3dr_image_pose = TangoService::Extract3DRPose(output[COLOR_CAMERA]);
            if (tangoDataset.size() > 10) {
                //get fixed pose
                ret = Tango3DR_getPoseAtTime(trajectory, timestamp, &t3dr_image_pose);
                if (ret == TANGO_3DR_SUCCESS)
                {
                    GLCamera pose;
                    pose.position.x = (float)t3dr_image_pose.translation[0];
                    pose.position.y = (float)t3dr_image_pose.translation[1];
                    pose.position.z = (float)t3dr_image_pose.translation[2];
                    pose.rotation.x = (float)t3dr_image_pose.orientation[0];
                    pose.rotation.y = (float)t3dr_image_pose.orientation[1];
                    pose.rotation.z = (float)t3dr_image_pose.orientation[2];
                    pose.rotation.w = (float)t3dr_image_pose.orientation[3];
                    glm::mat4 matrix = pose.GetTransformation();
                    glm::mat4 convert = {
                            1, 0, 0, 0,
                            0, 0, -1, 0,
                            0, 1, 0, 0,
                            0, 0, 0, 1
                    };
                    t3dr_image_pose = TangoService::Extract3DRPose(matrix * convert);
                }
            }
            ret = Tango3DR_updateTexture(context, &image, &t3dr_image_pose);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
        }
        if (tangoDataset.size() > 10)
            Tango3DR_Trajectory_destroy(trajectory);
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

    Image* TangoTexturize::GetLatestImage(Dataset dataset) {
        dataset.GetState(poses, width, height);
        return new Image(dataset.GetFileName(poses - 1, ".jpg"));
    }

    int TangoTexturize::GetLatestIndex(Dataset dataset) {
        dataset.GetState(poses, width, height);
        return poses - 1;
    }

    std::vector<glm::mat4> TangoTexturize::GetLatestPose(Dataset dataset) {
        dataset.GetState(poses, width, height);
        return dataset.GetPose(poses - 1);
    }

    bool TangoTexturize::Init(std::string filename, Tango3DR_CameraCalibration* camera) {
        event = "MERGE";
        Tango3DR_Mesh mesh;
        Tango3DR_Status ret;
        ret = Tango3DR_Mesh_loadFromObj(filename.c_str(), &mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        //prevent crash on saving empty model
        if (mesh.num_faces == 0) {
            ret = Tango3DR_Mesh_destroy(&mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            event = "";
            return false;
        }

        //create texturing context
        CreateContext(true, &mesh, camera);
        ret = Tango3DR_Mesh_destroy(&mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        return true;
    }

    bool TangoTexturize::Init(Tango3DR_ReconstructionContext context, Tango3DR_CameraCalibration* camera) {
        event = "PROCESS";
        Tango3DR_Mesh mesh;
        Tango3DR_Status ret;
        ret = Tango3DR_extractFullMesh(context, &mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        //prevent crash on saving empty model
        if (mesh.num_faces == 0) {
            ret = Tango3DR_Mesh_destroy(&mesh);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            event = "";
            return false;
        }

        //create texturing context
        CreateContext(false, &mesh, camera);
        ret = Tango3DR_Mesh_destroy(&mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        return true;
    }

    void TangoTexturize::Process(std::string filename) {
        //texturize mesh
        event = "UNWRAP";
        Tango3DR_Mesh mesh;
        Tango3DR_Status ret;
        ret = Tango3DR_getTexturedMesh(context, &mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        //save
        event = "CONVERT";
        ret = Tango3DR_Mesh_saveToObj(&mesh, filename.c_str());
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        //cleanup
        ret = Tango3DR_Mesh_destroy(&mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = Tango3DR_TexturingContext_destroy(context);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        event = "";
    }

    void TangoTexturize::CreateContext(bool finalize, Tango3DR_Mesh* mesh, Tango3DR_CameraCalibration* camera) {
        event = "SIMPLIFY";
        Tango3DR_Config textureConfig = Tango3DR_Config_create(TANGO_3DR_CONFIG_TEXTURING);
        Tango3DR_Status ret;
        ret = Tango3DR_Config_setDouble(textureConfig, "min_resolution", finalize ? 0.005 : 0.01);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = Tango3DR_Config_setInt32(textureConfig, "max_num_textures", 4);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = Tango3DR_Config_setInt32(textureConfig, "mesh_simplification_factor", 1);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = Tango3DR_Config_setInt32(textureConfig, "texturing_backend", TANGO_3DR_CPU_TEXTURING);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        context = Tango3DR_TexturingContext_create(textureConfig, mesh);
        if (context == nullptr)
            std::exit(EXIT_SUCCESS);
        Tango3DR_Config_destroy(textureConfig);

        Tango3DR_TexturingContext_setColorCalibration(context, camera);
    }
}
