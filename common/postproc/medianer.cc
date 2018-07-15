#include <algorithm>
#include "postproc/medianer.h"

const char* kMedianerShader[] = {R"glsl(
    uniform mat4 u_view;
    uniform float u_cx;
    uniform float u_cy;
    uniform float u_fx;
    uniform float u_fy;
    attribute vec4 v_vertex;
    attribute vec2 v_coord;
    varying vec2 v_UV;

    void main() {
      v_UV.x = v_coord.x;
      v_UV.y = 1.0 - v_coord.y;
      gl_Position = u_view * v_vertex;
      gl_Position.xy /= abs(gl_Position.z * gl_Position.w);
      gl_Position.x *= u_fx;
      gl_Position.y *= u_fy;
      gl_Position.x += u_cx;
      gl_Position.y += u_cy;
      gl_Position.z *= 0.01;
      gl_Position.w = 1.0;
    })glsl",

    R"glsl(
    uniform sampler2D u_texture;
    varying vec2 v_UV;

    void main() {
      gl_FragColor = texture2D(u_texture, v_UV);
    })glsl"
};

//#define COLORMIX_AVERAGE
#define COLORMIX_MEDIAN
//#define COLORMIX_NEAREST
#define EXPORT_TEXTURE

namespace oc {

    Medianer::Medianer(std::string path, std::string filename, bool renderToFileSupport) {
        /// init dataset
        dataset = new oc::Dataset(path);
        dataset->GetCalibration(cx, cy, fx, fy);
        dataset->GetState(poseCount, width, height);
        SetResolution(width, height);
        cx /= (double)width;
        cy /= (double)height;
        fx /= (double)width;
        fy /= (double)height;

        /// init frame rendering
        if (renderToFileSupport)
            renderer.Init(width, height, width, height);
        shader = new oc::GLSL(kMedianerShader[0], kMedianerShader[1]);

        /// load model
        File3d obj(dataset->GetPath() + "/" + filename, false);
        obj.ReadModel(25000, model);
    }

    Medianer::~Medianer() {
        delete dataset;
        delete shader;
    }

    void Medianer::Process(unsigned long& index, int &x1, int &x2, int &y, glm::dvec3 &z1, glm::dvec3 &z2) {
        if ((y >= 0) && (y < viewport_height)) {
            int offset = y * viewport_width;
            for (int x = glm::max(x1, 0); x <= glm::min(x2, viewport_width - 1); x++) {
                glm::dvec3 z = z1 + (double)(x - x1) * (z2 - z1) / (double)(x2 - x1);
                if ((z.z > 0) && (currentDepth[y * viewport_width + x] >= z.z)) {
                    currentDepth[offset + x] = z.z;
                    if (writeTexture) {
                        //get color from frame
                        Image* i = model[currentMesh].image;
                        z.t = 1.0 - z.t;
                        z.s *= i->GetWidth() - 1;
                        z.t *= i->GetHeight() - 1;
                        int mem = ((int)z.t) * i->GetWidth() + (int)z.s;
                        glm::vec4 color = currentImage->GetColorRGBA(x, y, 0);
                        color.a = z.z;
                        model[currentMesh].image->GetAditionalData()[mem].push_back(color);

#ifdef COLORMIX_AVERAGE
                        color = glm::vec4(0);
                        for (glm::vec4 c : model[currentMesh].image->GetAditionalData()[mem]) {
                            color += c;
                        }
                        color /= model[currentMesh].image->GetAditionalData()[mem].size();
#endif
#ifdef COLORMIX_MEDIAN
                        color = glm::vec4(255);
                        std::vector<int> r, g, b;
                        for (glm::vec4 c : model[currentMesh].image->GetAditionalData()[mem]) {
                            r.push_back(c.r);
                            g.push_back(c.g);
                            b.push_back(c.b);
                        }
                        std::sort(r.begin(), r.end());
                        std::sort(g.begin(), g.end());
                        std::sort(b.begin(), b.end());
                        color.r = r[r.size() / 2];
                        color.g = g[g.size() / 2];
                        color.b = b[b.size() / 2];
#endif
#ifdef COLORMIX_NEAREST
                        for (glm::vec4 c : model[currentMesh].image->GetAditionalData()[mem]) {
                            if (color.a > c.a) {
                                color = c;
                            }
                        }
#endif

                        //apply color
                        mem *= 4;
                        i->GetData()[mem + 0] = color.r;
                        i->GetData()[mem + 1] = color.g;
                        i->GetData()[mem + 2] = color.b;
                        i->GetData()[mem + 3] = 255;
                    }
                }
            }
        }
    }

    void Medianer::RenderPose(int index) {
        glm::mat4 view = glm::inverse(dataset->GetPose(index)[0]);
        shader->Bind();
        shader->UniformMatrix("u_view", glm::value_ptr(view));
        for (Mesh& m : model) {
            if (!m.vertices.empty()) {
                if (m.image && (m.image->GetTexture() == -1))
                    m.image->SetTexture((long) Image2GLTexture(m.image));
                glBindTexture(GL_TEXTURE_2D, m.image->GetTexture());
                shader->UniformFloat("u_cx", cx - 0.5f);
                shader->UniformFloat("u_cy",-cy + 0.5f);
                shader->UniformFloat("u_fx", 2.0f * fx);
                shader->UniformFloat("u_fy",-2.0f * fy);
                shader->Attrib(&m.vertices[0].x, 0, &m.uv[0].x, 0);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei) m.vertices.size());
            }
        }
    }

    void Medianer::RenderPose(int index, std::string filename) {
        renderer.Rtt(true);
        RenderPose(index);
        renderer.Rtt(false);
        oc::Image* img = renderer.ReadRtt();
        img->Write(filename);
        delete img;
    }

    void Medianer::RenderTexture() {

        //clear textures
        currentDepth = new double[viewport_width * viewport_height];
        for (Mesh& m : model) {
            if (m.image && m.imageOwner) {
                m.image->InitAditionalData();
                for (int i = 0; i < m.image->GetWidth() * m.image->GetHeight() * 4; i++)
                    m.image->GetData()[i] = 255;
            }
        }

        //render the texture
        for (int index = 0; index < poseCount; index++) {
            for (int i = 0; i < viewport_width * viewport_height; i++)
                currentDepth[i] = 9999;
            currentImage = new Image(dataset->GetFileName(index, ".jpg"));
            currentPose = glm::inverse(dataset->GetPose(index)[0]);
            for (int pass = 0; pass < 2; pass++) {
                writeTexture = pass == 1;
                for (currentMesh = 0; currentMesh < model.size(); currentMesh++)
                    AddUVVertices(model[currentMesh].vertices, model[currentMesh].uv, currentPose,
                                  cx - 0.5, cy - 0.5, 2.0 * fx, 2.0 * fy);
            }
            delete currentImage;

#ifdef EXPORT_TEXTURE
            for (currentMesh = 0; currentMesh < model.size(); currentMesh++)
                model[currentMesh].image->Write("test.png");
#endif
        }
        delete[] currentDepth;
    }

    GLuint Medianer::Image2GLTexture(oc::Image* img) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->GetWidth(), img->GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img->GetData());
        glGenerateMipmap(GL_TEXTURE_2D);
        return textureID;
    }
}
