#include "data/file3d.h"
#include "editor/effector.h"

namespace oc {

    bool Effector::IsSomethingSelected(std::vector<Mesh>& mesh) {
        for (Mesh& m : mesh)
            for (unsigned int& i : m.colors)
                if (i != 0)
                    return true;
        return false;
    }

    void Effector::ApplyEffect(std::vector<Mesh> &mesh, Effector::Effect effect, float value) {
        bool all = !IsSomethingSelected(mesh);

        if ((effect == GAMMA) || (effect == SATURATION)) {
            for (Mesh& m : mesh) {
                if (m.imageOwner) {
                    int c, r, g, b, index = 0;
                    unsigned char* data = m.image->GetData();
                    for (unsigned int i = 0; i < m.image->GetWidth() * m.image->GetHeight(); i++) {
                        r = data[index++];
                        g = data[index++];
                        b = data[index++];
                        if (effect == GAMMA) {
                            r += value;
                            g += value;
                            b += value;
                        }
                        if (effect == SATURATION) {
                            c = (r + g + b) / 3;
                            r -= (int) ((c - r) * value);
                            g -= (int) ((c - g) * value);
                            b -= (int) ((c - b) * value);
                        }
                        if (r < 0) r = 0;
                        if (g < 0) g = 0;
                        if (b < 0) b = 0;
                        if (r > 255) r = 255;
                        if (g > 255) g = 255;
                        if (b > 255) b = 255;
                        data[index - 3] = (unsigned char) r;
                        data[index - 2] = (unsigned char) g;
                        data[index - 1] = (unsigned char) b;
                    }
                }
                m.UpdateTexture();
            }
        }
    }
}