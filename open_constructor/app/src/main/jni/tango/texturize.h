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
        bool Init(std::string filename, std::string dataset);
        bool Init(Tango3DR_ReconstructionContext context, std::string dataset);
        void Process(std::string filename);

    private:
        void Init(std::string dataset, bool gl, Tango3DR_Mesh* mesh);
        std::string GetFileName(int index, std::string dataset, std::string extension);

        int poses;
        Tango3DR_TexturingContext context;
    };
}
#endif