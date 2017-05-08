#ifndef GL_RENDERER_H
#define GL_RENDERER_H

#include <stack>
#include <vector>
#include "data/mesh.h"
#include "gl/camera.h"
#include "gl/glsl.h"
#include "gl/opengl.h"

namespace oc {
    class GLRenderer {

    public:
        /**
         * @brief constructor
         */
        GLRenderer();

        /**
         * @brief destructor
         */
        ~GLRenderer();

        /**
         * @brief Init inits renderer
         * @param w is screen width
         * @param h is screen height
         * @param a is screen aliasing(reducing resolution)
         */
        void Init(int w, int h, float a);

        /**
         * @brief Render renders model into scene
         * @param vertices is pointer to vertices
         * @param normals is pointer to normals
         * @param uv is pointer to uv
         * @param colors is pointer to colors
         * @param size is of indices array for indexed geometry or size of vertex array for nonindiced
         * @param indices is pointer to indices
         */
        void Render(float* vertices, float* normals, float* uv, unsigned int* colors,
                    int size, unsigned int* indices = 0);

        /**
         * @brief Rtt enables rendering into FBO which makes posible to do reflections
         * @param enable is true to start drawing, false to render on screen
         */
        void Rtt(bool enable);

        GLCamera camera;

    private:
        void Cleanup();

        float aliasing;                       ///< Screen detail
        int width;                            ///< Screen width
        int height;                           ///< Screen height
        GLSL* scene;                          ///< Scene shader
        unsigned int* rendertexture;          ///< Texture for color buffer
        unsigned int* fboID;                  ///< Frame buffer object id
        unsigned int* rboID;                  ///< Render buffer object id
    };
}

#endif // GLES20_H
