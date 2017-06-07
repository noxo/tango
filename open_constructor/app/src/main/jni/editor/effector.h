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

    virtual void Process(unsigned long& index, int &x1, int &x2, int &y, double &z1, double &z2);

private:
    bool* mask;
    std::map<long, bool*> texture2mask;
};
}

#endif
