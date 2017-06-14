#include "gl/opengl.h"
#include "scene.h"

namespace oc {

    Scene::Scene() : color_vertex_shader(0), textured_shader(0), uniform(0) {
        vertex = TexturedVertexShader();
        fragment = TexturedFragmentShader();
    }

    Scene::~Scene() {
        delete color_vertex_shader;
        color_vertex_shader = 0;
        delete textured_shader;
        textured_shader = 0;
    }

    void Scene::SetupViewPort(int w, int h) {
        glViewport(0, 0, w, h);
        renderer = new GLRenderer();
        renderer->Init(w, h, 1);
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
        if (!textured_shader)
            textured_shader = new GLSL(vertex, fragment);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        long lastTexture = INT_MAX;
        for (Mesh& mesh : static_meshes_) {
            if (mesh.image && (mesh.image->GetTexture() == -1)) {
                GLuint textureID;
                glGenTextures(1, &textureID);
                mesh.image->SetTexture(textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mesh.image->GetWidth(), mesh.image->GetHeight(),
                             0, GL_RGB, GL_UNSIGNED_BYTE, mesh.image->GetData());
            }
            if (!mesh.image || (mesh.image->GetTexture() == -1)) {
                color_vertex_shader->Bind();
                renderer->Render(&mesh.vertices[0].x, 0, 0, mesh.colors.data(), mesh.vertices.size());
            } else {
                if (lastTexture != mesh.image->GetTexture()) {
                    lastTexture = (unsigned int)mesh.image->GetTexture();
                    glBindTexture(GL_TEXTURE_2D, (unsigned int)mesh.image->GetTexture());
                }
                textured_shader->Bind();
                textured_shader->UniformFloat("u_uniform", uniform);
                textured_shader->UniformFloat("u_uniformPitch", uniformPitch);
                textured_shader->UniformVec3("u_uniformPos", uniformPos.x, uniformPos.y, uniformPos.z);
                renderer->Render(&mesh.vertices[0].x, 0, &mesh.uv[0].s, mesh.colors.data(), mesh.vertices.size());
            }
        }
        color_vertex_shader->Bind();
        if(!frustum_.vertices.empty() && frustum)
            renderer->Render(&frustum_.vertices[0].x, 0, 0, frustum_.colors.data(),
                             frustum_.indices.size(), frustum_.indices.data());

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
}
