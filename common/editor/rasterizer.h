#ifndef EDITOR_RASTERIZER_H
#define EDITOR_RASTERIZER_H

#include "gl/opengl.h"

namespace oc {

class Rasterizer {
public:
    void AddUVS(std::vector<glm::vec2> uvs, std::vector<unsigned int> selected);
    void AddVertices(std::vector<glm::vec3>& vertices, glm::mat4 world2screen, bool culling);
    void AddUVVertices(std::vector<glm::vec3>& vertices, std::vector<glm::vec2>& uvs, glm::mat4 world2screen,
                       float cx, float cy, float fx, float fy);
    void SetResolution(int w, int h);

    virtual void Process(unsigned long& index, int &x1, int &x2, int &y, glm::dvec3 &z1, glm::dvec3 &z2) = 0;
private:
    bool Line(int x1, int y1, int x2, int y2, glm::dvec3 z1, glm::dvec3 z2, std::pair<int, glm::dvec3>* fillCache);
    bool Test(double p, double q, double &t1, double &t2);
    void Triangle(unsigned long& index, glm::vec3 &a, glm::vec3 &b, glm::vec3 &c, glm::vec2 ta = glm::vec2(), glm::vec2 tb = glm::vec2(), glm::vec2 tc = glm::vec2());

    std::vector<std::pair<int, glm::dvec3> > fillCache1, fillCache2;

protected:
    int viewport_width, viewport_height;
};
}

#endif
