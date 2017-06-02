#ifndef EDITOR_EFFECTOR_H
#define EDITOR_EFFECTOR_H

#include <map>
#include "data/mesh.h"
#include "rasterizer.h"

namespace oc {

class Effector : Rasterizer {
public:
    enum Effect{ GAMMA, SATURATION };

    void ApplyEffect(std::vector<Mesh>& mesh, Effect effect, float value);

    virtual void ProcessFragment(unsigned long& index, int &x, int &y, double &z);
    virtual void ProcessVertex(unsigned long& index, int &x1, int &x2, int &y, double &z1, double &z2);
    virtual bool VerticesOnly();

private:
    bool* mask;
    int stride;
    std::map<long, bool*> texture2mask;
};
}

#endif
