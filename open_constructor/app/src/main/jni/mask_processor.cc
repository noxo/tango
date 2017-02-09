#include "mask_processor.h"

SingleDynamicMesh* temp_mask_mesh = 0;
const int kGrowthFactor = 2;
const int kInitialVertexCount = 30;
const int kInitialIndexCount = 99;

namespace mesh_builder {
    MaskProcessor::MaskProcessor(Tango3DR_Context context, Tango3DR_GridIndex index) {
        if (!temp_mask_mesh) {
            // Build a dynamic mesh and add it to the scene.
            temp_mask_mesh = new SingleDynamicMesh();
            temp_mask_mesh->mesh.render_mode = GL_TRIANGLES;
            temp_mask_mesh->mesh.vertices.resize(kInitialVertexCount * 3);
            temp_mask_mesh->mesh.colors.resize(kInitialVertexCount * 3);
            temp_mask_mesh->mesh.indices.resize(kInitialIndexCount);
            temp_mask_mesh->tango_mesh = {
                    /* timestamp */ 0.0,
                    /* num_vertices */ 0u,
                    /* num_faces */ 0u,
                    /* num_textures */ 0u,
                    /* max_num_vertices */ static_cast<uint32_t>(temp_mask_mesh->mesh.vertices.capacity()),
                    /* max_num_faces */ static_cast<uint32_t>(temp_mask_mesh->mesh.indices.capacity() / 3),
                    /* max_num_textures */ 0u,
                    /* vertices */ reinterpret_cast<Tango3DR_Vector3 *>(temp_mask_mesh->mesh.vertices.data()),
                    /* faces */ reinterpret_cast<Tango3DR_Face *>(temp_mask_mesh->mesh.indices.data()),
                    /* normals */ nullptr,
                    /* colors */ reinterpret_cast<Tango3DR_Color *>(temp_mask_mesh->mesh.colors.data()),
                    /* texture_coords */ nullptr,
                    /* texture_ids */ nullptr,
                    /* textures */ nullptr};
        }

        Tango3DR_Status ret;
        while (true) {
            ret = Tango3DR_extractPreallocatedMeshSegment(context, index, &temp_mask_mesh->tango_mesh);
            if (ret == TANGO_3DR_INSUFFICIENT_SPACE) {
                unsigned long new_vertex_size = temp_mask_mesh->mesh.vertices.capacity() * kGrowthFactor;
                unsigned long new_index_size = temp_mask_mesh->mesh.indices.capacity() * kGrowthFactor;
                new_index_size -= new_index_size % 3;
                temp_mask_mesh->mesh.vertices.resize(new_vertex_size);
                temp_mask_mesh->mesh.colors.resize(new_vertex_size);
                temp_mask_mesh->mesh.indices.resize(new_index_size);
                temp_mask_mesh->tango_mesh.max_num_vertices = static_cast<uint32_t>(temp_mask_mesh->mesh.vertices.capacity());
                temp_mask_mesh->tango_mesh.max_num_faces = static_cast<uint32_t>(temp_mask_mesh->mesh.indices.capacity() / 3);
                temp_mask_mesh->tango_mesh.vertices = reinterpret_cast<Tango3DR_Vector3 *>(temp_mask_mesh->mesh.vertices.data());
                temp_mask_mesh->tango_mesh.colors = reinterpret_cast<Tango3DR_Color *>(temp_mask_mesh->mesh.colors.data());
                temp_mask_mesh->tango_mesh.faces = reinterpret_cast<Tango3DR_Face *>(temp_mask_mesh->mesh.indices.data());
            } else
                break;
        }
    }

    MaskProcessor::~MaskProcessor() {

    }
}
