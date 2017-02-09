#include "mask_processor.h"
#include "math_utils.h"

SingleDynamicMesh* temp_mask_mesh = 0;
std::vector<std::pair<int, glm::vec3> > fillCache1;
std::vector<std::pair<int, glm::vec3> > fillCache2;

const int kGrowthFactor = 2;
const int kInitialVertexCount = 30;
const int kInitialIndexCount = 99;

namespace mesh_builder {
    MaskProcessor::MaskProcessor(Tango3DR_Context context, Tango3DR_GridIndex index, int w, int h,
                                 glm::mat4 matrix, Tango3DR_CameraCalibration calib) {
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

        buffer = new glm::vec3[w * h];
        calibration = calib;
        viewport_width = w;
        viewport_height = h;
        world2uv = matrix;

        if (fillCache1.empty())
            fillCache1.resize((unsigned long) (h + 1));
        if (fillCache2.empty())
            fillCache2.resize((unsigned long) (h + 1));

        std::vector<glm::vec3> vertices;
        unsigned int pos;
        glm::vec3 vertex;
        for (unsigned int i = 0; i < temp_mask_mesh->tango_mesh.max_num_faces; i++) {
            for (unsigned int j = 0; j < 3; j++) {
                pos = temp_mask_mesh->tango_mesh.faces[i][j];
                vertex = glm::vec3(temp_mask_mesh->tango_mesh.vertices[pos][0],
                                   temp_mask_mesh->tango_mesh.vertices[pos][1],
                                   temp_mask_mesh->tango_mesh.vertices[pos][2]);
                vertices.push_back(vertex);
            }
        }
        triangles(&vertices[0].x, vertices.size() / 3);
    }

    MaskProcessor::~MaskProcessor() {
        delete[] buffer;
    }

    void MaskProcessor::maskMesh(SingleDynamicMesh* mesh, bool inverse) {
        if (mesh->mesh.indices.empty())
            return;
        std::vector<bool> valid;
        glm::vec3 d;
        glm::vec4 v;
        int x, y;
        for (unsigned long i = 0; i < mesh->mesh.vertices.size(); i++) {
            v = glm::vec4(mesh->mesh.vertices[i], 1.0f);
            Math::convert2uv(v, world2uv, calibration);
            x = (int) (v.x * viewport_width);
            y = (int) (v.y * viewport_height);
            if ((x < 0) || (y < 0) || (x >= viewport_width) || (y >= viewport_height))
                valid.push_back(false);
            else {
                d = glm::abs(buffer[x + y * viewport_width] - mesh->mesh.vertices[i]);
                valid.push_back(d.x + d.y + d.z < 0.01);
            }
        }
        bool ok;
        for (long i = (mesh->mesh.indices.size() - 1) / 3; i >= 0; i--) {
            ok = !inverse;
            if (valid[mesh->mesh.indices[i * 3 + 0]])
            if (valid[mesh->mesh.indices[i * 3 + 1]])
            if (valid[mesh->mesh.indices[i * 3 + 2]])
                ok = !ok;
            if (!ok) {
                mesh->mesh.indices.erase(mesh->mesh.indices.begin() + i * 3 + 2);
                mesh->mesh.indices.erase(mesh->mesh.indices.begin() + i * 3 + 1);
                mesh->mesh.indices.erase(mesh->mesh.indices.begin() + i * 3 + 0);
                if (mesh->mesh.indices.empty())
                    break;
            }
        }
        mesh->size = mesh->mesh.indices.size();
    }

    bool MaskProcessor::line(int x1, int y1, int x2, int y2, glm::vec3 z1, glm::vec3 z2,
                             std::pair<int, glm::vec3>* fillCache) {

        //Liang & Barsky clipping (only top-bottom)
        int h = y2 - y1;
        float t1 = 0, t2 = 1;
        if (test(-h, y1, t1, t2) && test(h, viewport_height - 1 - y1, t1, t2) ) {
            glm::vec3 z;
            int c0, c1, xp0, xp1, yp0, yp1, y, p, w;
            bool wp, hp;

            //clip line
            if (t1 > 0) {
                w = x2 - x1;
                z = z2 - z1;
                x1 += t1 * w;
                y1 += t1 * h;
                z1 += t1 * z;
            } else
                t1 = 0;
            if (t2 < 1) {
                w = x2 - x1;
                z = z2 - z1;
                t2 -= t1;
                x2 = x1 + t2 * w;
                y2 = y1 + t2 * h;
                z2 = z1 + t2 * z;
            }

            //count new line dimensions
            wp = x2 >= x1;
            w = wp ? x2 - x1 : x1 - x2;
            hp = y2 >= y1;
            h = hp ? y2 - y1 : y1 - y2;

            //line in x axis nearby
            if (w > h) {
                //direction from left to right
                xp0 = wp ? 1 : -1;
                yp0 = 0;

                //direction from top to bottom
                xp1 = wp ? 1 : -1;
                yp1 = hp ? 1 : -1;

                //line in y axis nearby
            } else {
                //direction from top to bottom
                xp0 = 0;
                yp0 = hp ? 1 : -1;

                //direction from left to right
                xp1 = wp ? 1 : -1;
                yp1 = hp ? 1 : -1;

                //apply line length
                y = w;
                w = h;
                h = y;
            }

            //count z coordinate step
            z = (z2 - z1) / (float)w;

            //Bresenham's algorithm
            c0 = h + h;
            p = c0 - w;
            c1 = p - w;
            y = y1;
            fillCache[y].first = x1;
            fillCache[y].second = z1;
            for (w--; w >= 0; w--) {

                //interpolate
                if (p < 0) {
                    p += c0;
                    x1 += xp0;
                    y1 += yp0;
                } else {
                    p += c1;
                    x1 += xp1;
                    y1 += yp1;
                }
                z1 += z;

                //write cache info
                if (wp || (y != y1)) {
                    y = y1;
                    fillCache[y].first = x1;
                    fillCache[y].second = z1;
                }
            }
            return true;
        }
        return false;
    }

