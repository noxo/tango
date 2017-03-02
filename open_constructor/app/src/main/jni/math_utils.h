#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <glm/glm.hpp>
#include <math.h>
#include <tango_3d_reconstruction_api.h>
#include <tango-gl/util.h>

class Math {

public:
    static inline void convert2uv(glm::vec4 &v, glm::mat4 &world2uv,
                                  Tango3DR_CameraCalibration &calibration) {
        v = world2uv * v;
        v.x /= glm::abs(v.w * v.z);
        v.y /= glm::abs(v.w * v.z);
        v.x *= calibration.fx / (float)calibration.width;
        v.y *= calibration.fy / (float)calibration.height;
        v.x += calibration.cx / (float)calibration.width;
        v.y += calibration.cy / (float)calibration.height;
    }

    static inline float diff(const glm::quat &a, const glm::quat &b)
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

    static inline Tango3DR_Pose extract3DRPose(const glm::mat4 &mat) {
        Tango3DR_Pose pose;
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
        tango_gl::util::DecomposeMatrix(mat, &translation, &rotation, &scale);
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

#endif //MATH_UTILS_H
