#include <sstream>
#include "data/image.h"
#include "gl/camera.h"
#include "tango/texturize.h"

#define PNG_TEXTURE_SCALE 4

namespace oc {

    TangoTexturize::TangoTexturize() : poses(0) {}

    void TangoTexturize::Add(Tango3DR_ImageBuffer t3dr_image, glm::mat4 image_matrix, std::string dataset) {
        Image frame(t3dr_image, PNG_TEXTURE_SCALE);
        frame.Write(GetFileName(poses, dataset, ".png").c_str());
        FILE* file = fopen(GetFileName(poses, dataset, ".txt").c_str(), "w");
        for (int i = 0; i < 4; i++)
            fprintf(file, "%f %f %f %f\n", image_matrix[i][0], image_matrix[i][1],
                                           image_matrix[i][2], image_matrix[i][3]);
        fprintf(file, "%lf\n", t3dr_image.timestamp);
        fclose(file);
        poses++;
    }

    void TangoTexturize::ApplyFrames(std::string dataset) {
        for (unsigned int i = 0; i < poses; i++) {
            glm::mat4 mat;
            double timestamp;
            FILE* file = fopen(GetFileName(i, dataset, ".txt").c_str(), "r");
            for (int i = 0; i < 4; i++)
                fscanf(file, "%f %f %f %f\n", &mat[i][0], &mat[i][1], &mat[i][2], &mat[i][3]);
            fscanf(file, "%lf\n", &timestamp);
            fclose(file);

            Image frame(GetFileName(i, dataset, ".png"));
            Tango3DR_ImageBuffer image;
            image.width = frame.GetWidth() * PNG_TEXTURE_SCALE;
            image.height = frame.GetHeight() * PNG_TEXTURE_SCALE;
            image.stride = frame.GetWidth() * PNG_TEXTURE_SCALE;
            image.timestamp = timestamp;
            image.format = TANGO_3DR_HAL_PIXEL_FORMAT_YCrCb_420_SP;
            image.data = frame.ExtractYUV(PNG_TEXTURE_SCALE);
            Tango3DR_Pose t3dr_image_pose = GLCamera::Extract3DRPose(mat);
            Tango3DR_Status ret;
            ret = Tango3DR_updateTexture(context, &image, &t3dr_image_pose);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            delete[] image.data;
        }
    }

    void TangoTexturize::Clear() {
        poses = 0;
    }

    bool TangoTexturize::Init(std::string filename, std::string dataset) {
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
            return false;
        }

        //create texturing context
        CreateContext(dataset, true, &mesh);
        ret = Tango3DR_Mesh_destroy(&mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        return true;
    }

    bool TangoTexturize::Init(Tango3DR_ReconstructionContext context, std::string dataset) {
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
            return false;
        }

        //create texturing context
        CreateContext(dataset, false, &mesh);
        ret = Tango3DR_Mesh_destroy(&mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        return true;
    }

    void TangoTexturize::Process(std::string filename) {
        //texturize mesh
        Tango3DR_Mesh mesh;
        Tango3DR_Status ret;
        ret = Tango3DR_getTexturedMesh(context, &mesh);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        //save
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
    }

    std::string TangoTexturize::GetFileName(int index, std::string dataset, std::string extension) {
        std::ostringstream ss;
        ss << dataset.c_str();
        ss << "/";
        ss << index;
        ss << extension.c_str();
        return ss.str();
    }

    void TangoTexturize::CreateContext(std::string dataset, bool gl, Tango3DR_Mesh* mesh) {
        Tango3DR_Config textureConfig = Tango3DR_Config_create(TANGO_3DR_CONFIG_TEXTURING);
        Tango3DR_Status ret;
        ret = Tango3DR_Config_setDouble(textureConfig, "min_resolution", 0.01);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        ret = Tango3DR_Config_setInt32(textureConfig, "mesh_simplification_factor", 100);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);
        int backend = gl ? TANGO_3DR_GL_TEXTURING : TANGO_3DR_CPU_TEXTURING;
        ret = Tango3DR_Config_setInt32(textureConfig, "texturing_backend", backend);
        if (ret != TANGO_3DR_SUCCESS)
            std::exit(EXIT_SUCCESS);

        context = Tango3DR_TexturingContext_create(textureConfig, dataset.c_str(), mesh);
        if (context == nullptr)
            std::exit(EXIT_SUCCESS);
        Tango3DR_Config_destroy(textureConfig);
    }
}
