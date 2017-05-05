#ifndef TANGO_SCAN_H
#define TANGO_SCAN_H

#include <tango_3d_reconstruction_api.h>
#include <unordered_map>
#include <vector>
#include "data/mesh.h"

namespace oc {

    struct GridIndex {
        Tango3DR_GridIndex indices;

        bool operator==(const GridIndex &other) const;
    };

    struct GridIndexHasher {
        std::size_t operator()(const oc::GridIndex &index) const {
            std::size_t val = std::hash<int>()(index.indices[0]);
            val = hash_combine(val, std::hash<int>()(index.indices[1]));
            val = hash_combine(val, std::hash<int>()(index.indices[2]));
            return val;
        }

        static std::size_t hash_combine(std::size_t val1, std::size_t val2) {
            return (val1 << 1) ^ val2;
        }
    };

    class TangoScan {
    public:
        TangoScan();
        ~TangoScan();
        void Clear();
        std::vector<SingleDynamicMesh*> MeshUpdate(Tango3DR_ReconstructionContext context,
                                                   Tango3DR_GridIndexArray *t3dr_updated);

    private:
        std::unordered_map<GridIndex, SingleDynamicMesh*, GridIndexHasher>* meshes;
    };
}
#endif
