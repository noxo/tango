#ifndef EDITOR_RASTERIZER_H
#define EDITOR_RASTERIZER_H

#include "gl/opengl.h"

namespace oc {

class Rasterizer {
public:
    void AddUVS(std::vector<glm::vec2> uvs);
    void AddVertices(std::vector<glm::vec3> vertices);
    void SetResolution(int w, int h);

    virtual void ProcessFragment(int &x, int &y, double &z) = 0;
    virtual void ProcessVertex(int &x1, int &x2, int &y, double &z1, double &z2) = 0;
    virtual bool VerticesOnly() = 0;
private:
    bool Line(int x1, int y1, int x2, int y2, double z1, double z2, std::pair<int, double>* fillCache);
    bool Test(double p, double q, double &t1, double &t2);
    void Triangle(glm::vec3 &a, glm::vec3 &b, glm::vec3 &c);

    std::vector<std::pair<int, double> > fillCache1, fillCache2;
    int viewport_width, viewport_height;
};
}

#endif
