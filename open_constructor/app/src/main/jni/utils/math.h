#ifndef UTILS_MATH_H
#define UTILS_MATH_H

#include <glm/glm.hpp>
#include <math.h>
#include <tango_3d_reconstruction_api.h>

namespace oc {
    class Math {

    public:
        static inline void Convert2uv(glm::vec4 &v, glm::mat4 &world2uv,
                                      Tango3DR_CameraCalibration &calibration) {
            v = world2uv * v;
            v.x /= glm::abs(v.w * v.z);
            v.y /= glm::abs(v.w * v.z);
            v.x *= calibration.fx / (float)calibration.width;
            v.y *= calibration.fy / (float)calibration.height;
            v.x += calibration.cx / (float)calibration.width;
            v.y += calibration.cy / (float)calibration.height;
        }

        static inline void DecomposeMatrix(const glm::mat4& transform_mat, glm::vec3* translation,
                                           glm::quat* rotation, glm::vec3* scale) {
            float scale_x = glm::length(
                glm::vec3(transform_mat[0][0], transform_mat[1][0], transform_mat[2][0]));
            float scale_y = glm::length(
                glm::vec3(transform_mat[0][1], transform_mat[1][1], transform_mat[2][1]));
            float scale_z = glm::length(
                glm::vec3(transform_mat[0][2], transform_mat[1][2], transform_mat[2][2]));
            if (glm::determinant(transform_mat) < 0.0)
                scale_x = -scale_x;

            translation->x = transform_mat[3][0];
            translation->y = transform_mat[3][1];
            translation->z = transform_mat[3][2];

            float inverse_scale_x = 1.0 / scale_x;
            float inverse_scale_y = 1.0 / scale_y;
            float inverse_scale_z = 1.0 / scale_z;

            glm::mat4 transform_unscaled = transform_mat;
            transform_unscaled[0][0] *= inverse_scale_x;
            transform_unscaled[1][0] *= inverse_scale_x;
            transform_unscaled[2][0] *= inverse_scale_x;
            transform_unscaled[0][1] *= inverse_scale_y;
            transform_unscaled[1][1] *= inverse_scale_y;
            transform_unscaled[2][1] *= inverse_scale_y;
            transform_unscaled[0][2] *= inverse_scale_z;
            transform_unscaled[1][2] *= inverse_scale_z;
            transform_unscaled[2][2] *= inverse_scale_z;

            *rotation = glm::quat_cast(transform_mat);
            scale->x = scale_x;
            scale->y = scale_y;
            scale->z = scale_z;
        }

        static inline float Diff(const glm::quat &a, const glm::quat &b)
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

        static inline Tango3DR_Pose Extract3DRPose(const glm::mat4 &mat) {
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
    };
};

#endif
