#ifndef EDITOR_EFFECTOR_H
#define EDITOR_EFFECTOR_H

#include <map>
#include "data/mesh.h"
#include "rasterizer.h"

namespace oc {

class Effector : Rasterizer {
public:
    enum Effect{ CONTRAST, GAMMA, SATURATION, TONE, RESET };

    void ApplyEffect(std::vector<Mesh>& mesh, Effect effect, float value);
    void PreviewEffect(std::string& vs, std::string& fs, Effect effect);

private:
    virtual void Process(unsigned long& index, int &x1, int &x2, int &y, double &z1, double &z2);

    bool* mask;
    std::map<long, bool*> texture2mask;
};
}

#endif
