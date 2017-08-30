#include "gl/opengl.h"
#include "scene.h"

namespace oc {

    Scene::Scene() : color_vertex_shader(0),
                     full_screen_shader(0),
                     textured_shader(0),
                     fullscreen_(0),
                     preview_(0),
                     previewTexture(-1),
                     uniform(0) {
        vertex = TexturedVertexShader();
        fragment = TexturedFragmentShader();
        lastVertex = vertex;
        lastFragment = fragment;
    }

    Scene::~Scene() {
        if (color_vertex_shader)
            delete color_vertex_shader;
        if (full_screen_shader)
            delete full_screen_shader;
        if (textured_shader)
            delete textured_shader;
        if (fullscreen_)
            delete fullscreen_;
        if (preview_)
            delete preview_;
    }

    double Scene::CountMatching() {
        if (!fullscreen_)
            return 0;
        if (!preview_)
            return 0;

        long match = 0;
        unsigned char* data1 = fullscreen_->GetData();
        unsigned char* data2 = preview_->GetData();
        unsigned int size = (unsigned int) (fullscreen_->GetWidth() * fullscreen_->GetHeight() * 3);
        for (unsigned int i = 0; i < size; i++)
            match += std::abs(data1[i] - data2[i]);
        double factor = 1.0 - match / (double)(size * 255);
        return glm::pow(factor, 2.0f);
    }

    void Scene::SetupViewPort(int w, int h) {
        glViewport(0, 0, w, h);
        renderer = new GLRenderer();
        renderer->Init(w, h, 1);
    }

    void Scene::SetFullScreen(Image *img) {
        if (fullscreen_)
            delete fullscreen_;
        fullscreen_ = img;
        fullscreenTexture = -1;
    }

    void Scene::SetPreview(Image *img) {
        if (preview_)
            delete preview_;
        preview_ = img;
    }

    void Scene::Render(bool frustum) {

        if ((lastVertex.compare(vertex) != 0) || (lastFragment.compare(fragment) != 0))
        {
            delete textured_shader;
            textured_shader = 0;
            lastVertex = vertex;
            lastFragment = fragment;
        }

        if (!color_vertex_shader)
            color_vertex_shader = new GLSL(ColorVertexShader(), ColorFragmentShader());
        if (!full_screen_shader)
            full_screen_shader = new GLSL(FullScreenVertexShader(), FullScreenFragmentShader());
        if (!textured_shader)
            textured_shader = new GLSL(vertex, fragment);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        long lastTexture = INT_MAX;
        for (Mesh& mesh : static_meshes_) {
            if (mesh.image && (mesh.image->GetTexture() == -1))
                mesh.image->SetTexture(Image2GLTexture(mesh.image));
            if (!mesh.image || (mesh.image->GetTexture() == -1)) {
                if (color_vertex_shader) {
                    color_vertex_shader->Bind();
                    renderer->Render(&mesh.vertices[0].x, 0, 0, mesh.colors.data(), mesh.vertices.size());
                }
            } else {
                if (lastTexture != mesh.image->GetTexture()) {
                    lastTexture = (unsigned int)mesh.image->GetTexture();
                    glBindTexture(GL_TEXTURE_2D, (unsigned int)mesh.image->GetTexture());
                }
                if (textured_shader) {
                    textured_shader->Bind();
                    textured_shader->UniformFloat("u_uniform", uniform);
                    textured_shader->UniformFloat("u_uniformPitch", uniformPitch);
                    textured_shader->UniformVec3("u_uniformPos", uniformPos.x, uniformPos.y, uniformPos.z);
                    renderer->Render(&mesh.vertices[0].x, 0, &mesh.uv[0].s, mesh.colors.data(), mesh.vertices.size());
                }
            }
        }
        if (color_vertex_shader) {
            color_vertex_shader->Bind();
            if(!frustum_.vertices.empty() && frustum)
                renderer->Render(&frustum_.vertices[0].x, 0, 0, frustum_.colors.data(),
                                 frustum_.indices.size(), frustum_.indices.data());
        }

        if (full_screen_shader && fullscreen_) {
            full_screen_shader->Bind();
            if (fullscreenTexture == -1)
                fullscreenTexture = Image2GLTexture(fullscreen_);
            glActiveTexture(GL_TEXTURE0);
            full_screen_shader->UniformTexture("u_texture", 0);
            glBindTexture(GL_TEXTURE_2D, fullscreenTexture);
            glDisable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            if (preview_)
            {
                glActiveTexture(GL_TEXTURE1);
                if (previewTexture != -1)
                    glDeleteTextures(1, (GLuint*)&previewTexture);
                previewTexture = Image2GLTexture(preview_);
                full_screen_shader->UniformTexture("u_preview", 1);
                glBindTexture(GL_TEXTURE_2D, previewTexture);
                glActiveTexture(GL_TEXTURE0);
            }

            /// vertices
            glm::vec3 vertices[] = {
                    glm::vec3(-1, +1, 0),
                    glm::vec3(-1, -1, 0),
                    glm::vec3(+1, -1, 0),
                    glm::vec3(-1, +1, 0),
                    glm::vec3(+1, -1, 0),
                    glm::vec3(+1, +1, 0),
            };

            /// render
            if (renderer->width < renderer->height) {
                glm::vec2 coords[] = {
                        glm::vec2(0, 0),
                        glm::vec2(1, 0),
                        glm::vec2(1, 1),
                        glm::vec2(0, 0),
                        glm::vec2(1, 1),
                        glm::vec2(0, 1),
                };
                full_screen_shader->Attrib(&vertices[0].x, 0, &coords[0].s, 0);
            } else {
                glm::vec2 coords[] = {
                        glm::vec2(0, 1),
                        glm::vec2(0, 0),
                        glm::vec2(1, 0),
                        glm::vec2(0, 1),
                        glm::vec2(1, 0),
                        glm::vec2(1, 1),
                };
                full_screen_shader->Attrib(&vertices[0].x, 0, &coords[0].s, 0);
            }
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDepthMask(GL_TRUE);
        }

        for (long i : Image::TexturesToDelete())
            glDeleteTextures(1, (const GLuint *) &i);
    }

