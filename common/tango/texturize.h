#ifndef TANGO_TEXTURIZE_H
#define TANGO_TEXTURIZE_H

#include <tango_3d_reconstruction_api.h>
#include "data/dataset.h"
#include "gl/opengl.h"

namespace oc {

    class TangoTexturize {
    public:
        TangoTexturize();
        void Add(Tango3DR_ImageBuffer t3dr_image, std::vector<glm::mat4> matrix, Dataset dataset, double depthTimestamp);
        void ApplyFrames(Dataset dataset);
        void Callback(int progress);
        void Clear(Dataset dataset);
        void GenerateTrajectory(std::string tangoDataset);
        std::string GetEvent() { return event; }
        Image* GetLatestImage(Dataset dataset);
        int GetLatestIndex(Dataset dataset);
        Tango3DR_Pose GetPose(Dataset dataset, int index, double timestamp, bool camera);
        std::vector<glm::mat4> GetLatestPose(Dataset dataset);
        Tango3DR_Trajectory* GetTrajectory() { return trajectory; }
        bool Init(std::string filename, Tango3DR_CameraCalibration* camera);
        bool Init(Tango3DR_ReconstructionContext context, Tango3DR_CameraCalibration* camera);
        bool IsPoseCorrected(double timestamp);
        void Process(std::string filename);
        void SetEvent(std::string value) { event = value; }
        void SetResolution(float value) { resolution = value; }

    private:
        void CreateContext(bool finalize, Tango3DR_Mesh* mesh, Tango3DR_CameraCalibration* camera);

        int poses;
        float resolution;
        std::string event;
        Tango3DR_TexturingContext context;
        Tango3DR_Trajectory* trajectory;
        int width, height;
    };
}
#endif
