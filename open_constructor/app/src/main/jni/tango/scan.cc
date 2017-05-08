#include "tango/scan.h"

namespace oc {

    const int kInitialVertexCount = 30;
    const int kInitialFaceCount = 10;
    const float kGrowthFactor = 1.1f;

    bool GridIndex::operator==(const GridIndex &o) const {
        return indices[0] == o.indices[0] && indices[1] == o.indices[1] && indices[2] == o.indices[2];
    }

    void TangoScan::Clear() {
        meshes.clear();
    }

    void TangoScan::Merge(std::vector<std::pair<GridIndex, Tango3DR_Mesh*> > added) {
        for (std::pair<GridIndex, Tango3DR_Mesh*> p : added) {
            if (meshes.find(p.first) != meshes.end())
                delete meshes[p.first];
            meshes[p.first] = p.second;
        }
    }

    std::vector<std::pair<GridIndex, Tango3DR_Mesh*> > TangoScan::Process(Tango3DR_ReconstructionContext context,
                                                                          Tango3DR_GridIndexArray *t3dr_updated) {
        Tango3DR_Status ret;
        std::pair<GridIndex, Tango3DR_Mesh*> pair;
        std::vector<std::pair<GridIndex, Tango3DR_Mesh*> > output;
        for (unsigned long it = 0; it < t3dr_updated->num_indices; ++it) {
            pair.first.indices[0] = t3dr_updated->indices[it][0];
            pair.first.indices[1] = t3dr_updated->indices[it][1];
            pair.first.indices[2] = t3dr_updated->indices[it][2];

            pair.second = new Tango3DR_Mesh();
            ret = Tango3DR_extractMeshSegment(context, t3dr_updated->indices[it], pair.second);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            output.push_back(pair);
        }
        return output;
    }
}