    void Scene::UpdateFrustum(glm::vec3 pos, float zoom) {
        if(frustum_.colors.empty()) {
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.colors.push_back(0xFFFFFF00);
            frustum_.indices.push_back(0);
            frustum_.indices.push_back(1);
            frustum_.indices.push_back(2);
            frustum_.indices.push_back(0);
            frustum_.indices.push_back(1);
            frustum_.indices.push_back(3);
            frustum_.indices.push_back(0);
            frustum_.indices.push_back(1);
            frustum_.indices.push_back(4);
            frustum_.indices.push_back(0);
            frustum_.indices.push_back(1);
            frustum_.indices.push_back(5);
            frustum_.indices.push_back(2);
            frustum_.indices.push_back(3);
            frustum_.indices.push_back(4);
            frustum_.indices.push_back(2);
            frustum_.indices.push_back(3);
            frustum_.indices.push_back(5);
        }
        if(!frustum_.vertices.empty())
            frustum_.vertices.clear();
        float f = zoom * 0.005f;
        frustum_.vertices.push_back(pos + glm::vec3(f, 0, 0));
        frustum_.vertices.push_back(pos + glm::vec3(-f, 0, 0));
        frustum_.vertices.push_back(pos + glm::vec3(0, 0, f));
        frustum_.vertices.push_back(pos + glm::vec3(0, 0, -f));
        frustum_.vertices.push_back(pos + glm::vec3(0, f, 0));
        frustum_.vertices.push_back(pos + glm::vec3(0, -f, 0));
    }


    std::string Scene::ColorFragmentShader() {
        return "varying vec4 f_color;\n"
                "void main() {\n"
                "  gl_FragColor = f_color;\n"
                "}";
    }

    std::string Scene::ColorVertexShader() {
        return "attribute vec4 v_vertex;\n"
                "attribute vec4 v_color;\n"
                "uniform mat4 MVP;\n"
                "varying vec4 f_color;\n"
                "void main() {\n"
                "  gl_Position = MVP * v_vertex;\n"
                "  f_color = v_color;\n"
                "}";
    }

    std::string Scene::FullScreenFragmentShader() {
        return "uniform sampler2D u_texture;\n"
               "uniform sampler2D u_preview;\n"
                "varying vec2 v_uv;\n"
                "void main() {\n"
                "  vec4 rgba = texture2D(u_texture, v_uv);\n"
                "  float gray = (rgba.r + rgba.g + rgba.b) * 0.333;\n"
                "  gl_FragColor = vec4(gray, gray, gray, 1.0);\n"
                "  gl_FragColor += 0.25 * texture2D(u_preview, v_uv);\n"
                "}";
    }

    std::string Scene::FullScreenVertexShader() {
        return "attribute vec4 v_vertex;\n"
                "attribute vec2 v_coord;\n"
                "varying vec2 v_uv;\n"
                "void main() {\n"
                "  v_uv.x = v_coord.x;\n"
                "  v_uv.y = 1.0 - v_coord.y;\n"
                "  gl_Position = v_vertex;\n"
                "}";
    }

    std::string Scene::TexturedFragmentShader() {
        return "uniform sampler2D u_texture;\n"
                "varying vec4 f_color;\n"
                "varying vec2 v_uv;\n"
                "void main() {\n"
                "  gl_FragColor = texture2D(u_texture, v_uv) - f_color;\n"
                "}";
    }

    std::string Scene::TexturedVertexShader() {
        return "attribute vec4 v_vertex;\n"
                "attribute vec2 v_coord;\n"
                "attribute vec4 v_color;\n"
                "varying vec4 f_color;\n"
                "varying vec2 v_uv;\n"
                "uniform mat4 MVP;\n"
                "void main() {\n"
                "  f_color = v_color;\n"
                "  v_uv.x = v_coord.x;\n"
                "  v_uv.y = 1.0 - v_coord.y;\n"
                "  gl_Position = MVP * v_vertex;\n"
                "}";
    }

    GLuint Scene::Image2GLTexture(Image* img) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->GetWidth(), img->GetHeight(),
                     0, GL_RGB, GL_UNSIGNED_BYTE, img->GetData());
        return textureID;
    }
}
