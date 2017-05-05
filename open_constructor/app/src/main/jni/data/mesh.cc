#include "data/mesh.h"

std::vector<unsigned int> mesh_textureToDelete;

namespace oc {

    Mesh::Mesh() : image(NULL), imageOwner(true), texture(-1) {}

    Mesh::~Mesh() {
        std::vector<glm::vec3>().swap(vertices);
        std::vector<glm::vec3>().swap(normals);
        std::vector<glm::vec2>().swap(uv);
        std::vector<unsigned int>().swap(colors);
        std::vector<unsigned int>().swap(indices);
    }

    void Mesh::Destroy() {
        if (image)
            if (imageOwner) {
                delete image;
                mesh_textureToDelete.push_back(texture);
                texture = -1;
            }
    }

    std::vector<unsigned int> Mesh::texturesToDelete() {
        std::vector<unsigned int> output;
        for (unsigned int i : mesh_textureToDelete)
            output.push_back(i);
        mesh_textureToDelete.clear();
        return output;
    }
}
