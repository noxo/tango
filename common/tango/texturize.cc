#include <sstream>
#include "data/image.h"
#include "gl/camera.h"
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
            //if (!IsPoseCorrected(image.timestamp))
              //  continue;
            Image::JPG2YUV(dataset.GetFileName(i, ".jpg"), image.data, width, height);
            Tango3DR_Pose t3dr_image_pose = GetPose(dataset, i, image.timestamp, true);
            if (Tango3DR_updateTexture(context, &image, &t3dr_image_pose) != TANGO_3DR_SUCCESS)
                exit(EXIT_SUCCESS);
        }
        if (trajectory)
        {
            Tango3DR_Trajectory_destroy(*trajectory);
            delete trajectory;
            trajectory = 0;
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

    void TangoTexturize::GenerateTrajectory(std::string tangoDataset) {
        trajectory = new Tango3DR_Trajectory();
        if (tangoDataset.size() > 10) {
            SetEvent("TRAJECTORY");
            FILE* file = fopen((tangoDataset + "/uuid.txt").c_str(), "r");
            TangoUUID uuid;
            fscanf(file, "%s", uuid);
            fclose(file);
            std::string adf = tangoDataset + "/" + uuid;
            instanceTexturize = this;
            Tango3DR_Status ret;
            Tango3DR_AreaDescription area_description;
            ret = Tango3DR_AreaDescription_loadFromAdf(adf.c_str(), &area_description);
            if (ret != TANGO_3DR_SUCCESS)
                exit(EXIT_SUCCESS);
            ret = Tango3DR_Trajectory_createFromAreaDescription(area_description, trajectory);
            if (ret != TANGO_3DR_SUCCESS)
                exit(EXIT_SUCCESS);

            double start = 0, end = 0;
            if (Tango3DR_Trajectory_getEarliestTime(*trajectory, &start) == TANGO_3DR_SUCCESS)
                if (Tango3DR_Trajectory_getLatestTime(*trajectory, &end) == TANGO_3DR_SUCCESS)
                    LOGI("Found trajectory %.2lf-%.2lf", start, end);

            ret = Tango3DR_AreaDescription_destroy(area_description);
            if (ret != TANGO_3DR_SUCCESS)
                exit(EXIT_SUCCESS);
        }
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

    Tango3DR_Pose TangoTexturize::GetPose(Dataset dataset, int index, double timestamp, bool camera) {
        Tango3DR_Pose output;
        if (trajectory)
        {
            if (Tango3DR_getPoseAtTime(*trajectory, timestamp, &output) == TANGO_3DR_SUCCESS)
            {
                GLCamera pose;
                pose.position.x = (float)output.translation[0];
                pose.position.y = (float)output.translation[1];
                pose.position.z = (float)output.translation[2];
                pose.rotation.x = (float)output.orientation[0];
                pose.rotation.y = (float)output.orientation[1];
                pose.rotation.z = (float)output.orientation[2];
                pose.rotation.w = (float)output.orientation[3];
                glm::mat4 matrix = pose.GetTransformation();
                glm::mat4 convert = {
                        1, 0, 0, 0,
                        0, 0, -1, 0,
                        0, 1, 0, 0,
                        0, 0, 0, 1
                };
                if (camera)
                    return TangoService::Extract3DRPose(matrix * convert);
                else
                    return TangoService::Extract3DRPose(matrix);
            }
        }
        return TangoService::Extract3DRPose(dataset.GetPose(index)[camera ? COLOR_CAMERA : DEPTH_CAMERA]);
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

    bool TangoTexturize::IsPoseCorrected(double timestamp) {
        Tango3DR_Pose output;
        if (trajectory)
        {
            if (Tango3DR_getPoseAtTime(*trajectory, timestamp, &output) == TANGO_3DR_SUCCESS)
            {
                return true;
            }
        }
        return false;
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

    void TangoTexturize::CreateContext(bool finalize, Tango3DR_Mesh* mesh, Tango3DR_CameraCalibration* camera) {
        event = "SIMPLIFY";
        Tango3DR_Config textureConfig = Tango3DR_Config_create(TANGO_3DR_CONFIG_TEXTURING);
        Tango3DR_Status ret;
        ret = Tango3DR_Config_setDouble(textureConfig, "min_resolution", finalize ? 0.005 : 0.01);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        ret = Tango3DR_Config_setInt32(textureConfig, "max_num_textures", 4);
        if (ret != TANGO_3DR_SUCCESS)
            exit(EXIT_SUCCESS);
        ret = Tango3DR_Config_setInt32(textureConfig, "mesh_simplification_factor", 1);
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
