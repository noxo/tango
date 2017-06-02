#ifndef EDITOR_EFFECTOR_H
#define EDITOR_EFFECTOR_H

#include "data/mesh.h"

namespace oc {

class Effector {
public:
    enum Effect{ GAMMA, SATURATION };

    void ApplyEffect(std::vector<Mesh>& mesh, Effect effect, float value);

private:
    bool IsSomethingSelected(std::vector<Mesh>& mesh);
};
}

#endif
