#ifndef TANGO_SCAN_H
#define TANGO_SCAN_H

#include <tango_3d_reconstruction_api.h>
#include <unordered_map>
#include <vector>
#include "data/dataset.h"
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
        void Clear();
        glm::dmat4 Convert(Tango3DR_Pose pose);
        Tango3DR_Pose Convert(glm::dmat4 pose);
        void CorrectPoses(Dataset dataset, Tango3DR_Trajectory trajectory);
        std::unordered_map<GridIndex, Tango3DR_Mesh*, GridIndexHasher> Data() { return meshes; }
        void Merge(std::vector<std::pair<GridIndex, Tango3DR_Mesh*> > added);
        std::vector<std::pair<GridIndex, Tango3DR_Mesh*> > Process(Tango3DR_ReconstructionContext context,
                                                                   Tango3DR_GridIndexArray *t3dr_updated);

        static Tango3DR_Pose GetPose(Tango3DR_Trajectory trajectory, Dataset dataset, int index, int pose);
        static Tango3DR_PointCloud LoadPointCloud(Dataset dataset, int index);
        static Tango3DR_Pose LoadPose(Dataset dataset, int index, int pose);
        static void SavePointCloud(Dataset dataset, int index, Tango3DR_PointCloud t3dr_depth);
        static void SavePose(Dataset dataset, int index, Tango3DR_Pose t3dr_depth_pose, Tango3DR_Pose t3dr_image_pose);

    private:
        std::unordered_map<GridIndex, Tango3DR_Mesh*, GridIndexHasher> meshes;
    };
}
#endif
