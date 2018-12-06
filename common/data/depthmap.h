#ifndef DATA_DEPTHMAP_H
#define DATA_DEPTHMAP_H

#include "data/mesh.h"

namespace oc {
    struct Rect {
        int a;
        int b;
        int c;
        int d;
    };

    class Depthmap : public Mesh {
    public:
        Depthmap(Image& jpg, std::vector<glm::vec3>& pointcloud, glm::mat4 sensor2world,
                 glm::mat4 world2uv, float cx, float cy, float fx, float fy, int mapScale);
        int GetHeight() { return height; }
        int GetWidth() { return stride; }
        bool Join(int x1, int y1, int x2, int y2);
        void MakeSurface(int margin);
        void SmoothSurface(int iterations);

    private:
        bool IsSurface(int a, int b, int c);

        int lastMargin;
        int height;
        int stride;
        glm::mat4 matrix;
        std::vector<float> depth;
        std::vector<int> map;
        std::vector<Rect> rects;
        std::vector<glm::vec3> vecmap;
    };
}
#endif
