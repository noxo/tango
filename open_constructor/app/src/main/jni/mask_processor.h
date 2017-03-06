#ifndef MASK_PROCESSOR_H
#define MASK_PROCESSOR_H

#include <tango_3d_reconstruction_api.h>
#include "math_utils.h"
#include "model_io.h"

namespace mesh_builder {

    class MaskProcessor {
    public:
        MaskProcessor(int w, int h);
        MaskProcessor(int w, int h, glm::mat4 matrix, Tango3DR_CameraCalibration calib);
        ~MaskProcessor();

        void AddContext(Tango3DR_Context context, Tango3DR_GridIndexArray* indices);
        void AddPointCloud(TangoPointCloud *front_cloud_, glm::mat4 matrix);
        void AddUVs(std::vector<SingleDynamicMesh*> meshes);
        void AddVertices(std::vector<SingleDynamicMesh*> meshes);
        void DetectEdges();
        double GetMask(int x, int y, int r = 2, bool minim = true);
        void MaskMesh(SingleDynamicMesh* mesh, bool processFront);

        double* GetBuffer() { return buffer; }
    private:
        bool Line(int x1, int y1, int x2, int y2, double z1, double z2,
                  std::pair<int, double>* fillCache);
        bool Test(double p, double q, double &t1, double &t2);
        int Triangle(glm::vec3 &a, glm::vec3 &b, glm::vec3 &c);
        void Triangles(float* vertices, unsigned long size);

        double* buffer;
        Tango3DR_CameraCalibration calibration;
        glm::vec4 camera;
        bool draw;
        bool exact;
        int viewport_width, viewport_height;
        glm::mat4 world2uv;
    };
} // namespace mesh_builder

#endif