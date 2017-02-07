#include <tango_3d_reconstruction_api.h>
#include "model_io.h"

namespace mesh_builder {

    class VertexProcessor {
    public:
        VertexProcessor(Tango3DR_Context context, Tango3DR_GridIndex index);
        ~VertexProcessor();
        void collideMesh(SingleDynamicMesh* slave, glm::mat4 to2d, Tango3DR_CameraCalibration calibration);
        void getDebugMesh(tango_gl::StaticMesh* result);
        void getMeshWithUV(glm::mat4 world2uv, Tango3DR_CameraCalibration calibration,
                           SingleDynamicMesh* result);
    private:
        bool intersect(glm::vec2 const& p0, glm::vec2 const& p1, glm::vec2 const& p2, glm::vec2 const& p3);

        std::vector<std::map<unsigned int, bool> > topology;
    };
} // namespace mesh_builder
