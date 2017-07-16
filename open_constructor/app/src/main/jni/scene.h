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
        double CountMatching();
        bool IsFullScreenActive() { return fullscreen_ != 0; }
        void Render(bool frustum);
        void SetupViewPort(int w, int h);
        void SetFullScreen(Image* img);
        void SetPreview(Image* img);
        void UpdateFrustum(glm::vec3 pos, float zoom);

        std::string ColorFragmentShader();
        std::string ColorVertexShader();
        std::string FullScreenFragmentShader();
        std::string FullScreenVertexShader();
        std::string TexturedFragmentShader();
        std::string TexturedVertexShader();
        GLuint Image2GLTexture(Image* img);

        int fullscreenTexture, previewTexture;
        Image* fullscreen_;
        Image* preview_;
        Mesh frustum_;
        std::vector<Mesh> static_meshes_;
        GLSL* color_vertex_shader;
        GLSL* full_screen_shader;
        GLSL* textured_shader;
        GLRenderer* renderer;

        std::string vertex;
        std::string fragment;
        float uniform;
        float uniformPitch;
        glm::vec3 uniformPos;
    private:
        std::string lastVertex;
        std::string lastFragment;
    };
}

#endif
