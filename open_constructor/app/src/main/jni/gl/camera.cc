#include "gl/camera.h"

namespace oc {

    void GLCamera::Convert2uv(glm::vec4 &v, glm::mat4 &world2uv, Tango3DR_CameraCalibration &c) {
        v = world2uv * v;
        v.x /= glm::abs(v.w * v.z);
        v.y /= glm::abs(v.w * v.z);
        v.x *= c.fx / (float)c.width;
        v.y *= c.fy / (float)c.height;
        v.x += c.cx / (float)c.width;
        v.y += c.cy / (float)c.height;
    }

    void GLCamera::DecomposeMatrix(const glm::mat4& matrix, glm::vec3* translation,
                                   glm::quat* rotation, glm::vec3* scale) {
        translation->x = matrix[3][0];
        translation->y = matrix[3][1];
        translation->z = matrix[3][2];
        *rotation = glm::quat_cast(matrix);
        scale->x = glm::length(glm::vec3(matrix[0][0], matrix[1][0], matrix[2][0]));
        scale->y = glm::length(glm::vec3(matrix[0][1], matrix[1][1], matrix[2][1]));
        scale->z = glm::length(glm::vec3(matrix[0][2], matrix[1][2], matrix[2][2]));
        if (glm::determinant(matrix) < 0.0)
            scale->x = -scale->x;
    }

    float GLCamera::Diff(const glm::quat &a, const glm::quat &b)
    {
        if (glm::abs(b.x) < 0.005)
            if (glm::abs(b.y) < 0.005)
                if (glm::abs(b.z) < 0.005)
                    if (glm::abs(b.w) - 1 < 0.005)
                        return 0;
        glm::vec3 diff = glm::eulerAngles(glm::inverse(a) * b);
        diff = glm::abs(diff);
        if (diff.x > M_PI)
            diff.x -= M_PI;
        if (diff.y > M_PI)
            diff.y -= M_PI;
        if (diff.z > M_PI)
            diff.z -= M_PI;
        return glm::degrees(glm::max(glm::max(diff.x, diff.y), diff.z));
    }

    Tango3DR_Pose GLCamera::Extract3DRPose(const glm::mat4 &mat) {
        Tango3DR_Pose pose;
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
        DecomposeMatrix(mat, &translation, &rotation, &scale);
        pose.translation[0] = translation[0];
        pose.translation[1] = translation[1];
        pose.translation[2] = translation[2];
        pose.orientation[0] = rotation[0];
        pose.orientation[1] = rotation[1];
        pose.orientation[2] = rotation[2];
        pose.orientation[3] = rotation[3];
        return pose;
    }

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
        DecomposeMatrix(transform, &position, &rotation, &scale);
    }
}
