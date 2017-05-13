#ifndef GL_CAMERA_H
#define GL_CAMERA_H

#include <tango_3d_reconstruction_api.h>
#include "gl/opengl.h"

namespace oc {
    class GLCamera {
    public:

        static void DecomposeMatrix(const glm::mat4& matrix, glm::vec3* translation,
                                    glm::quat* rotation, glm::vec3* scale);
        static float Diff(const glm::quat &a, const glm::quat &b);
        static Tango3DR_Pose Extract3DRPose(const glm::mat4 &mat);

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
