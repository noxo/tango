#ifndef VERTEX_PROCESSOR_H
#define VERTEX_PROCESSOR_H

#include <tango_3d_reconstruction_api.h>
#include "model_io.h"

namespace mesh_builder {

    class VertexProcessor {
    public:
        VertexProcessor(Tango3DR_Context context, Tango3DR_GridIndex index);
        ~VertexProcessor();
        static void Cleanup(tango_gl::StaticMesh* mesh);
        void GetMeshWithUV(glm::mat4 world2uv, Tango3DR_CameraCalibration calibration,
                           SingleDynamicMesh* result);
    };
} // namespace mesh_builder

#endif