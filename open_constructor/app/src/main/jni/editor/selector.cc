#include <map>
#include "editor/selector.h"

#define DESELECT_COLOR 0x00204040

char buffer[1024];

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
            mesh[selectModel].colors[selectFace + 0] = 0;
            mesh[selectModel].colors[selectFace + 1] = 0;
            mesh[selectModel].colors[selectFace + 2] = 0;
        }
    }

    void Selector::CompleteSelection(std::vector<Mesh> &mesh, bool inverse) {
        for (unsigned int m = 0; m < mesh.size(); m++)
            for (unsigned int i = 0; i < mesh[m].vertices.size(); i++)
                mesh[m].colors[i] = inverse ? 0 : DESELECT_COLOR;
    }

    void Selector::DecreaseSelection(std::vector<Mesh> &mesh) {
        //get vertices to deselect
        std::map<std::string, bool> toDeselect;
        for (unsigned int m = 0; m < mesh.size(); m++)
            for (unsigned int i = 0; i < mesh[m].vertices.size(); i += 3)
                if ((mesh[m].colors[i + 0] != 0) ||
                    (mesh[m].colors[i + 1] != 0) ||
                    (mesh[m].colors[i + 2] != 0)) {
                    toDeselect[VertexToKey(mesh[m].vertices[i + 0])] = true;
                    toDeselect[VertexToKey(mesh[m].vertices[i + 1])] = true;
                    toDeselect[VertexToKey(mesh[m].vertices[i + 2])] = true;
                }

        //deselect vertices
        for (unsigned int m = 0; m < mesh.size(); m++)
            for (unsigned int i = 0; i < mesh[m].vertices.size(); i += 3)
                if ((toDeselect.find(VertexToKey(mesh[m].vertices[i + 0])) != toDeselect.end()) ||
                    (toDeselect.find(VertexToKey(mesh[m].vertices[i + 1])) != toDeselect.end()) ||
                    (toDeselect.find(VertexToKey(mesh[m].vertices[i + 2])) != toDeselect.end())) {
                    mesh[m].colors[i + 0] = DESELECT_COLOR;
                    mesh[m].colors[i + 1] = DESELECT_COLOR;
                    mesh[m].colors[i + 2] = DESELECT_COLOR;
                }
    }

    void Selector::IncreaseSelection(std::vector<Mesh>& mesh) {

        //get vertices to select
        std::map<std::string, bool> toSelect;
        for (unsigned int m = 0; m < mesh.size(); m++)
            for (unsigned int i = 0; i < mesh[m].vertices.size(); i += 3)
                if ((mesh[m].colors[i + 0] == 0) ||
                    (mesh[m].colors[i + 1] == 0) ||
                    (mesh[m].colors[i + 2] == 0)) {
                    toSelect[VertexToKey(mesh[m].vertices[i + 0])] = true;
                    toSelect[VertexToKey(mesh[m].vertices[i + 1])] = true;
                    toSelect[VertexToKey(mesh[m].vertices[i + 2])] = true;
                }

        //select vertices
        for (unsigned int m = 0; m < mesh.size(); m++)
            for (unsigned int i = 0; i < mesh[m].vertices.size(); i += 3)
                if ((toSelect.find(VertexToKey(mesh[m].vertices[i + 0])) != toSelect.end()) ||
                    (toSelect.find(VertexToKey(mesh[m].vertices[i + 1])) != toSelect.end()) ||
                    (toSelect.find(VertexToKey(mesh[m].vertices[i + 2])) != toSelect.end())) {
                    mesh[m].colors[i + 0] = 0;
                    mesh[m].colors[i + 1] = 0;
                    mesh[m].colors[i + 2] = 0;
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

    std::string Selector::VertexToKey(glm::vec3& vec) {
        sprintf(buffer, "%.3f,%.3f,%.3f", vec.x, vec.y, vec.z);
        return std::string(buffer);
    }
}