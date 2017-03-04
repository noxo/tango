#include "mask_processor.h"
#include <queue>

SingleDynamicMesh* temp_mask_mesh = 0;
std::vector<std::pair<int, double> > fillCache1;
std::vector<std::pair<int, double> > fillCache2;

const int kGrowthFactor = 2;
const int kInitialVertexCount = 30;
const int kInitialIndexCount = 99;

namespace mesh_builder {

    MaskProcessor::MaskProcessor(int w, int h) {
        buffer = new double[w * h];
        draw = true;
        exact = false;
        viewport_width = w;
        viewport_height = h;

        for (int mem = 0; mem < w * h; mem++)
            buffer[mem] = INT_MAX;
        if (fillCache1.empty())
            fillCache1.resize((unsigned long) (h + 1));
        if (fillCache2.empty())
            fillCache2.resize((unsigned long) (h + 1));
    }

    MaskProcessor::MaskProcessor(int w, int h, glm::mat4 matrix, Tango3DR_CameraCalibration calib) {
        buffer = new double[w * h];
        calibration = calib;
        camera = matrix[3] / matrix[3][3];
        draw = true;
        exact = false;
        viewport_width = w;
        viewport_height = h;
        world2uv  = glm::inverse(matrix);

        for (int mem = 0; mem < w * h; mem++)
            buffer[mem] = INT_MAX;
        if (fillCache1.empty())
            fillCache1.resize((unsigned long) (h + 1));
        if (fillCache2.empty())
            fillCache2.resize((unsigned long) (h + 1));
    }

    MaskProcessor::~MaskProcessor() {
        delete[] buffer;
    }

    void MaskProcessor::AddContext(Tango3DR_Context context, Tango3DR_GridIndexArray *indices) {
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
            ret = Tango3DR_extractPreallocatedMesh(context, indices, &temp_mask_mesh->tango_mesh);
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

        std::vector<glm::vec3> vertices;
        unsigned int pos;
        glm::vec4 vertex;
        float z;
        for (unsigned int i = 0; i < temp_mask_mesh->tango_mesh.max_num_faces; i++) {
            for (unsigned int j = 0; j < 3; j++) {
                pos = temp_mask_mesh->tango_mesh.faces[i][j];
                vertex = glm::vec4(temp_mask_mesh->tango_mesh.vertices[pos][0],
                                   temp_mask_mesh->tango_mesh.vertices[pos][1],
                                   temp_mask_mesh->tango_mesh.vertices[pos][2], 1.0f);
                z = glm::length(vertex - camera);
                Math::convert2uv(vertex, world2uv, calibration);
                vertices.push_back(glm::vec3(vertex.x, vertex.y, z));
            }
        }
        Triangles(&vertices[0].x, vertices.size() / 3);
    }

    void MaskProcessor::AddPointClound(TangoPointCloud *front_cloud_, glm::mat4 matrix) {
        glm::vec4 v;
        float z;
        std::queue<glm::vec3> toAdd;
        for (long i = 0; i < front_cloud_->num_points; i++) {
            v.x = front_cloud_->points[i][0];
            v.y = front_cloud_->points[i][1];
            v.z = front_cloud_->points[i][2];
            v.w = front_cloud_->points[i][3];
            v /= v.w;
            z = glm::length(v - camera);
            v = matrix * v;
            Math::convert2uv(v, world2uv, calibration);
            v.x *= (float)(viewport_width - 1);
            v.y *= (float)(viewport_height - 1);
            if ((v.x < 0) ||(v.y < 0) || (v.x >= viewport_width) || (v.y >= viewport_height))
                continue;
            toAdd.push(glm::vec3(v.x, v.y, z - 0.25f));
        }
        int index;
        glm::vec3 p, t;
        while (!toAdd.empty()) {
            p = toAdd.front();
            toAdd.pop();
            index = (int)p.y * viewport_width + (int)p.x;
            if (buffer[index] > INT_MAX - 10000) {
                buffer[index] = p.z;
                for (int i = 1; i <= 4; i++) {
                    t = p;
                    if (i == 1)
                        t.x += 1.0f;
                    else if (i == 2)
                        t.x -= 1.0f;
                    else if (i == 3)
                        t.y += 1.0f;
                    else if (i == 4)
                        t.y -= 1.0f;
                    if ((t.x < 0) ||(t.y < 0) || (t.x >= viewport_width) || (t.y >= viewport_height))
                        continue;
                    toAdd.push(t);
                }
            }
        }
    }

    void MaskProcessor::AddUVs(std::vector<SingleDynamicMesh *> meshes) {
        std::vector<glm::vec3> vertices;
        unsigned int pos;
        for (unsigned int i = 0; i < meshes.size(); i++) {
            meshes[i]->mutex.lock();
            for (unsigned int j = 0; j < meshes[i]->mesh.indices.size() / 3; j++) {
                for (int k = 0; k < 3; k++) {
                    pos = meshes[i]->mesh.indices[j * 3 + k];
                    vertices.push_back(glm::vec3(meshes[i]->mesh.uv[pos], 1.0f));
                }
            }
            meshes[i]->mutex.unlock();
        }
        Triangles(&vertices[0].x, vertices.size() / 3);
    }

