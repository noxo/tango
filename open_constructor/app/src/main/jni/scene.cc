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
#include <tango-gl/shaders.h>
#include <tango-gl/tango-gl.h>

#include "scene.h"

namespace mesh_builder {

    Scene::Scene() { }

    Scene::~Scene() { }

    void Scene::InitGLContent() {
        camera_ = new tango_gl::Camera();
        color_vertex_shader = new tango_gl::Material();
        color_vertex_shader->SetShader(tango_gl::shaders::GetColorVertexShader().c_str(),
                                          tango_gl::shaders::GetBasicFragmentShader().c_str());
        textured_shader = new tango_gl::Material();
        textured_shader->SetShader(tango_gl::shaders::GetTexturedVertexShader().c_str(),
                                       tango_gl::shaders::GetTexturedVertexShader().c_str());
    }

    void Scene::DeleteResources() {
        delete color_vertex_shader;
        color_vertex_shader = nullptr;
        delete textured_shader;
        textured_shader = nullptr;
    }

    void Scene::SetupViewPort(int w, int h) {
        if (h == 0) {
            LOGE("Setup graphic height not valid");
        }
        camera_->SetAspectRatio(static_cast<float>(w) / static_cast<float>(h));
        glViewport(0, 0, w, h);
    }

    void Scene::Render(bool frustum) {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        for (tango_gl::StaticMesh mesh : static_meshes_) {
            if (mesh.texture == -1)
                tango_gl::Render(mesh, *color_vertex_shader, tango_gl::Transform(), *camera_, -1);
            else {
                //TODO:render textured meshes
            }
        }
        for (SingleDynamicMesh *mesh : dynamic_meshes_) {
            mesh->mutex.lock();
            tango_gl::Render(mesh->mesh, *color_vertex_shader, tango_gl::Transform(), *camera_, mesh->size);
            mesh->mutex.unlock();
        }
        if(!frustum_.vertices.empty() && frustum)
            tango_gl::Render(frustum_, *color_vertex_shader, tango_gl::Transform(), *camera_, frustum_.indices.size());
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
