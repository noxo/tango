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
    varying vec2 v_Screen;

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
      v_Screen.x = gl_Position.x * 0.5 + 0.5;
      v_Screen.y = 1.0 - gl_Position.y * 0.5 + 0.5;
    })glsl",

    R"glsl(
    uniform sampler2D u_texture;
    uniform sampler2D u_photo;
    uniform float u_diff;
    varying vec2 v_UV;
    varying vec2 v_Screen;

    void main() {
      gl_FragColor = texture2D(u_texture, v_UV);
      gl_FragColor -= texture2D(u_photo, v_Screen) * u_diff;
    })glsl"
};

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
            glm::vec4 color;
            int mem;
            int offset = y * viewport_width;
            for (int x = glm::max(x1, 0); x <= glm::min(x2, viewport_width - 1); x++) {
                glm::dvec3 z = z1 + (double)(x - x1) * (z2 - z1) / (double)(x2 - x1);
                if ((z.z > 0) && (currentDepth[y * viewport_width + x] >= z.z)) {
                    currentDepth[offset + x] = z.z;

                    if (currentPass == PASS_TEXTURE) {
                        //get color from frame
                        Image* i = model[currentMesh].image;
                        z.t = 1.0 - z.t;
                        z.s *= i->GetWidth() - 1;
                        z.t *= i->GetHeight() - 1;
                        color = currentImage->GetColorRGBA(x, y, 0);
                        color.a = z.z;

                        //apply color
                        mem = (((int)z.t) * i->GetWidth() + (int)z.s) * 4;
                        i->GetData()[mem + 0] = color.r;
                        i->GetData()[mem + 1] = color.g;
                        i->GetData()[mem + 2] = color.b;
                        i->GetData()[mem + 3] = 255;
                    }
                }
            }
        }
    }

    void Medianer::RenderDiff(int index) {
        currentImage = new Image(dataset->GetFileName(index, ".jpg"));
        shader->Bind();
        shader->UniformInt("u_photo", 1);
        glActiveTexture(GL_TEXTURE1);
        GLuint texture = Image2GLTexture(currentImage);
        glBindTexture(GL_TEXTURE_2D, texture);
        RenderPose(index, true);
        delete currentImage;
        glDeleteTextures(1, &texture);
    }

    void Medianer::RenderPose(int index, bool diff) {
        glm::mat4 view = glm::inverse(dataset->GetPose(index)[0]);
        shader->Bind();
        shader->UniformFloat("u_diff", diff ? 1 : 0);
        shader->UniformMatrix("u_view", glm::value_ptr(view));
        for (Mesh& m : model) {
            if (!m.vertices.empty()) {
                glActiveTexture(GL_TEXTURE0);
                if (m.image && (m.image->GetTexture() == -1))
                    m.image->SetTexture((long) Image2GLTexture(m.image));
                glBindTexture(GL_TEXTURE_2D, m.image->GetTexture());
                shader->UniformInt("u_texture", 0);
                shader->UniformFloat("u_cx", cx - 0.5f);
                shader->UniformFloat("u_cy",-cy + 0.5f);
                shader->UniformFloat("u_fx", 2.0f * fx);
                shader->UniformFloat("u_fy",-2.0f * fy);
                shader->Attrib(&m.vertices[0].x, 0, &m.uv[0].x, 0);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei) m.vertices.size());
            }
        }
        for (Mesh& m : model) {
            if (m.image && (m.image->GetTexture() != -1)) {
                GLuint texture = m.image->GetTexture();
                glDeleteTextures(1, &texture);
                m.image->SetTexture(-1);
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
        currentDepth = new double[viewport_width * viewport_height];
        for (int index = 0; index <= poseCount; index++) {
            RenderTexture(index, false);
#ifdef EXPORT_TEXTURE
            for (currentMesh = 0; currentMesh < model.size(); currentMesh++)
                model[currentMesh].image->Write("test.png");
#endif
        }
        delete[] currentDepth;
    }

    void Medianer::RenderTexture(int index, bool handleDepth) {
        if (handleDepth)
            currentDepth = new double[viewport_width * viewport_height];
        for (int i = 0; i < viewport_width * viewport_height; i++)
            currentDepth[i] = 9999;

        currentImage = new Image(dataset->GetFileName(index, ".jpg"));
        currentPose = glm::inverse(dataset->GetPose(index)[0]);
        for (currentPass = 0; currentPass < PASS_COUNT; currentPass++) {
            for (currentMesh = 0; currentMesh < model.size(); currentMesh++)
                AddUVVertices(model[currentMesh].vertices, model[currentMesh].uv, currentPose,
                              cx - 0.5, cy - 0.5, 2.0 * fx, 2.0 * fy);
        }
        delete currentImage;

        if (handleDepth)
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
