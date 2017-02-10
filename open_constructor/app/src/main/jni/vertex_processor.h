#include <tango_3d_reconstruction_api.h>
#include "model_io.h"

namespace mesh_builder {

    class VertexProcessor {
    public:
        VertexProcessor(Tango3DR_Context context, Tango3DR_GridIndex index);
        ~VertexProcessor();
        static void cleanup(tango_gl::StaticMesh* mesh);
        void getMeshWithUV(glm::mat4 world2uv, Tango3DR_CameraCalibration calibration,
                           SingleDynamicMesh* result);
    };
} // namespace mesh_builder
