#include <tango_3d_reconstruction_api.h>
#include "model_io.h"

namespace mesh_builder {

    class MaskProcessor {
    public:
        MaskProcessor(Tango3DR_Context context, Tango3DR_GridIndex index);
        ~MaskProcessor();
    };
} // namespace mesh_builder
