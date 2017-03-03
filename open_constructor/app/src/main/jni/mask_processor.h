#ifndef OPEN_CONSTRUCTOR_MASK_PROCESSOR_H
#define OPEN_CONSTRUCTOR_MASK_PROCESSOR_H

#include <tango_3d_reconstruction_api.h>
#include "math_utils.h"
#include "model_io.h"

namespace mesh_builder {

    class MaskProcessor {
    public:
        MaskProcessor(std::vector<SingleDynamicMesh*> meshes, int w, int h);
        MaskProcessor(Tango3DR_Context context, int w, int h, Tango3DR_GridIndexArray* indices,
                      glm::mat4 matrix, Tango3DR_CameraCalibration calib);
        MaskProcessor(TangoPointCloud* front_cloud_, glm::mat4 point_cloud_matrix_, int w, int h,
                      glm::mat4 matrix, Tango3DR_CameraCalibration calib);
        ~MaskProcessor();
        void CleanUp(std::vector<glm::vec3>* vertices);
        double GetMask(int x, int y, int r = 2, bool minim = true);
        void MaskMesh(SingleDynamicMesh* mesh, bool processFront);

        double* GetBuffer() { return buffer; }
        std::vector<glm::vec3>* getClearedVertices() { return &toClear; }
    private:
        bool Line(int x1, int y1, int x2, int y2, double z1, double z2,
                  std::pair<int, double>* fillCache);
        bool Test(double p, double q, double &t1, double &t2);
        int Triangle(glm::vec3 &a, glm::vec3 &b, glm::vec3 &c);
        void Triangles(float* vertices, unsigned long size);

        double* buffer;
        Tango3DR_CameraCalibration calibration;
        glm::vec4 camera;
        bool depth_test;
        bool draw;
        std::vector<glm::vec3> toClear;
        int viewport_width, viewport_height;
        glm::mat4 world2uv;
    };
} // namespace mesh_builder

#endif