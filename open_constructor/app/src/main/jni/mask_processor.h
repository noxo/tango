#include <tango_3d_reconstruction_api.h>
#include "model_io.h"

namespace mesh_builder {

    class MaskProcessor {
    public:
        MaskProcessor(Tango3DR_Context context, Tango3DR_GridIndex index, int w, int h,
                      glm::mat4 matrix, Tango3DR_CameraCalibration calib);
        ~MaskProcessor();
        void maskMesh(SingleDynamicMesh* mesh, bool inverse);
    private:
        bool line(int x1, int y1, int x2, int y2, glm::vec3 z1, glm::vec3 z2,
                  std::pair<int, glm::vec3>* fillCache);
        bool test(double p, double q, float &t1, float &t2);
        void triangles(float* vertices, unsigned long size);

        glm::vec3* buffer;
        Tango3DR_CameraCalibration calibration;
        int viewport_width, viewport_height;
        glm::mat4 world2uv;
    };
} // namespace mesh_builder
