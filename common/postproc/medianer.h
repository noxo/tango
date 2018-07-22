#ifndef POSTPROCESSOR_MEDIANER_H
#define POSTPROCESSOR_MEDIANER_H

#include <data/dataset.h>
#include <data/file3d.h>
#include <editor/rasterizer.h>
#include <gl/camera.h>
#include <gl/renderer.h>

namespace oc {

    enum Pass { PASS_DEPTH, PASS_TEXTURE, PASS_COUNT };

    class Medianer : Rasterizer {
    public:
        Medianer(std::string path, std::string filename, bool renderToFileSupport);
        ~Medianer();
        int GetPoseCount() { return poseCount; }
        virtual void Process(unsigned long& index, int &x1, int &x2, int &y, glm::dvec3 &z1, glm::dvec3 &z2);
        void RenderDiff(int index);
        void RenderPose(int index, bool diff = false);
        void RenderPose(int index, std::string filename);
        void RenderTexture();
        void RenderTexture(int index, bool handleDepth = true);
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
        unsigned int currentPass;
        glm::mat4 currentPose;
        unsigned int pixelCount;
        glm::vec4 pixelSum;
    };
}
#endif
