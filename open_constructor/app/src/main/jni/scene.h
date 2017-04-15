#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include "model_io.h"
#include "gl/glsl.h"
#include "gl/renderer.h"

namespace oc {
    class Scene {
    public:
        Scene();
        ~Scene();
        void InitGLContent();
        void DeleteResources();
        void SetupViewPort(int w, int h);
        void Render(bool frustum);
        void UpdateFrustum(glm::vec3 pos, float zoom);
        void AddDynamicMesh(SingleDynamicMesh* mesh);
        void ClearDynamicMeshes();

        GLMesh frustum_;
        std::vector<unsigned int> textureMap;
        std::vector<GLMesh> static_meshes_;
        std::vector<SingleDynamicMesh*> dynamic_meshes_;
        GLSL* color_vertex_shader;
        GLSL* textured_shader;
        GLRenderer* renderer;
    };
}

#endif
