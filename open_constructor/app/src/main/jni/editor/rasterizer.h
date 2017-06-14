#ifndef EDITOR_RASTERIZER_H
#define EDITOR_RASTERIZER_H

#include "gl/opengl.h"

namespace oc {

class Rasterizer {
public:
    void AddUVS(std::vector<glm::vec2> uvs, std::vector<unsigned int> selected);
    void AddVertices(std::vector<glm::vec3>& vertices, glm::mat4 world2screen, bool culling);
    void SetResolution(int w, int h);

    virtual void Process(unsigned long& index, int &x1, int &x2, int &y, double &z1, double &z2) = 0;
private:
    bool Line(int x1, int y1, int x2, int y2, double z1, double z2, std::pair<int, double>* fillCache);
    bool Test(double p, double q, double &t1, double &t2);
    void Triangle(unsigned long& index, glm::vec3 &a, glm::vec3 &b, glm::vec3 &c);

    std::vector<std::pair<int, double> > fillCache1, fillCache2;

protected:
    int viewport_width, viewport_height;
};
}

#endif
