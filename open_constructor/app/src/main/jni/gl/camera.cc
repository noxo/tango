#include "gl/camera.h"

namespace oc {

    glm::mat4 GLCamera::GetTransformation() const {
        glm::mat4 transform = glm::scale(glm::mat4_cast(rotation), scale);
        transform[3][0] = position.x;
        transform[3][1] = position.y;
        transform[3][2] = position.z;
        return transform;
    }

    glm::mat4 GLCamera::GetView() const {
        return glm::inverse(GetTransformation());
    }

    void GLCamera::SetTransformation(const glm::mat4& transform) {
        Math::DecomposeMatrix(transform, &position, &rotation, &scale);
    }
}
