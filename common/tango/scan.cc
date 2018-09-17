#include "tango/scan.h"

namespace oc {

    bool GridIndex::operator==(const GridIndex &o) const {
        return indices[0] == o.indices[0] && indices[1] == o.indices[1] && indices[2] == o.indices[2];
    }

    void TangoScan::Clear() {
        Tango3DR_Status ret;
        for (std::pair<GridIndex, Tango3DR_Mesh*> p : meshes) {
            ret = Tango3DR_Mesh_destroy(p.second);
            if (ret != TANGO_3DR_SUCCESS)
                exit(EXIT_SUCCESS);
            delete p.second;
        }
        meshes.clear();
    }

    void TangoScan::CorrectPoses(Dataset dataset, Tango3DR_Trajectory trajectory) {
        int count, width, height;
        dataset.GetState(count, width, height);
        std::vector<int> sessions = dataset.GetSessions();
        for (int i = sessions[sessions.size() - 1]; i < count; i++) {
            Tango3DR_Pose image_pose;
            if (Tango3DR_getPoseAtTime(trajectory, dataset.GetPoseTime(i, COLOR_CAMERA), &image_pose) != TANGO_3DR_SUCCESS) {
                exit(EXIT_SUCCESS);
            }
            Tango3DR_Pose depth_pose;
            if (Tango3DR_getPoseAtTime(trajectory, dataset.GetPoseTime(i, DEPTH_CAMERA), &depth_pose) != TANGO_3DR_SUCCESS) {
                exit(EXIT_SUCCESS);
            }

            Tango3DR_Pose t3dr_depth_pose = LoadPose(dataset, i, DEPTH_CAMERA);
            Tango3DR_Pose t3dr_image_pose = LoadPose(dataset, i, COLOR_CAMERA);

            //TODO:convert corrected poses
            SavePose(dataset, i, t3dr_depth_pose, t3dr_image_pose);
        }
    }

    void TangoScan::Merge(std::vector<std::pair<GridIndex, Tango3DR_Mesh*> > added) {
        Tango3DR_Status ret;
        for (std::pair<GridIndex, Tango3DR_Mesh*> p : added) {
            if (meshes.find(p.first) != meshes.end()) {
                ret = Tango3DR_Mesh_destroy(meshes[p.first]);
                if (ret != TANGO_3DR_SUCCESS)
                    exit(EXIT_SUCCESS);
                delete meshes[p.first];
            }
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
                exit(EXIT_SUCCESS);
            output.push_back(pair);
        }
        return output;
    }


    Tango3DR_PointCloud TangoScan::LoadPointCloud(Dataset dataset, int index) {
        int size = 0;
        FILE* file = fopen(dataset.GetFileName(index, ".pcl").c_str(), "r");
        fscanf(file, "%d\n", &size);
        Tango3DR_PointCloud t3dr_depth;
        Tango3DR_PointCloud_init(size, &t3dr_depth);
        for (int j = 0; j < t3dr_depth.num_points; j++) {
            fscanf(file, "%f %f %f %f\n", &t3dr_depth.points[j][0], &t3dr_depth.points[j][1], &t3dr_depth.points[j][2], &t3dr_depth.points[j][3]);
        }
        fclose(file);
        return t3dr_depth;
    }


    Tango3DR_Pose TangoScan::LoadPose(Dataset dataset, int index, int pose) {
        Tango3DR_Pose t3dr_pose = Tango3DR_Pose();
        FILE* file = fopen(dataset.GetFileName(index, ".pos").c_str(), "r");
        for (int i = 0; i <= pose; i++) {
            fscanf(file, "%lf %lf %lf %lf %lf %lf %lf\n", &t3dr_pose.translation[0], &t3dr_pose.translation[1], &t3dr_pose.translation[2],
                   &t3dr_pose.orientation[0], &t3dr_pose.orientation[1], &t3dr_pose.orientation[2], &t3dr_pose.orientation[3]);
        }
        fclose(file);
        return t3dr_pose;
    }

    void TangoScan::SavePointCloud(Dataset dataset, int index, Tango3DR_PointCloud t3dr_depth) {
        FILE* file = fopen(dataset.GetFileName(index, ".pcl").c_str(), "w");
        fprintf(file, "%d\n", t3dr_depth.num_points);
        for (int i = 0; i < t3dr_depth.num_points; i++) {
            fprintf(file, "%f %f %f %f\n", t3dr_depth.points[i][0], t3dr_depth.points[i][1], t3dr_depth.points[i][2], t3dr_depth.points[i][3]);
        }
        fclose(file);
    }

    void TangoScan::SavePose(Dataset dataset, int index, Tango3DR_Pose t3dr_depth_pose, Tango3DR_Pose t3dr_image_pose) {
        FILE* file = fopen(dataset.GetFileName(index, ".pos").c_str(), "w");
        fprintf(file, "%lf %lf %lf %lf %lf %lf %lf\n", t3dr_image_pose.translation[0], t3dr_image_pose.translation[1], t3dr_image_pose.translation[2],
                t3dr_image_pose.orientation[0], t3dr_image_pose.orientation[1], t3dr_image_pose.orientation[2], t3dr_image_pose.orientation[3]);
        fprintf(file, "%lf %lf %lf %lf %lf %lf %lf\n", t3dr_depth_pose.translation[0], t3dr_depth_pose.translation[1], t3dr_depth_pose.translation[2],
                t3dr_depth_pose.orientation[0], t3dr_depth_pose.orientation[1], t3dr_depth_pose.orientation[2], t3dr_depth_pose.orientation[3]);
        fclose(file);
    }
}
