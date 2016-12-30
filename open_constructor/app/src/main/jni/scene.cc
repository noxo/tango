/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <tango-gl/conversions.h>
#include <tango-gl/tango-gl.h>

#include "mesh_builder/scene.h"

namespace {
    const char *kPerVertexColorVS =
            "precision mediump float;\n"
                    "precision mediump int;\n"
                    "\n"
                    "attribute vec4 vertex;\n"
                    "attribute vec4 color;\n"
                    "attribute vec2 uv;\n"
                    "uniform mat4 mvp;\n"
                    "varying vec4 vs_color;\n"
                    "varying vec2 f_textureCoords;\n"
                    "void main() {\n"
                    "  gl_Position = mvp * vertex;\n"
                    "  f_textureCoords = uv;\n"
                    "  vs_color = color;\n"
                    "}\n";

    const char *kPerVertexColorPS =
            "precision mediump float;\n"
                    "uniform sampler2D texture;\n"
                    "varying vec4 vs_color;\n"
                    "varying vec2 f_textureCoords;\n"
                    "void main() {\n"
                    //"  gl_FragColor = texture2D(texture, f_textureCoords);\n"
                    "  gl_FragColor = vs_color;\n"
                    "}\n";
}  // namespace

namespace mesh_builder {

    Scene::Scene() { }

    Scene::~Scene() { }

    void Scene::InitGLContent() {
        camera_ = new tango_gl::Camera();
        dynamic_mesh_material_ = new tango_gl::Material();
        dynamic_mesh_material_->SetShader(kPerVertexColorVS, kPerVertexColorPS);
    }

    void Scene::DeleteResources() {
        delete dynamic_mesh_material_;
        dynamic_mesh_material_ = nullptr;
    }

    void Scene::SetupViewPort(int w, int h) {
        if (h == 0) {
            LOGE("Setup graphic height not valid");
        }
        camera_->SetAspectRatio(static_cast<float>(w) / static_cast<float>(h));
        glViewport(0, 0, w, h);
    }

    void Scene::Render() {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        for (SingleDynamicMesh *mesh : dynamic_meshes_) {
            mesh->mutex.lock();
            tango_gl::Render(mesh->mesh, *dynamic_mesh_material_, tango_gl::Transform(), *camera_);
            mesh->mutex.unlock();
        }
        if(!frustum_.vertices.empty())
            tango_gl::Render(frustum_, *dynamic_mesh_material_, tango_gl::Transform(), *camera_);
    }

    void Scene::UpdateFrustum(glm::vec3 pos, float zoom) {
        if(frustum_.colors.empty()) {
            frustum_.render_mode = GL_TRIANGLES;
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

    void Scene::ClearDynamicMeshes() { dynamic_meshes_.clear(); }

}  // namespace mesh_builder