    void MaskProcessor::AddVertices(std::vector<SingleDynamicMesh *> meshes) {
        std::vector<glm::vec3> vertices;
        float z;
        unsigned int pos;
        glm::vec4 vertex;
        for (unsigned int i = 0; i < meshes.size(); i++) {
            meshes[i]->mutex.lock();
            for (unsigned int j = 0; j < meshes[i]->mesh.indices.size() / 3; j++) {
                for (int k = 0; k < 3; k++) {
                    pos = meshes[i]->mesh.indices[j * 3 + k];
                    vertex = glm::vec4(meshes[i]->mesh.vertices[pos], 1.0f);
                    z = glm::length(vertex - camera);
                    Math::convert2uv(vertex, world2uv, calibration);
                    vertices.push_back(glm::vec3(vertex.x, vertex.y, z));
                }
            }
            meshes[i]->mutex.unlock();
        }
        Triangles(&vertices[0].x, vertices.size() / 3);
    }

    double MaskProcessor::GetMask(int x, int y, int r, bool minim) {
        double output = minim ? INT_MAX : 0;
        for (int i = -r; i <= r; i++)
            for (int j = -r; j <= r; j++)
                if ((x + i >= 0) && (x + i < viewport_width))
                    if ((y + j >= 0) && (y + j < viewport_height)) {
                        double v = buffer[(y + j) * viewport_width + x + i];
                        if (minim && (output > v))
                            output = v;
                        if (!minim && (output < v))
                            output = v;
                    }
        return output;
    }

    void MaskProcessor::MaskMesh(SingleDynamicMesh* mesh, bool processFront) {
        if (mesh->mesh.indices.empty())
            return;
        std::vector<bool> edgeFace;
        std::vector<glm::vec3> vertices;
        glm::vec4 v;
        float z;
        for (unsigned long i = 0; i < mesh->mesh.vertices.size(); i++) {
            v = glm::vec4(mesh->mesh.vertices[i], 1.0f);
            z = glm::length(v - camera);
            Math::convert2uv(v, world2uv, calibration);
            edgeFace.push_back((v.x < 0) ||(v.y < 0) || (v.x > 1) || (v.y > 1));
            v.x *= (float)(viewport_width - 1);
            v.y *= (float)(viewport_height - 1);
            v.z = z;
            vertices.push_back(glm::vec3(v.x, v.y, v.z));
        }

        draw = false;
        exact = !processFront;
        glm::vec3 a, b, c;
        for (long i = (mesh->mesh.indices.size() - 1) / 3; i >= 0; i--) {
            if (!processFront) {
                if (edgeFace[mesh->mesh.indices[i * 3 + 0]])
                    continue;
                if (edgeFace[mesh->mesh.indices[i * 3 + 1]])
                    continue;
                if (edgeFace[mesh->mesh.indices[i * 3 + 2]])
                    continue;
            }
            a = vertices[mesh->mesh.indices[i * 3 + 0]];
            b = vertices[mesh->mesh.indices[i * 3 + 1]];
            c = vertices[mesh->mesh.indices[i * 3 + 2]];
            int r = Triangle(a, b, c);
            if ((!processFront && (r == 100)) || (processFront && (r < 100))) {
                mesh->mesh.indices.erase(mesh->mesh.indices.begin() + i * 3 + 2);
                mesh->mesh.indices.erase(mesh->mesh.indices.begin() + i * 3 + 1);
                mesh->mesh.indices.erase(mesh->mesh.indices.begin() + i * 3 + 0);
                if (mesh->mesh.indices.empty())
                    break;
            }
        }
        mesh->size = mesh->mesh.indices.size();
        draw = true;
        exact = false;
    }

