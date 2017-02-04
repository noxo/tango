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

    void VertexProcessor::collideMesh(SingleDynamicMesh* slave, glm::mat4 to2d, Tango3DR_CameraCalibration calibration) {
        std::vector<glm::vec2> uvs;
        slave->mutex.lock();
        for (unsigned int i = 0; i < slave->mesh.vertices.size(); i++) {
            glm::vec4 v = glm::vec4(slave->mesh.vertices[i].x,
                                    slave->mesh.vertices[i].y,
                                    slave->mesh.vertices[i].z, 1);
            v = to2d * v;
            v /= fabs(v.w * v.z);
            v.x *= calibration.fx / (float)calibration.width;
            v.y *= calibration.fy / (float)calibration.height;
            v.x += calibration.cx / (float)calibration.width;
            v.y += calibration.cy / (float)calibration.height;
            uvs.push_back(glm::vec2(v.x, v.y));
        }
        unsigned int a, b, c, d, e, f, count;
        for (unsigned int i = 0; i < temp_mesh->tango_mesh.num_faces; i++) {
            a = temp_mesh->tango_mesh.faces[i][0];
            b = temp_mesh->tango_mesh.faces[i][1];
            c = temp_mesh->tango_mesh.faces[i][2];
            for (int j = slave->mesh.indices.size() / 3 - 1; j >= 0; j--) {
                d = slave->mesh.indices[j * 3 + 0];
                e = slave->mesh.indices[j * 3 + 1];
                f = slave->mesh.indices[j * 3 + 2];
                count = 0;
                if (intersect(temp_mesh->mesh.uv[a], temp_mesh->mesh.uv[b], uvs[d], uvs[e]))
                    count++;
                else if (intersect(temp_mesh->mesh.uv[a], temp_mesh->mesh.uv[b], uvs[d], uvs[f]))
                    count++;
                else if (intersect(temp_mesh->mesh.uv[a], temp_mesh->mesh.uv[b], uvs[e], uvs[f]))
                    count++;

                if (intersect(temp_mesh->mesh.uv[a], temp_mesh->mesh.uv[c], uvs[d], uvs[e]))
                    count++;
                else if (intersect(temp_mesh->mesh.uv[a], temp_mesh->mesh.uv[c], uvs[d], uvs[f]))
                    count++;
                else if (intersect(temp_mesh->mesh.uv[a], temp_mesh->mesh.uv[c], uvs[e], uvs[f]))
                    count++;

                if (intersect(temp_mesh->mesh.uv[b], temp_mesh->mesh.uv[c], uvs[d], uvs[e]))
                    count++;
                else if (intersect(temp_mesh->mesh.uv[b], temp_mesh->mesh.uv[c], uvs[d], uvs[f]))
                    count++;
                else if (intersect(temp_mesh->mesh.uv[b], temp_mesh->mesh.uv[c], uvs[e], uvs[f]))
                    count++;
                if (count == 3) {
                    slave->mesh.indices.erase(slave->mesh.indices.begin() + j * 3 + 2);
                    slave->mesh.indices.erase(slave->mesh.indices.begin() + j * 3 + 1);
                    slave->mesh.indices.erase(slave->mesh.indices.begin() + j * 3 + 0);
                    break;
                }
            }
        }
        slave->mutex.unlock();
    }

    void VertexProcessor::getMeshWithUV(glm::mat4 world2uv, Tango3DR_CameraCalibration calibration,
                                        SingleDynamicMesh* result) {
        result->mesh.render_mode = GL_TRIANGLES;
        unsigned int offset = result->mesh.vertices.size();
        for (unsigned int i = 0; i < temp_mesh->tango_mesh.num_vertices; i++) {
            glm::vec4 v = glm::vec4(temp_mesh->tango_mesh.vertices[i][0],
                                    temp_mesh->tango_mesh.vertices[i][1],
                                    temp_mesh->tango_mesh.vertices[i][2], 1);
            v = world2uv * v;
            v /= fabs(v.w * v.z);
            v.x *= calibration.fx / (float)calibration.width;
            v.y *= calibration.fy / (float)calibration.height;
            v.x += calibration.cx / (float)calibration.width;
            v.y += calibration.cy / (float)calibration.height;
            result->mesh.uv.push_back(glm::vec2(v.x, v.y));
            result->mesh.vertices.push_back(glm::vec3(temp_mesh->tango_mesh.vertices[i][0],
                                                      temp_mesh->tango_mesh.vertices[i][1],
                                                      temp_mesh->tango_mesh.vertices[i][2]));
            topology.push_back(std::map<unsigned int, bool>());
            temp_mesh->mesh.uv.push_back(glm::vec2(v.x, v.y));
        }
        unsigned int a, b, c;
        for (unsigned int i = 0; i < temp_mesh->tango_mesh.num_faces; i++) {
            a = temp_mesh->tango_mesh.faces[i][0];
            b = temp_mesh->tango_mesh.faces[i][1];
            c = temp_mesh->tango_mesh.faces[i][2];
            result->mesh.indices.push_back(a + offset);
            result->mesh.indices.push_back(b + offset);
            result->mesh.indices.push_back(c + offset);
            topology[a][b] = true; topology[b][a] = true;
            topology[a][c] = true; topology[c][a] = true;
            topology[b][c] = true; topology[c][b] = true;
        }
    }

    bool VertexProcessor::intersect(glm::vec2 const& p0, glm::vec2 const& p1, glm::vec2 const& p2, glm::vec2 const& p3) {
        glm::vec2 const s1 = p1 - p0;
        glm::vec2 const s2 = p3 - p2;
        glm::vec2 const u = p0 - p2;
        float const ip = 1.f / (-s2.x * s1.y + s1.x * s2.y);
        float const s = (-s1.y * u.x + s1.x * u.y) * ip;
        float const t = ( s2.x * u.y - s2.y * u.x) * ip;
        return s >= 0 && s <= 1 && t >= 0 && t <= 1;
    }
}
