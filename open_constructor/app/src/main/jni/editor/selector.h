#ifndef EDITOR_SELECTOR_H
#define EDITOR_SELECTOR_H

#include "data/mesh.h"
#include "editor/rasterizer.h"

namespace oc {

class Selector : Rasterizer {
public:
    void ApplySelection(std::vector<Mesh>& mesh, glm::mat4 world2screen, float x, float y);
    void DecreaseSelection(std::vector<Mesh>& mesh);
    void IncreaseSelection(std::vector<Mesh>& mesh);
    void Init(int w, int h);

    virtual void ProcessFragment(unsigned long& index, int &x, int &y, double &z);
    virtual void ProcessVertex(unsigned long& index, int &x1, int &x2, int &y, double &z1, double &z2);
    virtual bool VerticesOnly();
private:
    std::string VertexToKey(glm::vec3& vec);

    double depth;
    int pointX, pointY;
    int selected;
};
}

#endif
