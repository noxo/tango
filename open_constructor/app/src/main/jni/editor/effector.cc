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
        float fValue = value / 255.0f;
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
                                r -= (int) ((c - r) * fValue * 2.0f);
                                g -= (int) ((c - g) * fValue * 2.0f);
                                b -= (int) ((c - b) * fValue * 2.0f);
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

    void Effector::PreviewEffect(std::string& vs, std::string& fs, Effector::Effect effect) {
        if (effect == GAMMA) {
            fs = "uniform sampler2D u_texture;\n"
                 "uniform float u_uniform;\n"
                 "varying vec4 f_color;\n"
                 "varying vec2 v_uv;\n"
                 "void main() {\n"
                 "  gl_FragColor = texture2D(u_texture, v_uv) - f_color;\n"
                 "  if (f_color.r < 0.005)\n"
                 "    gl_FragColor.rgb += u_uniform;\n"
                 "}";
        }
        if (effect == SATURATION) {
            fs = "uniform sampler2D u_texture;\n"
                    "uniform float u_uniform;\n"
                    "varying vec4 f_color;\n"
                    "varying vec2 v_uv;\n"
                    "void main() {\n"
                    "  gl_FragColor = texture2D(u_texture, v_uv) - f_color;\n"
                    "  if (f_color.r < 0.005)\n"
                    "  {\n"
                    "    float c = (gl_FragColor.r + gl_FragColor.g + gl_FragColor.b) / 3.0;\n"
                    "    gl_FragColor.r -= (c - gl_FragColor.r) * u_uniform * 2.0;\n"
                    "    gl_FragColor.g -= (c - gl_FragColor.g) * u_uniform * 2.0;\n"
                    "    gl_FragColor.b -= (c - gl_FragColor.b) * u_uniform * 2.0;\n"
                    "  }\n"
                    "}";
        }
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