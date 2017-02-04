#include <tango_3d_reconstruction_api.h>
#include "model_io.h"

namespace mesh_builder {

    class VertexProcessor {
    public:
        VertexProcessor(Tango3DR_Context context, Tango3DR_GridIndex index);
        ~VertexProcessor();
        void getMeshWithUV(glm::mat4 world2uv, Tango3DR_CameraCalibration calibration,
                           SingleDynamicMesh* result);
    private:
        std::vector<std::map<unsigned int, bool> > topology;
    };
} // namespace mesh_builder
