#include <stdint.h>
#include "gl/glsl.h"
#include "gl/opengl.h"
#include "utils/io.h"

oc::GLSL* gl_last_shader = 0;

namespace oc {
    GLSL::GLSL(std::string vert, std::string frag) {
        /// add header
        std::string header = "#version 100\nprecision highp float;\n";
        vert = header + vert;
        frag = header + frag;

        /// compile shader
        id = InitShader(vert.c_str(), frag.c_str());

        /// Attach VBO attributes
        attribute_v_vertex = glGetAttribLocation(id, "v_vertex");
        attribute_v_coord = glGetAttribLocation(id, "v_coord");
        attribute_v_normal = glGetAttribLocation(id, "v_normal");
        attribute_v_color = glGetAttribLocation(id, "v_color");
    }

    GLSL::~GLSL() {
        glDetachShader(id, shader_vp);
        glDetachShader(id, shader_fp);
        glDeleteShader(shader_vp);
        glDeleteShader(shader_fp);
        glUseProgram(0);
        glDeleteProgram(id);
    }

    void GLSL::Attrib(glm::vec3* vertices, glm::vec3* normals, glm::vec2* coords, unsigned int* colors) {
        /// send attributes to GPU
        glVertexAttribPointer(attribute_v_vertex, 3, GL_FLOAT, GL_FALSE, 0, &vertices[0].x);
        if ((attribute_v_normal != -1) && (normals != 0))
          glVertexAttribPointer(attribute_v_normal, 3, GL_FLOAT, GL_FALSE, 0, &normals[0].x);
        if ((attribute_v_coord != -1) && (coords != 0))
          glVertexAttribPointer(attribute_v_coord, 2, GL_FLOAT, GL_FALSE, 0, &coords[0].x);
        if ((attribute_v_color != -1) && (colors != 0))
          glVertexAttribPointer(attribute_v_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, colors);
    }

    void GLSL::Bind() {
        /// bind shader
        if (gl_last_shader == this)
            return;
        glUseProgram(id);
        gl_last_shader = this;

        /// set attributes
        glEnableVertexAttribArray(attribute_v_vertex);
        if (attribute_v_normal != -1)
            glEnableVertexAttribArray(attribute_v_normal);
        if (attribute_v_coord != -1)
            glEnableVertexAttribArray(attribute_v_coord);
        if (attribute_v_color != -1)
            glEnableVertexAttribArray(attribute_v_color);
    }

    GLSL* GLSL::CurrentShader() {
        return gl_last_shader;
    }

    unsigned int GLSL::InitShader(const char *vs, const char *fs) {
        /// Load shader
        shader_vp = glCreateShader(GL_VERTEX_SHADER);
        shader_fp = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(shader_vp, 1, &vs, 0);
        glShaderSource(shader_fp, 1, &fs, 0);

        /// Alocate buffer for logs
        const unsigned int BUFFER_SIZE = 512;
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        GLsizei length = 0;

        /// Compile shaders
        glCompileShader(shader_vp);
        glGetShaderInfoLog(shader_vp, BUFFER_SIZE, &length, buffer);
        if (length > 0)
            LOGI("GLSL compile log: %s\n%s", buffer, vs);
        glCompileShader(shader_fp);
        glGetShaderInfoLog(shader_fp, BUFFER_SIZE, &length, buffer);
        if (length > 0)
            LOGI("GLSL compile log: %s\n%s", buffer, fs);

        /// Link program
        unsigned int shader_id = glCreateProgram();
        glAttachShader(shader_id, shader_fp);
        glAttachShader(shader_id, shader_vp);
        glLinkProgram(shader_id);
        glGetProgramInfoLog(shader_id, BUFFER_SIZE, &length, buffer);
        if (length > 0)
            LOGI("GLSL program info log: %s", buffer);

        /// Check shader
        glValidateProgram(shader_id);
        GLint status;
        glGetProgramiv(shader_id, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
            LOGI("GLSL error linking");
        return shader_id;
    }

    void GLSL::Unbind() {
        gl_last_shader = 0;
        glUseProgram(0);
    }

    void GLSL::UniformFloat(const char* name, float value) {
        glUniform1f(glGetUniformLocation(id, name), value);
    }

    void GLSL::UniformMatrix(const char* name, const float* value) {
        glUniformMatrix4fv(glGetUniformLocation(id,name),1, GL_FALSE, value);
    }
}
