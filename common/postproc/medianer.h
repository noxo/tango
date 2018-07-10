#ifndef POSTPROCESSOR_MEDIANER_H
#define POSTPROCESSOR_MEDIANER_H

#include <data/dataset.h>
#include <data/file3d.h>
#include <gl/camera.h>
#include <gl/renderer.h>

namespace oc {

    class Medianer {
    public:
        Medianer(std::string path, std::string filename, bool renderToFileSupport);
        ~Medianer();
        void RenderPose(int index);
        void RenderPose(int index, std::string filename);
    private:
        GLuint Image2GLTexture(oc::Image* img);

        /// Rendering stuff
        std::vector<oc::Mesh> model;
        oc::GLRenderer renderer;
        oc::GLSL* shader;

        oc::Dataset* dataset;  ///< Path to dataset
        float cx, cy, fx, fy;  ///< Calibration
        int poseCount;         ///< Pose count
        int width, height;     ///< Camera dimension
    };
}
#endif