    bool MaskProcessor::test(double p, double q, float &t1, float &t2) {
        //negative cutting
        if (p < 0) {
            double t = q/p;

            //cut nothing
            if (t > t2)
                return false;
                //cut the first coordinate
            else if (t > t1)
                t1 = (float) t;

            //positive cutting
        } else if (p > 0) {
            double t = q/p;

            //cut nothing
            if (t < t1)
                return false;
                //cut the second coordinate
            else if (t < t2)
                t2 = (float) t;

            //line is right to left(or bottom to top)
        } else if (q < 0)
            return false;
        return true;
    }

    void MaskProcessor::triangles(float* vertices, unsigned long size) {
        int ab, ac, bc, step, x, x1, x2, y, min, mem, memy, max;
        float t1, t2;
        glm::vec4 a, b, c;
        glm::vec3 az, bz, cz, z, z1, z2;
        int v = 0;
        for (unsigned long i = 0; i < size; i++, v += 9) {
            //get vertices
            a = glm::vec4(vertices[v + 0], vertices[v + 1], vertices[v + 2], 1.0f);
            b = glm::vec4(vertices[v + 3], vertices[v + 4], vertices[v + 5], 1.0f);
            c = glm::vec4(vertices[v + 6], vertices[v + 7], vertices[v + 8], 1.0f);
            //transfer into mask coordinates from 0 to 1
            Math::convert2uv(a, world2uv, calibration);
            Math::convert2uv(b, world2uv, calibration);
            Math::convert2uv(c, world2uv, calibration);
            //scale into mask dimensions
            a.x *= (float)viewport_width;
            a.y *= (float)viewport_height;
            b.x *= (float)viewport_width;
            b.y *= (float)viewport_height;
            c.x *= (float)viewport_width;
            c.y *= (float)viewport_height;
            //get coordinates for interpolation
            az = glm::vec3(vertices[v + 0], vertices[v + 1], vertices[v + 2]);
            bz = glm::vec3(vertices[v + 3], vertices[v + 4], vertices[v + 5]);
            cz = glm::vec3(vertices[v + 6], vertices[v + 7], vertices[v + 8]);

            //create markers for filling
            ab = (int) glm::abs(a.y - b.y);
            ac = (int) glm::abs(a.y - c.y);
            bc = (int) glm::abs(b.y - c.y);
            if ((ab >= ac) && (ab >= bc)) {
                line(a.x, a.y, b.x, b.y, az, bz, &fillCache1[0]);
                line(a.x, a.y, c.x, c.y, az, cz, &fillCache2[0]);
                line(b.x, b.y, c.x, c.y, bz, cz, &fillCache2[0]);
                min = glm::max(0, (int) glm::min(a.y, b.y));
                max = glm::min((int) glm::max(a.y, b.y), viewport_height - 1);
            } else if ((ac >= ab) && (ac >= bc)) {
                line(a.x, a.y, c.x, c.y, az, cz, &fillCache1[0]);
                line(a.x, a.y, b.x, b.y, az, bz, &fillCache2[0]);
                line(b.x, b.y, c.x, c.y, bz, cz, &fillCache2[0]);
                min = glm::max(0, (int) glm::min(a.y, c.y));
                max = glm::min((int) glm::max(a.y, c.y), viewport_height - 1);
            }else {
                line(b.x, b.y, c.x, c.y, bz, cz, &fillCache1[0]);
                line(a.x, a.y, b.x, b.y, az, bz, &fillCache2[0]);
                line(a.x, a.y, c.x, c.y, az, cz, &fillCache2[0]);
                min = glm::max(0, (int) glm::min(b.y, c.y));
                max = glm::min((int) glm::max(b.y, c.y), viewport_height - 1);
            }

            //fill triangle
            memy = min * viewport_width;
            for (y = min; y <= max; y++) {
                x1 = fillCache1[y].first;
                x2 = fillCache2[y].first;
                z1 = fillCache1[y].second;
                z2 = fillCache2[y].second;

                //Liang & Barsky clipping
                t1 = 0;
                t2 = 1;
                x = x2 - x1;
                if (test(-x, x1, t1, t2) && test(x, viewport_width - 1 - x1, t1, t2)) {

                    //clip line
                    z = z2 - z1;
                    if (t1 > 0) {
                        x1 += t1 * x;
                        z1 += t1 * z;
                    } else
                        t1 = 0;
                    if (t2 < 1) {
                        t2 -= t1;
                        x2 = x1 + t2 * x;
                        z2 = z1 + t2 * z;
                    }

                    //filling algorithm initialize
                    x = glm::abs(x2 - x1);
                    step = (x2 >= x1) ? 1 : -1;
                    z = (z2 - z1) / (float)x;
                    mem = x1 + memy;
                    for (; x >= 0; x--) {
                        buffer[mem] = z1;

                        //interpolate
                        mem += step;
                        z1 += z;
                    }
                }
                memy += viewport_width;
            }
        }
    }
}
