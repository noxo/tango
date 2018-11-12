#ifndef DATA_DEPTHMAP_H
#define DATA_DEPTHMAP_H

#include "data/mesh.h"

namespace oc {
    class Depthmap : public Mesh {
    public:
        Depthmap(Image& jpg, std::vector<glm::vec3>& pointcloud, glm::mat4 sensor2world,
                 glm::mat4 world2uv, float cx, float cy, float fx, float fy,
                 int* map, glm::vec3* vecmap, int mapScale);
        void MakeSurface(int margin, int* map);
        void SmoothSurface(int iterations, int* map, glm::vec3* vecmap, glm::mat4 sensor2world);

    private:
        bool IsSurface(int a, int b, int c);

        int height;
        int stride;
        std::vector<float> depth;
    };
}
#endif
