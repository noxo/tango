#ifndef GL_RENDERER_H
#define GL_RENDERER_H

#include <stack>
#include <vector>
#include "gl/camera.h"
#include "gl/glsl.h"
#include "gl/mesh.h"
#include "gl/opengl.h"
#include "utils/math.h"

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
         * @brief RenderModel renders model into scene
         * @param m is instance of model to render
         * @param size is amount of indices to render indexed geometry or -1 to render vertex array
         */
        void Render(GLMesh m, int size);

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
