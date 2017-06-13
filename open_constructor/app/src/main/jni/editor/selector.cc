#include <map>
#include <stack>
#include "editor/selector.h"

char buffer[1024];

namespace oc {

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

    glm::vec3 Selector::GetCenter(std::vector<Mesh> &mesh) {
        glm::vec3 min = glm::vec3(INT_MAX, INT_MAX, INT_MAX);
        glm::vec3 max = glm::vec3(INT_MIN, INT_MIN, INT_MIN);
        glm::vec3 p;

        for (unsigned int m = 0; m < mesh.size(); m++) {
            for (unsigned int i = 0; i < mesh[m].vertices.size(); i++) {
                if (mesh[m].colors[i] == 0) {
                    p = mesh[m].vertices[i];
                    if (min.x > p.x)
                        min.x = p.x;
                    if (min.z > p.z)
                        min.z = p.z;
                    if (max.x < p.x)
                        max.x = p.x;
                    if (max.z < p.z)
                        max.z = p.z;
                }
            }
        }
        return (min + max) * 0.5f;
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

    void Selector::Process(unsigned long &index, int &x1, int &x2, int &y, double &z1, double &z2) {
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

    std::string Selector::VertexToKey(glm::vec3& vec) {
        sprintf(buffer, "%.3f,%.3f,%.3f", vec.x, vec.y, vec.z);
        return std::string(buffer);
    }

    void Selector::SelectObject(std::vector<Mesh> &mesh, glm::mat4 world2screen, float x, float y) {
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
        if (selectModel < 0)
            return;

        //create graph of vertices
        std::string a, b, c;
        std::pair<int, int> p;
        if (connections.empty()) {
            for (unsigned int m = 0; m < mesh.size(); m++)
                for (unsigned int i = 0; i < mesh[m].vertices.size(); i += 3) {
                    a = VertexToKey(mesh[m].vertices[i + 0]);
                    b = VertexToKey(mesh[m].vertices[i + 1]);
                    c = VertexToKey(mesh[m].vertices[i + 2]);
                    p = std::pair<int, int>(m, i);
                    if (connections.find(a) == connections.end())
                        connections[a] = std::map<std::pair<int, int>, bool>();
                    if (connections.find(b) == connections.end())
                        connections[b] = std::map<std::pair<int, int>, bool>();
                    if (connections.find(c) == connections.end())
                        connections[c] = std::map<std::pair<int, int>, bool>();
                    connections[a][p] = true;
                    connections[b][p] = true;
                    connections[c][p] = true;
                }
        }

        //select initial triangle
        mesh[selectModel].colors[selectFace + 0] = 0;
        mesh[selectModel].colors[selectFace + 1] = 0;
        mesh[selectModel].colors[selectFace + 2] = 0;
        glm::vec3 va = mesh[selectModel].vertices[selectFace + 0];
        glm::vec3 vb = mesh[selectModel].vertices[selectFace + 1];
        glm::vec3 vc = mesh[selectModel].vertices[selectFace + 2];
        glm::vec3 n = glm::normalize(glm::cross(va - vb, va - vc));

        //process
        glm::vec3 n0;
        std::vector<std::string> keys;
        p = std::pair<int, int>(selectModel, selectFace);
        std::map<std::pair<int, int>, bool> processed;
        std::stack<std::pair<int, int> > toProcess;
        toProcess.push(p);
        while (!toProcess.empty()) {
            //update queue
            p = toProcess.top();
            toProcess.pop();
            if (processed.find(p) != processed.end())
                continue;
            processed[p] = true;

            //select and add neighbours
            keys.clear();
            keys.push_back(VertexToKey(mesh[p.first].vertices[p.second + 0]));
            keys.push_back(VertexToKey(mesh[p.first].vertices[p.second + 1]));
            keys.push_back(VertexToKey(mesh[p.first].vertices[p.second + 2]));
            for (std::string& j : keys) {
                for (std::pair<const std::pair<int, int>, bool>& i : connections[j]) {
                    va = mesh[i.first.first].vertices[i.first.second + 0];
                    vb = mesh[i.first.first].vertices[i.first.second + 1];
                    vc = mesh[i.first.first].vertices[i.first.second + 2];
                    n0 = glm::normalize(glm::cross(va - vb, va - vc));
                    if (glm::abs(n.x * n0.x + n.y * n0.y + n.z * n0.z) >= 0.97) {
                        mesh[i.first.first].colors[i.first.second + 0] = 0;
                        mesh[i.first.first].colors[i.first.second + 1] = 0;
                        mesh[i.first.first].colors[i.first.second + 2] = 0;
                        toProcess.push(i.first);
                    }
                }
            }
        }
    }

    void Selector::SelectTriangle(std::vector<Mesh> &mesh, glm::mat4 world2screen, float x, float y) {
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
}