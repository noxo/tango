#ifndef GL_MESH_H
#define GL_MESH_H

#include "gl/opengl.h"
#include "rgb_image.h"

namespace oc {
    class GLMesh {
    public:

        GLMesh();
        void Destroy();
        static std::vector<unsigned int> texturesToDelete();

        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> colors;
        std::vector<unsigned int> indices;
        std::vector<glm::vec2> uv;
        RGBImage* image;
        bool imageOwner;
        long texture;
    };
}
#endif
