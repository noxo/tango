#ifndef TANGO_TEXTURIZE_H
#define TANGO_TEXTURIZE_H

#include <tango_3d_reconstruction_api.h>
#include "gl/opengl.h"

namespace oc {

    class TangoTexturize {
    public:
        TangoTexturize();
        void Add(Tango3DR_ImageBuffer t3dr_image, glm::mat4 image_matrix, std::string dataset);
        void ApplyFrames(std::string dataset);
        void Clear();
        bool Init(std::string filename, Tango3DR_CameraCalibration* camera);
        bool Init(Tango3DR_ReconstructionContext context, Tango3DR_CameraCalibration* camera);
        std::string GetEvent() { return event; }
        void Process(std::string filename);
        void SetResolution(float value) { resolution = value; }

    private:
        void CreateContext(bool gl, Tango3DR_Mesh* mesh, Tango3DR_CameraCalibration* camera);
        std::string GetFileName(int index, std::string dataset, std::string extension);

        int poses;
        float resolution;
        std::string event;
        Tango3DR_TexturingContext context;
        int width, height;
    };
}
#endif
