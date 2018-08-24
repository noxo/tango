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

#define DOWNSIZE_FRAME 4
#define DOWNSIZE_TEXTURE 4

namespace oc {

    Medianer::Medianer(std::string path, std::string filename) {
        /// init dataset
        dataset = new oc::Dataset(path);
        dataset->GetCalibration(cx, cy, fx, fy);
        dataset->GetState(poseCount, width, height);
        SetResolution(width / DOWNSIZE_FRAME, height / DOWNSIZE_FRAME);
        currentDepth = new double[viewport_width * viewport_height];
        cx /= (double)width;
        cy /= (double)height;
        fx /= (double)width;
        fy /= (double)height;

        /// init frame rendering
        shader = new oc::GLSL(kMedianerShader[0], kMedianerShader[1]);

        /// load model
        File3d obj(dataset->GetPath() + "/" + filename, false);
        obj.ReadModel(25000, model);
        for (Mesh& m : model) {
            if (m.image && m.imageOwner) {
                m.image->Downsize(DOWNSIZE_TEXTURE);
                m.image->InitExtraData();
            }
        }

        /// init variables
        currentImage = 0;
        lastIndex = -1;
    }

    Medianer::~Medianer() {
        delete[] currentDepth;
        if (currentImage)
            delete currentImage;
        delete dataset;
        delete shader;
    }

    void Medianer::PreparePhoto(int index) {
        Image img(dataset->GetFileName(index, ".jpg"));
        img.Downsize(DOWNSIZE_FRAME);
        img.Write(dataset->GetFileName(index, ".edg"));
    }

    void Medianer::Process(unsigned long& index, int &x1, int &x2, int &y, glm::dvec3 &z1, glm::dvec3 &z2) {
        if ((y >= 0) && (y < viewport_height)) {
            glm::ivec4 color;
            int mem, memV;
            int offset = y * viewport_width;
            Image* i = model[currentMesh].image;
            for (int x = glm::max(x1, 0); x <= glm::min(x2, viewport_width - 1); x++) {
                glm::dvec3 z = z1 + (double)(x - x1) * (z2 - z1) / (double)(x2 - x1);
                if ((z.z > 0) && (currentDepth[y * viewport_width + x] >= z.z)) {
                    currentDepth[offset + x] = z.z;

                    //get color from frame
                    if (currentPass != PASS_DEPTH) {
                        z.t = 1.0 - z.t;
                        z.s *= i->GetWidth() - 1;
                        z.t *= i->GetHeight() - 1;
                        memV = (((int)z.t) * i->GetWidth() + (int)z.s);
                        mem = memV * 4;
                        color = currentImage->GetColorRGBA(x, y, 0);
                    }

                    //summary colors
                    if (currentPass == PASS_SUMMARY) {
                        color.a = 1;
                        i->GetExtraData()[memV] += color;
                    }

                    //average colors
                    if (currentPass == PASS_AVERAGE) {
                        color = i->GetExtraData()[memV];
                        color /= color.a;
                        color.a = 255;
                        i->GetExtraData()[memV] = color;
                        if (i->GetData()[mem + 3] == 0) {
                            i->GetData()[mem + 0] = color.r;
                            i->GetData()[mem + 1] = color.g;
                            i->GetData()[mem + 2] = color.b;
                        }
                        i->GetData()[mem + 3] = 255;
                    }

                    //repair texture
                    if (currentPass == PASS_REPAIR) {
                        glm::ivec4 diff1 = glm::abs(i->GetExtraData()[memV] - color);
                        glm::ivec4 diff2 = glm::abs(glm::ivec4(i->GetData()[mem + 0], i->GetData()[mem + 1], i->GetData()[mem + 2], 255) - color);
                        if (diff1.r + diff1.g + diff1.b > diff2.r + diff2.g + diff2.b) {
                            i->GetData()[mem + 0] = color.r;
                            i->GetData()[mem + 1] = color.g;
                            i->GetData()[mem + 2] = color.b;
                        }
                    }

                    //render texture into the frame
                    if (currentPass == PASS_SAVE) {
                        int index = (y * currentImage->GetWidth() + x) * 4;
                        currentImage->GetData()[index + 0] = i->GetData()[mem + 0];
                        currentImage->GetData()[index + 1] = i->GetData()[mem + 1];
                        currentImage->GetData()[index + 2] = i->GetData()[mem + 2];
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

    void Medianer::RenderTexture(int index, int mainPass) {
        for (int i = 0; i < viewport_width * viewport_height; i++)
            currentDepth[i] = 9999;
        if (index != lastIndex) {
            if (currentImage)
                delete currentImage;
            currentImage = new Image(dataset->GetFileName(index, ".edg"));
            lastIndex = index;
        }
        currentPose = glm::inverse(dataset->GetPose(index)[0]);
        for (currentPass = 0; currentPass < PASS_COUNT; currentPass++) {
            if (currentPass != PASS_DEPTH)
                if (currentPass != mainPass)
                    continue;
            for (currentMesh = 0; currentMesh < model.size(); currentMesh++)
                AddUVVertices(model[currentMesh].vertices, model[currentMesh].uv, currentPose,
                              cx - 0.5, cy - 0.5, 2.0 * fx, 2.0 * fy);
        }
        if (mainPass == PASS_SAVE)
            currentImage->Write(dataset->GetFileName(index, ".edg"));
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
