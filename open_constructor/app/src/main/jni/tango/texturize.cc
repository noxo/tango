#include <sstream>
#include "data/image.h"
#include "gl/camera.h"
#include "tango/texturize.h"

namespace oc {

    TangoTexturize::TangoTexturize() : poses(0) {}

    void TangoTexturize::Add(Tango3DR_ImageBuffer t3dr_image, glm::mat4 image_matrix, std::string dataset) {
        //save frame
        width = t3dr_image.width;
        height = t3dr_image.height;
        Image::YUV2JPG(t3dr_image.data, t3dr_image.width, t3dr_image.height, GetFileName(poses, dataset, ".jpg"));

        //save transform
        FILE* file = fopen(GetFileName(poses, dataset, ".txt").c_str(), "w");
        for (int i = 0; i < 4; i++)
            fprintf(file, "%f %f %f %f\n", image_matrix[i][0], image_matrix[i][1],
                                           image_matrix[i][2], image_matrix[i][3]);
        fprintf(file, "%lf\n", t3dr_image.timestamp);
        fclose(file);
        poses++;
    }

    void TangoTexturize::ApplyFrames(std::string dataset) {
        Tango3DR_ImageBuffer image;
        image.width = (uint32_t) width;
        image.height = (uint32_t) height;
        image.stride = (uint32_t) width;
        image.format = TANGO_3DR_HAL_PIXEL_FORMAT_YCrCb_420_SP;
        image.data = new unsigned char[width * height * 3];

        for (unsigned int i = 0; i < poses; i++) {
            std::ostringstream ss;
            ss << "Processing image ";
            ss << i + 1;
            ss << "/";
            ss << poses;
            event = ss.str();

            glm::mat4 mat;
            double timestamp;
            FILE* file = fopen(GetFileName(i, dataset, ".txt").c_str(), "r");
            for (int j = 0; j < 4; j++)
                fscanf(file, "%f %f %f %f\n", &mat[j][0], &mat[j][1], &mat[j][2], &mat[j][3]);
            fscanf(file, "%lf\n", &timestamp);
            fclose(file);

            image.timestamp = timestamp;
            Image::JPG2YUV(GetFileName(i, dataset, ".jpg"), image.data, width, height);
            Tango3DR_Pose t3dr_image_pose = GLCamera::Extract3DRPose(mat);
            Tango3DR_Status ret;
            ret = Tango3DR_updateTexture(context, &image, &t3dr_image_pose);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
        }
        delete[] image.data;
    }

    void TangoTexturize::Clear() {
        poses = 0;
    }

    bool TangoTexturize::Init(std::string filename, Tango3DR_CameraCalibration* camera) {
        event = "Merging results";
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
        event = "Processing model";
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
        event = "Unwrapping model";
        Tango3DR_Mesh mesh;
        Tango3DR_Status ret;
        ret = Tango3DR_getTexturedMesh(context, &mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        //save
        event = "Converting data";
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

    void TangoTexturize::CreateContext(bool gl, Tango3DR_Mesh* mesh, Tango3DR_CameraCalibration* camera) {
        event = "Simplifying mesh";
        Tango3DR_Config textureConfig = Tango3DR_Config_create(TANGO_3DR_CONFIG_TEXTURING);
        Tango3DR_Status ret;
        ret = Tango3DR_Config_setDouble(textureConfig, "min_resolution", gl ? 0.002 : 0.01);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        int simplify = (mesh->max_num_faces < 1000) ? 1 : (gl ? 1 : 50);
        if (resolution < 0.006)
            simplify = 1;
        ret = Tango3DR_Config_setInt32(textureConfig, "mesh_simplification_factor", simplify);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        int backend = gl ? TANGO_3DR_GL_TEXTURING : TANGO_3DR_CPU_TEXTURING;
        ret = Tango3DR_Config_setInt32(textureConfig, "texturing_backend", backend);
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
        ss << dataset.c_str();
        ss << "/";
        ss << index;
        ss << extension.c_str();
        return ss.str();
    }
}
