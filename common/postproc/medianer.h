#ifndef POSTPROCESSOR_MEDIANER_H
#define POSTPROCESSOR_MEDIANER_H

#include <data/dataset.h>
#include <data/file3d.h>
#include <editor/rasterizer.h>
#include <gl/camera.h>
#include <gl/glsl.h>

namespace oc {

    enum Pass { PASS_DEPTH, PASS_ERROR, PASS_APPLY, PASS_COUNT };

    class Medianer : Rasterizer {
    public:
        Medianer(std::string path, std::string filename, bool preparePhotos = true);
        ~Medianer();
        int GetPoseCount() { return poseCount; }
        virtual void Process(unsigned long& index, int &x1, int &x2, int &y, glm::dvec3 &z1, glm::dvec3 &z2);
        void RenderPose(int index);
        bool RenderTexture(int index);
    private:
        GLuint Image2GLTexture(Image* img);

        /// Rendering stuff
        std::vector<oc::Mesh> model;
        oc::GLSL* shader;

        oc::Dataset* dataset;  ///< Path to dataset
        float cx, cy, fx, fy;  ///< Calibration
        int poseCount;         ///< Pose count
        int width, height;     ///< Camera dimension

        /// Processing
        unsigned int currentCount;
        double* currentDepth;
        unsigned int currentError;
        Image* currentImage;
        unsigned int currentMesh;
        unsigned int currentPass;
        glm::mat4 currentPose;
    };
}
#endif
