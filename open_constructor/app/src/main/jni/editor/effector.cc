#include "data/file3d.h"
#include "editor/effector.h"

namespace oc {

    void Effector::ApplyEffect(std::vector<Mesh> &mesh, Effector::Effect effect, float value) {

        if (mesh.empty())
            return;

        // create masks
        for (Mesh& m : mesh) {
            if (m.texture == -1)
                continue;
            if (texture2mask.find(m.texture) == texture2mask.end()) {
                mask = new bool[m.image->GetWidth() * m.image->GetHeight()];
                for (unsigned int i = 0; i < m.image->GetWidth() * m.image->GetHeight(); i++)
                    mask[i] = false;
                texture2mask[m.texture] = mask;
            }
        }

        // fill masks
        for (Mesh& m : mesh) {
            if (m.texture == -1)
                continue;
            mask = texture2mask[m.texture];
            stride = m.image->GetWidth();
            SetResolution(m.image->GetWidth(), m.image->GetHeight());
            AddUVS(m.uv, m.colors);
        }

        // apply effect
        if ((effect == GAMMA) || (effect == SATURATION)) {
            for (Mesh& m : mesh) {
                if (m.imageOwner) {
                    int c, r, g, b, index = 0;
                    unsigned char* data = m.image->GetData();
                    mask = texture2mask[m.texture];
                    for (unsigned int i = 0; i < m.image->GetWidth() * m.image->GetHeight(); i++) {
                        r = data[index++];
                        g = data[index++];
                        b = data[index++];
                        //effects implementation
                        if (mask[i]) {
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
                }
                m.UpdateTexture();
            }
        }

        //cleanup
        for (std::pair<const long, bool *> & m : texture2mask)
            delete[] m.second;
        texture2mask.clear();
    }

    void Effector::ProcessFragment(unsigned long &index, int &x, int &y, double &z) {
        //dummy
    }

    void Effector::ProcessVertex(unsigned long &index, int &x1, int &x2, int &y, double &z1, double &z2) {
        int offset = y * stride;
        int start = x1;
        if (start < 0)
            start = 0;
        int finish = x2;
        if (finish >= stride)
            finish = stride - 1;
        for (int x = start; x <= finish; x++)
          mask[x + offset] = true;
    }

    bool Effector::VerticesOnly() {
        return true;
    }
}