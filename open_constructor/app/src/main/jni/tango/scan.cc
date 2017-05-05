#include "tango/scan.h"

namespace oc {

    const int kInitialVertexCount = 30;
    const int kInitialIndexCount = 99;
    const int kGrowthFactor = 2;

    bool GridIndex::operator==(const GridIndex &o) const {
        return indices[0] == o.indices[0] && indices[1] == o.indices[1] && indices[2] == o.indices[2];
    }

    void TangoScan::Clear() {
        meshes.clear();
    }

    std::vector<SingleDynamicMesh*> TangoScan::MeshUpdate(Tango3DR_ReconstructionContext context,
                                                          Tango3DR_GridIndexArray *t3dr_updated) {
        std::vector<SingleDynamicMesh*> output;
        for (unsigned long it = 0; it < t3dr_updated->num_indices; ++it) {
            GridIndex updated_index;
            updated_index.indices[0] = t3dr_updated->indices[it][0];
            updated_index.indices[1] = t3dr_updated->indices[it][1];
            updated_index.indices[2] = t3dr_updated->indices[it][2];

            // Build a dynamic mesh and add it to the scene.
            SingleDynamicMesh* dynamic_mesh = meshes[updated_index];
            if (dynamic_mesh == nullptr) {
                dynamic_mesh = new SingleDynamicMesh();
                dynamic_mesh->mesh.vertices.resize(kInitialVertexCount * 3);
                dynamic_mesh->mesh.colors.resize(kInitialVertexCount * 3);
                dynamic_mesh->mesh.indices.resize(kInitialIndexCount);
                dynamic_mesh->tango_mesh = {
                        /* timestamp */ 0.0,
                        /* num_vertices */ 0u,
                        /* num_faces */ 0u,
                        /* num_textures */ 0u,
                        /* max_num_vertices */ static_cast<uint32_t>(dynamic_mesh->mesh.vertices.capacity()),
                        /* max_num_faces */ static_cast<uint32_t>(dynamic_mesh->mesh.indices.capacity() / 3),
                        /* max_num_textures */ 0u,
                        /* vertices */ reinterpret_cast<Tango3DR_Vector3 *>(dynamic_mesh->mesh.vertices.data()),
                        /* faces */ reinterpret_cast<Tango3DR_Face *>(dynamic_mesh->mesh.indices.data()),
                        /* normals */ nullptr,
                        /* colors */ reinterpret_cast<Tango3DR_Color *>(dynamic_mesh->mesh.colors.data()),
                        /* texture_coords */ nullptr,
                        /* texture_ids */ nullptr,
                        /* textures */ nullptr};
                output.push_back(dynamic_mesh);
            }
            dynamic_mesh->mutex.lock();

            while (true) {
                Tango3DR_Status ret;
                ret = Tango3DR_extractPreallocatedMeshSegment(context, updated_index.indices,
                                                              &dynamic_mesh->tango_mesh);
                if (ret == TANGO_3DR_INSUFFICIENT_SPACE) {
                    unsigned long new_vertex_size = dynamic_mesh->mesh.vertices.capacity() * kGrowthFactor;
                    unsigned long new_index_size = dynamic_mesh->mesh.indices.capacity() * kGrowthFactor;
                    new_index_size -= new_index_size % 3;
                    dynamic_mesh->mesh.vertices.resize(new_vertex_size);
                    dynamic_mesh->mesh.colors.resize(new_vertex_size);
                    dynamic_mesh->mesh.indices.resize(new_index_size);
                    dynamic_mesh->tango_mesh.max_num_vertices = static_cast<uint32_t>(dynamic_mesh->mesh.vertices.capacity());
                    dynamic_mesh->tango_mesh.max_num_faces = static_cast<uint32_t>(dynamic_mesh->mesh.indices.capacity() / 3);
                    dynamic_mesh->tango_mesh.vertices = reinterpret_cast<Tango3DR_Vector3 *>(dynamic_mesh->mesh.vertices.data());
                    dynamic_mesh->tango_mesh.colors = reinterpret_cast<Tango3DR_Color *>(dynamic_mesh->mesh.colors.data());
                    dynamic_mesh->tango_mesh.faces = reinterpret_cast<Tango3DR_Face *>(dynamic_mesh->mesh.indices.data());
                } else
                    break;
            }
            dynamic_mesh->size = dynamic_mesh->tango_mesh.num_faces * 3;
            dynamic_mesh->mesh.texture = -1;
            dynamic_mesh->mutex.unlock();
        }
        return output;
    }
}
