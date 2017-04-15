#ifndef GL_MESH_H
#define GL_MESH_H

#include "utils/math.h"

namespace oc {
    class GLMesh {

    public:
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> colors;
        std::vector<unsigned int> indices;
        std::vector<glm::vec2> uv;
        unsigned int texture;
    };
}
#endif
