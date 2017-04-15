#include <stdint.h>
#include "gl/glsl.h"
#include "gl/opengl.h"
#include "utils/io.h"

namespace oc {
    GLSL::GLSL(std::vector<std::string> vert, std::vector<std::string> frag) {
        /// convert vertex shader source code
        std::string header = "#version 100\nprecision highp float;\n";
        int size = header.length();
        for (unsigned int i = 0; i < vert.size(); i++)
            size += vert[i].length() + 2;

        char vs[size];
        strcpy(vs, header.c_str());
        for (unsigned int i = 0; i < vert.size(); i++)
        {
            strcat(vs, vert[i].c_str());
            strcat(vs, "\n");
        }

        /// convert fragment shader source code
        size = header.length();
        for (unsigned int i = 0; i < frag.size(); i++)
            size += frag[i].length() + 2;
        char fs[size];
        strcpy(fs, header.c_str());
        for (unsigned int i = 0; i < frag.size(); i++)
        {
            strcat(fs, frag[i].c_str());
            strcat(fs, "\n");
        }

        /// compile shader
        id = initShader(vs, fs);

        /// Attach VBO attributes
        attribute_v_vertex = glGetAttribLocation(id, "v_vertex");
        attribute_v_coord = glGetAttribLocation(id, "v_coord");
        attribute_v_normal = glGetAttribLocation(id, "v_normal");
    }

    GLSL::~GLSL() {
        glDetachShader(id, shader_vp);
        glDetachShader(id, shader_fp);
        glDeleteShader(shader_vp);
        glDeleteShader(shader_fp);
        glUseProgram(0);
        glDeleteProgram(id);
    }

    void GLSL::attrib(unsigned int size) {
        /// apply attributes
        glVertexAttribPointer(attribute_v_vertex, 3, GL_FLOAT, GL_FALSE, 0, ( const void *) 0);
        unsigned int len = 3;
        if (attribute_v_normal != -1)
        {
            glVertexAttribPointer(attribute_v_normal, 3, GL_FLOAT, GL_FALSE, 0, ( const void *) (intptr_t)(size * len));
            len += 3;
        }
        if (attribute_v_coord != -1)
        {
            glVertexAttribPointer(attribute_v_coord, 2, GL_FLOAT, GL_FALSE, 0, ( const void *) (intptr_t)(size * len));
            len += 2;
        }
    }

    void GLSL::attrib(float* vertices, float* normals, float* coords) {
        /// send attributes to GPU
        glVertexAttribPointer(attribute_v_vertex, 3, GL_FLOAT, GL_FALSE, 0, vertices);
        if ((attribute_v_normal != -1) && (normals != 0))
          glVertexAttribPointer(attribute_v_normal, 3, GL_FLOAT, GL_FALSE, 0, normals);
        if ((attribute_v_coord != -1) && (coords != 0))
          glVertexAttribPointer(attribute_v_coord, 2, GL_FLOAT, GL_FALSE, 0, coords);
    }

    void GLSL::bind() {
        /// bind shader
        glUseProgram(id);

        /// set attributes
        glEnableVertexAttribArray(attribute_v_vertex);
        if (attribute_v_normal != -1)
            glEnableVertexAttribArray(attribute_v_normal);
        if (attribute_v_coord != -1)
            glEnableVertexAttribArray(attribute_v_coord);
    }

    unsigned int GLSL::initShader(const char *vs, const char *fs) {
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
            LOGI("GLSL compile log: %s", buffer);
        glCompileShader(shader_fp);
        glGetShaderInfoLog(shader_fp, BUFFER_SIZE, &length, buffer);
        if (length > 0)
            LOGI("GLSL compile log: %s", buffer);

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

    void GLSL::unbind() {
        glUseProgram(0);
    }

    void GLSL::uniformFloat(const char* name, float value) {
        glUniform1f(glGetUniformLocation(id, name), value);
    }

    void GLSL::uniformMatrix(const char* name, float* value) {
        glUniformMatrix4fv(glGetUniformLocation(id,name),1, GL_FALSE, value);
    }
}
