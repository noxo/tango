#ifndef EDITOR_SELECTOR_H
#define EDITOR_SELECTOR_H

#include "data/mesh.h"
#include "editor/rasterizer.h"

namespace oc {

class Selector : Rasterizer {
public:
    void CompleteSelection(std::vector<Mesh>& mesh, bool inverse);
    void DecreaseSelection(std::vector<Mesh>& mesh);
    void IncreaseSelection(std::vector<Mesh>& mesh);
    void Init(int w, int h);
    void SelectObject(std::vector<Mesh>& mesh, glm::mat4 world2screen, float x, float y);
    void SelectTriangle(std::vector<Mesh>& mesh, glm::mat4 world2screen, float x, float y);

    virtual void Process(unsigned long& index, int &x1, int &x2, int &y, double &z1, double &z2);
private:
    std::string VertexToKey(glm::vec3& vec);

    std::map<std::string, std::map<std::pair<int, int>, bool> > connections;
    double depth;
    int pointX, pointY;
    int selected;
};
}

#endif
