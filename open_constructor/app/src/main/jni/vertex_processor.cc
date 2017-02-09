#include "math_utils.h"
#include "vertex_processor.h"

SingleDynamicMesh* temp_mesh = 0;
const int kGrowthFactor = 2;
const int kInitialVertexCount = 30;
const int kInitialIndexCount = 99;

namespace mesh_builder {
    VertexProcessor::VertexProcessor(Tango3DR_Context context, Tango3DR_GridIndex index) {
        if (!temp_mesh) {
            // Build a dynamic mesh and add it to the scene.
            temp_mesh = new SingleDynamicMesh();
            temp_mesh->mesh.render_mode = GL_TRIANGLES;
            temp_mesh->mesh.vertices.resize(kInitialVertexCount * 3);
            temp_mesh->mesh.colors.resize(kInitialVertexCount * 3);
            temp_mesh->mesh.indices.resize(kInitialIndexCount);
            temp_mesh->tango_mesh = {
                    /* timestamp */ 0.0,
                    /* num_vertices */ 0u,
                    /* num_faces */ 0u,
                    /* num_textures */ 0u,
                    /* max_num_vertices */ static_cast<uint32_t>(temp_mesh->mesh.vertices.capacity()),
                    /* max_num_faces */ static_cast<uint32_t>(temp_mesh->mesh.indices.capacity() / 3),
                    /* max_num_textures */ 0u,
                    /* vertices */ reinterpret_cast<Tango3DR_Vector3 *>(temp_mesh->mesh.vertices.data()),
                    /* faces */ reinterpret_cast<Tango3DR_Face *>(temp_mesh->mesh.indices.data()),
                    /* normals */ nullptr,
                    /* colors */ reinterpret_cast<Tango3DR_Color *>(temp_mesh->mesh.colors.data()),
                    /* texture_coords */ nullptr,
                    /* texture_ids */ nullptr,
                    /* textures */ nullptr};
        }

        Tango3DR_Status ret;
        while (true) {
            ret = Tango3DR_extractPreallocatedMeshSegment(context, index, &temp_mesh->tango_mesh);
            if (ret == TANGO_3DR_INSUFFICIENT_SPACE) {
                unsigned long new_vertex_size = temp_mesh->mesh.vertices.capacity() * kGrowthFactor;
                unsigned long new_index_size = temp_mesh->mesh.indices.capacity() * kGrowthFactor;
                new_index_size -= new_index_size % 3;
                temp_mesh->mesh.vertices.resize(new_vertex_size);
                temp_mesh->mesh.colors.resize(new_vertex_size);
                temp_mesh->mesh.indices.resize(new_index_size);
                temp_mesh->tango_mesh.max_num_vertices = static_cast<uint32_t>(temp_mesh->mesh.vertices.capacity());
                temp_mesh->tango_mesh.max_num_faces = static_cast<uint32_t>(temp_mesh->mesh.indices.capacity() / 3);
                temp_mesh->tango_mesh.vertices = reinterpret_cast<Tango3DR_Vector3 *>(temp_mesh->mesh.vertices.data());
                temp_mesh->tango_mesh.colors = reinterpret_cast<Tango3DR_Color *>(temp_mesh->mesh.colors.data());
                temp_mesh->tango_mesh.faces = reinterpret_cast<Tango3DR_Face *>(temp_mesh->mesh.indices.data());
            } else
                break;
        }
    }

    VertexProcessor::~VertexProcessor() {

    }

    void VertexProcessor::getMeshWithUV(glm::mat4 world2uv, Tango3DR_CameraCalibration calibration,
                                        SingleDynamicMesh* result) {
        result->mesh.render_mode = GL_TRIANGLES;
        std::vector<bool> valid;
        unsigned int offset = (unsigned int) result->mesh.vertices.size();
        for (unsigned int i = 0; i < temp_mesh->tango_mesh.num_vertices; i++) {
            glm::vec4 v = glm::vec4(temp_mesh->tango_mesh.vertices[i][0],
                                    temp_mesh->tango_mesh.vertices[i][1],
                                    temp_mesh->tango_mesh.vertices[i][2], 1);
            Math::convert2uv(v, world2uv, calibration);
            result->mesh.uv.push_back(glm::vec2(v.x, v.y));
            result->mesh.vertices.push_back(glm::vec3(temp_mesh->tango_mesh.vertices[i][0],
                                                      temp_mesh->tango_mesh.vertices[i][1],
                                                      temp_mesh->tango_mesh.vertices[i][2]));
            temp_mesh->mesh.uv.push_back(glm::vec2(v.x, v.y));
            valid.push_back((v.x > 0) && (v.y > 0) && (v.x < 1) && (v.y < 1));
        }
        unsigned int a, b, c;
        for (unsigned int i = 0; i < temp_mesh->tango_mesh.num_faces; i++) {
            a = temp_mesh->tango_mesh.faces[i][0];
            b = temp_mesh->tango_mesh.faces[i][1];
            c = temp_mesh->tango_mesh.faces[i][2];
            if(valid[a] && valid[b] && valid[c]) {
                result->mesh.indices.push_back(a + offset);
                result->mesh.indices.push_back(b + offset);
                result->mesh.indices.push_back(c + offset);
            }
        }
    }
}
