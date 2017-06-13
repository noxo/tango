#include "data/file3d.h"
#include "editor/effector.h"
#include "editor/selector.h"

namespace oc {

    void Effector::ApplyEffect(std::vector<Mesh> &mesh, Effector::Effect effect, float value) {

        if (mesh.empty())
            return;

        if ((effect == CONTRAST) || (effect == GAMMA) || (effect == SATURATION) || (effect == TONE) || (effect == RESET))
            ApplyColorEffect(mesh, effect, value);
        else
            ApplyGeometryEffect(mesh, effect, value);
    }

    void Effector::PreviewEffect(std::string& vs, std::string& fs, Effector::Effect effect) {
        if (effect == CONTRAST) {
            fs = "uniform sampler2D u_texture;\n"
                    "uniform float u_uniform;\n"
                    "varying vec4 f_color;\n"
                    "varying vec2 v_uv;\n"
                    "void main() {\n"
                    "  gl_FragColor = texture2D(u_texture, v_uv) - f_color;\n"
                    "  if (f_color.r < 0.005)\n"
                    "  {\n"
                    "    float c = 0.5;\n"
                    "    gl_FragColor.r -= (c - gl_FragColor.r) * u_uniform * 2.0;\n"
                    "    gl_FragColor.g -= (c - gl_FragColor.g) * u_uniform * 2.0;\n"
                    "    gl_FragColor.b -= (c - gl_FragColor.b) * u_uniform * 2.0;\n"
                    "  }\n"
                    "}";
        }
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
        if (effect == TONE) {
            fs = "uniform sampler2D u_texture;\n"
                 "uniform float u_uniform;\n"
                 "varying vec4 f_color;\n"
                 "varying vec2 v_uv;\n"
                 "void main() {\n"
                 "  gl_FragColor = texture2D(u_texture, v_uv) - f_color;\n"
                 "  if (f_color.r < 0.005)\n"
                 "  {\n"
                 "    float factor = u_uniform > 0.0 ? 3.0 : -3.0;\n"
                 "    float hue = abs(u_uniform);\n"
                 "    if ((hue >= 0.0) && (hue < 0.15))\n"
                 "      gl_FragColor.r += (hue - 0.0) * factor;\n"
                 "    if ((hue >= 0.15) && (hue < 0.3))\n"
                 "      gl_FragColor.r += (0.3 - hue) * factor;\n"
                 "    if ((hue >= 0.15) && (hue < 0.3))\n"
                 "      gl_FragColor.g += (hue - 0.15) * factor;\n"
                 "    if ((hue >= 0.3) && (hue < 0.45))\n"
                 "      gl_FragColor.g += (0.45 - hue) * factor;\n"
                 "    if ((hue >= 0.3) && (hue < 0.45))\n"
                 "      gl_FragColor.b += (hue - 0.3) * factor;\n"
                 "    if ((hue >= 0.45) && (hue < 0.6))\n"
                 "      gl_FragColor.b += (0.6 - hue) * factor;\n"
                 "  }\n"
                 "}";
        }
    }

    void Effector::ApplyColorEffect(std::vector<Mesh> &mesh, Effector::Effect effect, float value) {
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
        for (Mesh& m : mesh) {
            if (!m.image || (m.image->GetTexture() == -1))
                continue;
            if (m.imageOwner) {
                Image* img = 0;
                if (effect == RESET)
                    img = new Image(m.image->GetName());

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
                        else if ((effect == CONTRAST) || (effect == SATURATION)) {
                            c = effect == CONTRAST ? 128 : (r + g + b) / 3;
                            r -= (int) ((c - r) * fValue * 2.0f);
                            g -= (int) ((c - g) * fValue * 2.0f);
                            b -= (int) ((c - b) * fValue * 2.0f);
                        }
                        else if (effect == RESET) {
                            r = img->GetData()[index - 3];
                            g = img->GetData()[index - 2];
                            b = img->GetData()[index - 1];
                        }
                        else if (effect == TONE) {
                            float factor = 255.0f * (fValue > 0.0 ? 3.0f : -3.0f);
                            double hue = fabs(fValue);
                            if ((hue >= 0.0) && (hue < 0.15))
                                r += (hue - 0.0) * factor;
                            if ((hue >= 0.15) && (hue < 0.3))
                                r += (0.3 - hue) * factor;
                            if ((hue >= 0.15) && (hue < 0.3))
                                g += (hue - 0.15) * factor;
                            if ((hue >= 0.3) && (hue < 0.45))
                                g += (0.45 - hue) * factor;
                            if ((hue >= 0.3) && (hue < 0.45))
                                b += (hue - 0.3) * factor;
                            if ((hue >= 0.45) && (hue < 0.6))
                                b += (0.6 - hue) * factor;
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
                if (img)
                    delete img;
                m.image->UpdateTexture();
            }
        }

        //cleanup
        for (std::pair<const long, bool *> & m : texture2mask)
            delete[] m.second;
        texture2mask.clear();
    }

    void Effector::ApplyGeometryEffect(std::vector<Mesh> &mesh, Effector::Effect effect, float value) {
        for (Mesh& m : mesh) {
            long size = m.vertices.size();
            if (effect == CLONE) {
                for (long i = 0; i < size; i += 3) {
                    if (m.colors[i] == 0) {
                        //front face
                        for (int k = 0; k < 3; k++) {
                            m.colors.push_back(DESELECT_COLOR);
                            m.normals.push_back(m.normals[i + k]);
                            m.uv.push_back(m.uv[i + k]);
                            m.vertices.push_back(m.vertices[i + k]);
                        }
                        //back face
                        for (int k = 2; k >= 0; k--) {
                            m.colors.push_back(DESELECT_COLOR);
                            m.normals.push_back(m.normals[i + k]);
                            m.uv.push_back(m.uv[i + k]);
                            m.vertices.push_back(m.vertices[i + k]);
                        }
                    }
                }
            } else if (effect == DELETE) {
                std::vector<glm::vec3> vertices;
                std::vector<glm::vec3> normals;
                std::vector<unsigned int> colors;
                std::vector<glm::vec2> uv;
                for (long i = 0; i < size; i++) {
                    if (m.colors[i] != 0) {
                        colors.push_back(m.colors[i]);
                        normals.push_back(m.normals[i]);
                        uv.push_back(m.uv[i]);
                        vertices.push_back(m.vertices[i]);
                    }
                }
                m.colors = colors;
                m.normals = normals;
                m.uv = uv;
                m.vertices = vertices;
            }
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