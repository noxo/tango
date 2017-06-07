#include "data/file3d.h"
#include "editor/effector.h"

namespace oc {

    void Effector::ApplyEffect(std::vector<Mesh> &mesh, Effector::Effect effect, float value) {

        if (mesh.empty())
            return;

        // create masks
        for (Mesh& m : mesh) {
            if (!m.image || (m.image->GetTexture() == -1))
                continue;
            if (texture2mask.find(m.image->GetTexture()) == texture2mask.end()) {
                mask = new bool[m.image->GetWidth() * m.image->GetHeight()];
                for (unsigned int i = 0; i < m.image->GetWidth() * m.image->GetHeight(); i++)
                    mask[i] = false;
                texture2mask[m.image->GetTexture()] = mask;
            }
        }

        // fill masks
        for (Mesh& m : mesh) {
            if (!m.image || (m.image->GetTexture() == -1))
                continue;
            mask = texture2mask[m.image->GetTexture()];
            SetResolution(m.image->GetWidth(), m.image->GetHeight());
            AddUVS(m.uv, m.colors);
        }

        // apply effect
        if ((effect == GAMMA) || (effect == SATURATION)) {
            for (Mesh& m : mesh) {
                if (!m.image || (m.image->GetTexture() == -1))
                    continue;
                if (m.imageOwner) {
                    int c, r, g, b, index = 0;
                    unsigned char* data = m.image->GetData();
                    mask = texture2mask[m.image->GetTexture()];
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
                    m.image->UpdateTexture();
                }
            }
        }

        //cleanup
        for (std::pair<const long, bool *> & m : texture2mask)
            delete[] m.second;
        texture2mask.clear();
    }

    void Effector::Process(unsigned long &index, int &x1, int &x2, int &y, double &z1, double &z2) {
        int start = x1 - 2;
        if (start < 0)
            start = 0;
        int finish = x2 + 2;
        if (finish >= viewport_width)
            finish = viewport_width - 1;

        for (int i = -2; i <= 2; i++) {
            if (y + i < 0)
                continue;
            if (y + i >= viewport_height)
                continue;
            int offset = (y + i) * viewport_width;
            for (int x = start; x <= finish; x++)
                mask[x + offset] = true;
        }
    }
}