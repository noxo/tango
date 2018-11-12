#include "data/depthmap.h"
#include "data/file3d.h"

namespace oc {

    Depthmap::Depthmap(Image& jpg, std::vector<glm::vec3>& pointcloud, glm::mat4 sensor2world,
                       glm::mat4 world2uv, float cx, float cy, float fx, float fy,
                       int* map, glm::vec3* vecmap, int mapScale) {

        stride = jpg.GetWidth() / mapScale;
        height = jpg.GetHeight() / mapScale;
        for (int i = 0; i < stride * height; i++) {
            map[i] = -1;
            vecmap[i] = glm::vec3(-1);
        }
        for (glm::vec3& v : pointcloud) {
            glm::vec4 w = sensor2world * glm::vec4(v, 1.0f);
            w /= glm::abs(w.w);
            glm::vec4 t = world2uv * glm::vec4(w.x, w.y, w.z, 1.0f);
            t.x /= glm::abs(t.z * t.w);
            t.y /= glm::abs(t.z * t.w);
            t.x *= fx / (float)jpg.GetWidth();
            t.y *= fy / (float)jpg.GetHeight();
            t.x += cx / (float)jpg.GetWidth();
            t.y += cy / (float)jpg.GetHeight();
            int x = (int)(t.x * jpg.GetWidth());
            int y = (int)(t.y * jpg.GetHeight());
            int index = (y * jpg.GetWidth() + x) * 4;
            if ((x >= 0) && (x < jpg.GetWidth()) && (y >= 0) && (y < jpg.GetHeight())) {
                glm::ivec3 color;
                color.r = jpg.GetData()[index + 0];
                color.g = jpg.GetData()[index + 1];
                color.b = jpg.GetData()[index + 2];
                colors.push_back(oc::File3d::CodeColor(color));
                int index = ((int)(y / mapScale)) * stride + (int)(x / mapScale);
                map[index] = vertices.size();
                vecmap[index] = v;

                depth.push_back(v.z);
                vertices.push_back(glm::vec3(w.x, w.y, w.z));
            }
        }
    }

    void Depthmap::MakeSurface(int margin, int* map) {
        for (int x = 1 + margin; x < stride - margin; x++) {
            for (int y = 1 + margin; y < height - margin; y++) {
                int a = map[(y - 1) * stride + x - 1];
                int b = map[(y - 1) * stride + x];
                int c = map[y * stride + x - 1];
                int d = map[y * stride + x];
                if (IsSurface(a, b, c)) {
                    indices.push_back(c);
                    indices.push_back(b);
                    indices.push_back(a);
                }
                if (IsSurface(b, c, d)) {
                    indices.push_back(b);
                    indices.push_back(c);
                    indices.push_back(d);
                }
            }
        }
    }

    void Depthmap::SmoothSurface(int iterations, int* map, glm::vec3* vecmap, glm::mat4 sensor2world) {

        for (int it = 0; it < iterations; it++) {
            //create distance map
            float dstmap[stride * height];
            for (int i = 0; i < stride * height; i++)
                dstmap[i] = -1;
            for (int x = 1; x < stride - 1; x++) {
                for (int y = 1; y < height - 1; y++) {
                    if (map[y * stride + x] < 0)
                        continue;

                    //count average and add value into distance map
                    int count = 0;
                    float distance = 0;
                    for (int i = x; i <= x + 1; i++) {
                        for (int j = y; j <= y + 1; j++) {
                            int a = map[(j - 1) * stride + i - 1];
                            int b = map[(j - 1) * stride + i];
                            int c = map[j * stride + i - 1];
                            int d = map[j * stride + i];
                            if (IsSurface(a, b, c) && IsSurface(b, c, d)) {
                                for (int k = i - 1; k <= i; k++) {
                                    for (int l = j - 1; l <= j; l++) {
                                        if (map[l * stride + k] >= 0) {
                                            distance += vecmap[l * stride + k].z;
                                            count++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (count > 0) {
                        dstmap[y * stride + x] = distance / (float)count;
                    }
                }
            }

            //apply distance map to vector map
            for (int i = 0; i < stride * height; i++)
                if (dstmap[i] >= 0)
                    vecmap[i].z = dstmap[i];
        }

        //apply vector map to geometry
        for (int x = 0; x < stride; x++) {
            for (int y = 0; y < height; y++) {
                int index = map[y * stride + x];
                if (index >= 0) {
                    glm::vec3 v = vecmap[y * stride + x];
                    glm::vec4 w = sensor2world * glm::vec4(v, 1.0f);
                    w /= glm::abs(w.w);
                    vertices[index] = glm::vec3(w.x, w.y, w.z);
                }
            }
        }
    }

    bool Depthmap::IsSurface(int a, int b, int c) {
        float aspect = 0.075f;
        if ((a >= 0) && (b >= 0) && (c >= 0) && (a != b) && (b != c) && (c != a)) {
            float avrg = aspect * (depth[c] + depth[b] + depth[a]) / 3.0f;
            float len1 = fabs(depth[a] - depth[b]);
            float len2 = fabs(depth[a] - depth[c]);
            float len3 = fabs(depth[b] - depth[c]);
            if ((len1 < avrg) && (len2 < avrg) && (len3 < avrg)) {
                return true;
            }
        }
        return false;
    }
}
