#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include "data/file3d.h"
#include "gl/glsl.h"
#include "gl/renderer.h"
#include "tango/scan.h"

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

        Mesh frustum_;
        std::vector<Mesh> static_meshes_;
        GLSL* color_vertex_shader;
        GLSL* textured_shader;
        GLRenderer* renderer;
    };
}

#endif
