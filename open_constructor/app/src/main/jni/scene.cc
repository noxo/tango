#include "gl/opengl.h"
#include "scene.h"

std::string ColorFragmentShader() {
  return "varying vec4 v_color;\n"
         "void main() {\n"
         "  gl_FragColor = v_color;\n"
         "}\n";
}

std::string ColorVertexShader() {
  return "attribute vec4 v_vertex;\n"
         "attribute vec4 v_normal;\n"
         "uniform mat4 MVP;\n"
         "varying vec4 v_color;\n"
         "void main() {\n"
         "  gl_Position = MVP * v_vertex;\n"
         "  v_color = v_normal;\n"
         "}\n";
}

std::string TexturedFragmentShader() {
  return "uniform sampler2D u_texture;\n"
         "varying vec2 v_uv;\n"
         "void main() {\n"
         "  gl_FragColor = texture2D(u_texture, v_uv);\n"
         "}\n";
}

std::string TexturedVertexShader() {
  return "attribute vec4 v_vertex;\n"
         "attribute vec2 v_coord;\n"
         "varying vec2 v_uv;\n"
         "uniform mat4 MVP;\n"
         "void main() {\n"
         "  v_uv.x = v_coord.x;\n"
         "  v_uv.y = 1.0 - v_coord.y;\n"
         "  gl_Position = MVP * v_vertex;\n"
         "}\n";
}

namespace oc {

    Scene::Scene() { }

    Scene::~Scene() { }

    void Scene::InitGLContent() {
        renderer = new GLRenderer();
        color_vertex_shader = new GLSL(ColorVertexShader(), ColorFragmentShader());
        textured_shader = new GLSL(TexturedVertexShader(), TexturedFragmentShader());
    }

    void Scene::DeleteResources() {
        delete color_vertex_shader;
        color_vertex_shader = nullptr;
        delete textured_shader;
        textured_shader = nullptr;
    }

    void Scene::SetupViewPort(int w, int h) {
        glViewport(0, 0, w, h);
        renderer->Init(w, h, 1);
    }

    void Scene::Render(bool frustum) {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        unsigned int lastTexture = INT_MAX;
        for (GLMesh mesh : static_meshes_) {
            if (mesh.texture == -1) {
                color_vertex_shader->Bind();
                renderer->Render(mesh, -1);
            } else {
                unsigned int texture = textureMap[mesh.texture];
                if (lastTexture != texture) {
                    lastTexture = texture;
                    glBindTexture(GL_TEXTURE_2D, texture);
                }
                textured_shader->Bind();
                renderer->Render(mesh, -1);
            }
        }
        for (SingleDynamicMesh *mesh : dynamic_meshes_) {
            mesh->mutex.lock();
            if (mesh->mesh.texture == -1) {
                color_vertex_shader->Bind();
                renderer->Render(mesh->mesh, mesh->size);
            } else {
                unsigned int texture = textureMap[mesh->mesh.texture];
                if (lastTexture != texture) {
                    lastTexture = texture;
                    glBindTexture(GL_TEXTURE_2D, texture);
                }
                textured_shader->Bind();
                renderer->Render(mesh->mesh, mesh->size);
            }
            mesh->mutex.unlock();
        }
        color_vertex_shader->Bind();
        for (GLMesh mesh : debug_meshes_)
            renderer->Render(mesh, -1);
        if(!frustum_.vertices.empty() && frustum)
            renderer->Render(frustum_, frustum_.indices.size());
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

    void Scene::AddDynamicMesh(SingleDynamicMesh *mesh) {
        dynamic_meshes_.push_back(mesh);
    }

    void Scene::ClearDynamicMeshes() {
        for (unsigned int i = 0; i < dynamic_meshes_.size(); i++) {
            delete dynamic_meshes_[i];
        }
        dynamic_meshes_.clear();
        textureMap.clear();
    }
}
