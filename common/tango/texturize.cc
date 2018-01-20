#include <sstream>
#include "data/image.h"
#include "gl/camera.h"
#include "tango/service.h"
#include "tango/texturize.h"

namespace oc {

    TangoTexturize::TangoTexturize() : poses(0) {}

    void TangoTexturize::Add(Tango3DR_ImageBuffer t3dr_image, std::vector<glm::mat4> matrix, std::string dataset) {
        if (poses == 0)
            UpdatePoses(dataset);

        //save frame
        width = t3dr_image.width;
        height = t3dr_image.height;
        Image::YUV2JPG(t3dr_image.data, t3dr_image.width, t3dr_image.height, GetFileName(poses, dataset, ".jpg"), false);

        //save transform
        FILE* file = fopen(GetFileName(poses, dataset, ".txt").c_str(), "w");
        fprintf(file, "%d\n", (int) matrix.size());
        for (int k = 0; k < matrix.size(); k++)
            for (int i = 0; i < 4; i++)
                fprintf(file, "%f %f %f %f\n", matrix[k][i][0], matrix[k][i][1],
                                               matrix[k][i][2], matrix[k][i][3]);
        fprintf(file, "%lf\n", t3dr_image.timestamp);
        fclose(file);
        poses++;

        file = fopen(GetFileName(-1, dataset, ".txt").c_str(), "w");
        fprintf(file, "%d %d %d\n", poses, width, height);
        fclose(file);
    }

    void TangoTexturize::ApplyFrames(std::string dataset) {
        UpdatePoses(dataset);
        Tango3DR_ImageBuffer image;
        image.width = (uint32_t) width;
        image.height = (uint32_t) height;
        image.stride = (uint32_t) width;
        image.format = TANGO_3DR_HAL_PIXEL_FORMAT_YCrCb_420_SP;
        image.data = new unsigned char[width * height * 3];

        for (unsigned int i = 0; i < poses; i++) {
            std::ostringstream ss;
            ss << "IMAGE ";
            ss << i + 1;
            ss << "/";
            ss << poses;
            event = ss.str();

            int count;
            glm::mat4 mat;
            double timestamp;
            std::vector<glm::mat4> output;
            FILE* file = fopen(GetFileName(i, dataset, ".txt").c_str(), "r");
            fscanf(file, "%d\n", &count);
            for (int k = 0; k < count; k++) {
                for (int j = 0; j < 4; j++)
                    fscanf(file, "%f %f %f %f\n", &mat[j][0], &mat[j][1], &mat[j][2], &mat[j][3]);
                output.push_back(mat);
            }
            fscanf(file, "%lf\n", &timestamp);
            fclose(file);

            image.timestamp = timestamp;
            Image::JPG2YUV(GetFileName(i, dataset, ".jpg"), image.data, width, height);
            Tango3DR_Pose t3dr_image_pose = TangoService::Extract3DRPose(output[COLOR_CAMERA]);
            Tango3DR_Status ret;
            ret = Tango3DR_updateTexture(context, &image, &t3dr_image_pose);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
        }
        delete[] image.data;
    }

    void TangoTexturize::Clear(std::string dataset) {
        poses = 0;
        FILE* file = fopen(GetFileName(-1, dataset, ".txt").c_str(), "w");
        fprintf(file, "%d %d %d\n", poses, width, height);
        fclose(file);
    }

    Image* TangoTexturize::GetLatestImage(std::string dataset) {
        UpdatePoses(dataset);
        return new Image(GetFileName(poses - 1, dataset, ".jpg"));
    }

    std::vector<glm::mat4> TangoTexturize::GetLatestPose(std::string dataset) {
        UpdatePoses(dataset);
        int count = 0;
        glm::mat4 mat;
        std::vector<glm::mat4> output;
        FILE* file = fopen(GetFileName(poses - 1, dataset, ".txt").c_str(), "r");
        fscanf(file, "%d\n", &count);
        for (int i = 0; i < count; i++) {
            for (int j = 0; j < 4; j++)
                fscanf(file, "%f %f %f %f\n", &mat[j][0], &mat[j][1], &mat[j][2], &mat[j][3]);
            output.push_back(mat);
        }
        fclose(file);
        return output;
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

    std::string TangoTexturize::GetFileName(int index, std::string dataset, std::string extension) {
        std::ostringstream ss;
        ss << index;
        std::string number = ss.str();
        while(number.size() < 8)
            number = "0" + number;
        return dataset + "/" + number + extension;
    }

    void TangoTexturize::UpdatePoses(std::string dataset) {
        FILE* file = fopen(GetFileName(-1, dataset, ".txt").c_str(), "r");
        if (file) {
            fscanf(file, "%d %d %d\n", &poses, &width, &height);
            fclose(file);
        }
    }
}
