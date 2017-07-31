#ifndef GL_CAMERA_H
#define GL_CAMERA_H

#include "gl/opengl.h"

namespace oc {
    class GLCamera {
    public:

        static void DecomposeMatrix(const glm::mat4& matrix, glm::vec3* translation,
                                    glm::quat* rotation, glm::vec3* scale);
        static float Diff(glm::vec3& pa, glm::vec3& pb, glm::quat &a, glm::quat &b);

        glm::mat4 GetTransformation() const;
        glm::mat4 GetView() const;
        void SetTransformation(const glm::mat4& transform);

        glm::mat4 projection;
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
    };
}
#endif
