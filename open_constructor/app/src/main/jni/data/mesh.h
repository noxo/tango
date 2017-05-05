#ifndef DATA_MESH_H
#define DATA_MESH_H

#include <mutex>
#ifndef NOTANGO
#include <tango_3d_reconstruction_api.h>
#endif
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

    struct SingleDynamicMesh {
#ifndef NOTANGO
        Tango3DR_Mesh tango_mesh;
#endif
        Mesh mesh;
        std::mutex mutex;
        unsigned long size;
    };
}
#endif