    bool MaskProcessor::Line(int x1, int y1, int x2, int y2, double z1, double z2,
                             std::pair<int, double>* fillCache) {

        //Liang & Barsky clipping (only top-bottom)
        int h = y2 - y1;
        double t1 = 0, t2 = 1;
        if (Test(-h, y1, t1, t2) && Test(h, viewport_height - 1 - y1, t1, t2) ) {
            double z;
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
                x2 = (int) (x1 + t2 * w);
                y2 = (int) (y1 + t2 * h);
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
            z = (z2 - z1) / (double)w;

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

    bool MaskProcessor::Test(double p, double q, double &t1, double &t2) {
        //negative cutting
        if (p < 0) {
            double t = q/p;

            //cut nothing
            if (t > t2)
                return false;
                //cut the first coordinate
            else if (t > t1)
                t1 = t;

            //positive cutting
        } else if (p > 0) {
            double t = q/p;

            //cut nothing
            if (t < t1)
                return false;
                //cut the second coordinate
            else if (t < t2)
                t2 = t;

            //line is right to left(or bottom to top)
        } else if (q < 0)
            return false;
        return true;
    }

    int MaskProcessor::Triangle(glm::vec3 &a, glm::vec3 &b, glm::vec3 &c) {

        //create markers for filling
        int min, max;
        int ab = (int) glm::abs(a.y - b.y);
        int ac = (int) glm::abs(a.y - c.y);
        int bc = (int) glm::abs(b.y - c.y);
        if ((ab >= ac) && (ab >= bc)) {
            Line((int) a.x, (int) a.y, (int) b.x, (int) b.y, a.z, b.z, &fillCache1[0]);
            Line((int) a.x, (int) a.y, (int) c.x, (int) c.y, a.z, c.z, &fillCache2[0]);
            Line((int) b.x, (int) b.y, (int) c.x, (int) c.y, b.z, c.z, &fillCache2[0]);
            min = glm::max(0, (int) glm::min(a.y, b.y));
            max = glm::min((int) glm::max(a.y, b.y), viewport_height - 1);
        } else if ((ac >= ab) && (ac >= bc)) {
            Line((int) a.x, (int) a.y, (int) c.x, (int) c.y, a.z, c.z, &fillCache1[0]);
            Line((int) a.x, (int) a.y, (int) b.x, (int) b.y, a.z, b.z, &fillCache2[0]);
            Line((int) b.x, (int) b.y, (int) c.x, (int) c.y, b.z, c.z, &fillCache2[0]);
            min = glm::max(0, (int) glm::min(a.y, c.y));
            max = glm::min((int) glm::max(a.y, c.y), viewport_height - 1);
        }else {
            Line((int) b.x, (int) b.y, (int) c.x, (int) c.y, b.z, c.z, &fillCache1[0]);
            Line((int) a.x, (int) a.y, (int) b.x, (int) b.y, a.z, b.z, &fillCache2[0]);
            Line((int) a.x, (int) a.y, (int) c.x, (int) c.y, a.z, c.z, &fillCache2[0]);
            min = glm::max(0, (int) glm::min(b.y, c.y));
            max = glm::min((int) glm::max(b.y, c.y), viewport_height - 1);
        }

        //fill triangle
        int count = 0;
        int passed = 0;
        double tolerancy = draw ? 0 : 0.25;
        int memy = min * viewport_width;
        for (int y = min; y <= max; y++) {
            int x1 = fillCache1[y].first;
            int x2 = fillCache2[y].first;
            double z1 = fillCache1[y].second;
            double z2 = fillCache2[y].second;

            //Liang & Barsky clipping
            double t1 = 0;
            double t2 = 1;
            int x = x2 - x1;
            if (Test(-x, x1, t1, t2) && Test(x, viewport_width - 1 - x1, t1, t2)) {

                //clip line
                double z = z2 - z1;
                if (t1 > 0) {
                    x1 += t1 * x;
                    z1 += t1 * z;
                } else
                    t1 = 0;
                if (t2 < 1) {
                    t2 -= t1;
                    x2 = (int) (x1 + t2 * x);
                    z2 = z1 + t2 * z;
                }

                //filling algorithm
                x = glm::abs(x2 - x1);
                int step = (x2 >= x1) ? 1 : -1;
                z = (z2 - z1) / (float)x;
                int mem = x1 + memy;
                for (; x >= 0; x--) {
                    if (exact) {
                        if ((z1 > 0) && (fabs(buffer[mem] - z1) <= tolerancy)) {
                            if (draw)
                                buffer[mem] = z1;
                            passed++;
                        }
                    } else {
                        if ((z1 > 0) && (buffer[mem] >= z1 - tolerancy)) {
                            if (draw)
                                buffer[mem] = z1;
                            passed++;
                        }
                    }
                    count++;
                    mem += step;
                    z1 += z;
                }
            }
            memy += viewport_width;
        }
        if (count == 0)
            return 0;
        return (100 * passed) / count;
    }

    void MaskProcessor::Triangles(float* vertices, unsigned long size) {
        glm::vec3 a, b, c;
        int v = 0;
        for (unsigned long i = 0; i < size; i++, v += 9) {
            //get vertices
            a = glm::vec3(vertices[v + 0], vertices[v + 1], vertices[v + 2]);
            b = glm::vec3(vertices[v + 3], vertices[v + 4], vertices[v + 5]);
            c = glm::vec3(vertices[v + 6], vertices[v + 7], vertices[v + 8]);
            //scale into mask dimensions
            a.x *= (float)(viewport_width - 1);
            a.y *= (float)(viewport_height - 1);
            b.x *= (float)(viewport_width - 1);
            b.y *= (float)(viewport_height - 1);
            c.x *= (float)(viewport_width - 1);
            c.y *= (float)(viewport_height - 1);
            //process
            Triangle(a, b, c);
        }
    }
}
