#ifndef GL_GLSL_H
#define GL_GLSL_H

#include <string>
#include <vector>

namespace oc {
    class GLSL {
    public:
        /**
         * @brief Constructor
         * @param vert is vertex shader code
         * @param frag is fragment shader code
         */
        GLSL(std::string vert, std::string frag);

        ~GLSL();

        /**
         * @brief it sets pointer to geometry
         * @param size is amount of data
         */
        void Attrib(unsigned int size);

        /**
         * @brief it sends geometry into GPU
         * @param vertices is vertices
         * @param normals is normals
         * @param coords is texture coords
         */
        void Attrib(float* vertices, float* normals, float* coords);

        /**
         * @brief it binds shader
         */
        void Bind();

        /**
         * @brief initShader creates shader from code
         * @param vs is vertex shader code
         * @param fs is fragment shader code
         * @return shader program id
         */
        unsigned int InitShader(const char *vs, const char *fs);

        /**
         * @brief it unbinds shader
         */
        void Unbind();

        /**
         * @brief UniformFloat send float into shader
         * @param name is uniform name
         * @param value is uniform value
         */
        void UniformFloat(const char* name, float value);

        /**
         * @brief UniformMatrix send matrix into shader
         * @param name is uniform name
         * @param value is uniform value
         */
        void UniformMatrix(const char* name, float* value);

    private:
        unsigned int id;          ///< Shader id
        unsigned int shader_vp;   ///< Vertex shader
        unsigned int shader_fp;   ///< Fragment shader
        int attribute_v_vertex;   ///< VBO vertices
        int attribute_v_coord;    ///< VBO coords
        int attribute_v_normal;   ///< VBO normals
    };
}
#endif
