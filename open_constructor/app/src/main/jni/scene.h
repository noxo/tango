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
        void SetupViewPort(int w, int h);
        void Render(bool frustum);
        void UpdateFrustum(glm::vec3 pos, float zoom);

        std::string ColorFragmentShader();
        std::string ColorVertexShader();
        std::string TexturedFragmentShader();
        std::string TexturedVertexShader();

        Mesh frustum_;
        std::vector<Mesh> static_meshes_;
        GLSL* color_vertex_shader;
        GLSL* textured_shader;
        GLRenderer* renderer;

        std::string vertex;
        std::string fragment;
        float uniform;
    private:
        std::string lastVertex;
        std::string lastFragment;
    };
}

#endif
