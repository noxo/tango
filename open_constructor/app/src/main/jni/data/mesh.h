#ifndef DATA_MESH_H
#define DATA_MESH_H

#include "data/image.h"
#include "gl/opengl.h"

namespace oc {
    class Mesh {
    public:

        Mesh();
        void Destroy();
        static std::vector<unsigned int> texturesToDelete();

        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> colors;
        std::vector<unsigned int> indices;
        std::vector<glm::vec2> uv;
        Image* image;
        bool imageOwner;
        long texture;
    };
}
#endif
