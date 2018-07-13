#ifndef POSTPROCESSOR_MEDIANER_H
#define POSTPROCESSOR_MEDIANER_H

#include <data/dataset.h>
#include <data/file3d.h>
#include <editor/rasterizer.h>
#include <gl/camera.h>
#include <gl/renderer.h>

namespace oc {

    class Medianer : Rasterizer {
    public:
        Medianer(std::string path, std::string filename, bool renderToFileSupport);
        ~Medianer();
        virtual void Process(unsigned long& index, int &x1, int &x2, int &y, glm::dvec3 &z1, glm::dvec3 &z2);
        void RenderPose(int index);
        void RenderPose(int index, std::string filename);
        void RenderTexture();
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

        /// Texturing
        double* currentDepth;
        Image* currentImage;
        unsigned int currentMesh;
        glm::mat4 currentPose;
        bool writeTexture;
    };
}
#endif
