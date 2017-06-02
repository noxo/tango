#include "editor/selector.h"

#define SELECTOR_COLOR 0x004000

namespace oc {

    void Selector::ApplySelection(std::vector<Mesh>& mesh, glm::mat4 world2screen, float x, float y) {
        depth = INT_MAX;
        pointX = (int) x;
        pointY = (int) y;
        int selectModel = -1;
        int selectFace = -1;
        for (unsigned int i = 0; i < mesh.size(); i++) {
            selected = -1;
            AddVertices(mesh[i].vertices, world2screen);
            if (selected >= 0) {
                selectModel = i;
                selectFace = selected;
            }
        }
        if (selectModel >= 0) {
            mesh[selectModel].colors[selectFace + 0] = SELECTOR_COLOR;
            mesh[selectModel].colors[selectFace + 1] = SELECTOR_COLOR;
            mesh[selectModel].colors[selectFace + 2] = SELECTOR_COLOR;
        }
    }

    void Selector::Init(int w, int h) {
        SetResolution(w, h);
    }

    void Selector::ProcessFragment(unsigned long &index, int &x, int &y, double &z) {
        //dummy
    }

    void Selector::ProcessVertex(unsigned long &index, int &x1, int &x2, int &y, double &z1, double &z2) {
        if (pointY == y) {
            if ((x1 <= pointX) && (pointX <= x2)) {
                double z = z1 + (pointX - x1) * (z2 - z1) / (double)(x2 - x1);
                if ((z > 0) && (depth > z)) {
                    depth = z;
                    selected = (int) index;
                }
            }
        }
    }

    bool Selector::VerticesOnly() {
        return false;
    }
}