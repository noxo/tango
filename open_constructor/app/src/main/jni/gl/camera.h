#ifndef GL_CAMERA_H
#define GL_CAMERA_H

#include "utils/math.h"

namespace oc {
    class GLCamera {
    public:

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
