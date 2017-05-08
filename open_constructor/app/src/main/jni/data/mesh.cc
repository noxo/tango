#include "data/mesh.h"

std::vector<unsigned int> mesh_textureToDelete;

namespace oc {

    Mesh::Mesh() : image(NULL), imageOwner(true), texture(-1) {}

    void Mesh::Destroy() {
        if (image && imageOwner) {
            delete image;
            mesh_textureToDelete.push_back(texture);
            texture = -1;
            image = 0;
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
