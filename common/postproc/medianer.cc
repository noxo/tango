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

namespace oc {

    Medianer::Medianer(std::string path, std::string filename, bool renderToFileSupport) {
        /// init dataset
        dataset = new oc::Dataset(path);
        dataset->GetCalibration(cx, cy, fx, fy);
        dataset->GetState(poseCount, width, height);

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

    void Medianer::RenderPose(int index) {
        glm::mat4 view = glm::inverse(dataset->GetPose(index)[0]);
        shader->Bind();
        shader->UniformMatrix("u_view", glm::value_ptr(view));
        for (Mesh& m : model) {
            if (!m.vertices.empty()) {
                if (m.image && (m.image->GetTexture() == -1))
                    m.image->SetTexture((long) Image2GLTexture(m.image));
                glBindTexture(GL_TEXTURE_2D, m.image->GetTexture());
                shader->UniformFloat("u_cx", cx / (float)width - 0.5f);
                shader->UniformFloat("u_cy",-cy / (float)height + 0.5f);
                shader->UniformFloat("u_fx", 2.0f * fx / (float)width);
                shader->UniformFloat("u_fy",-2.0f * fy / (float)height);
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
