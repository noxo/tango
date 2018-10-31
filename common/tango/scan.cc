#include "tango/scan.h"
#include <glm/gtx/quaternion.hpp>

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

    glm::dmat4 TangoScan::Convert(Tango3DR_Pose pose) {
        glm::dquat rotation;
        rotation.x = pose.orientation[0];
        rotation.y = pose.orientation[1];
        rotation.z = pose.orientation[2];
        rotation.w = pose.orientation[3];
        glm::dmat4 output = glm::mat4_cast(rotation);
        output[3][0] = pose.translation[0];
        output[3][1] = pose.translation[1];
        output[3][2] = pose.translation[2];
        return output;
    }

    Tango3DR_Pose TangoScan::Convert(glm::dmat4 pose) {
        Tango3DR_Pose output;
        glm::dquat rotation = glm::quat_cast(pose);
        output.translation[0] = pose[3][0];
        output.translation[1] = pose[3][1];
        output.translation[2] = pose[3][2];
        output.orientation[0] = rotation[0];
        output.orientation[1] = rotation[1];
        output.orientation[2] = rotation[2];
        output.orientation[3] = rotation[3];
        return output;
    }

    void TangoScan::CorrectPoses(Dataset dataset, Tango3DR_Trajectory trajectory) {
        int count, width, height;
        dataset.GetState(count, width, height);
        std::vector<int> sessions = dataset.GetSessions();
        int firstPose = sessions[sessions.size() - 1];
        Tango3DR_Pose color, depth;
        dataset.GetPose(firstPose, COLOR_CAMERA, color.translation, color.orientation);
        dataset.GetPose(firstPose, DEPTH_CAMERA, depth.translation, depth.orientation);
        glm::dmat4 trImage = Convert(color);
        glm::dmat4 trDepth = Convert(depth);
        trImage = trImage * glm::inverse(Convert(GetPose(trajectory, dataset, firstPose, COLOR_CAMERA)));
        trDepth = trDepth * glm::inverse(Convert(GetPose(trajectory, dataset, firstPose, DEPTH_CAMERA)));

        //correct poses
        for (int i = firstPose; i < count; i++) {
            Tango3DR_Pose t3dr_image_pose = GetPose(trajectory, dataset, i, COLOR_CAMERA);
            Tango3DR_Pose t3dr_depth_pose = GetPose(trajectory, dataset, i, DEPTH_CAMERA);
            t3dr_image_pose = Convert(trImage * Convert(t3dr_image_pose));
            t3dr_depth_pose = Convert(trDepth * Convert(t3dr_depth_pose));
            dataset.WritePose(i, t3dr_image_pose.translation, t3dr_image_pose.orientation,
                              t3dr_depth_pose.translation, t3dr_depth_pose.orientation,
                              dataset.GetPoseTime(i, COLOR_CAMERA),
                              dataset.GetPoseTime(i, DEPTH_CAMERA));
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

    Tango3DR_Pose TangoScan::GetPose(Tango3DR_Trajectory trajectory, Dataset dataset, int index, int pose) {
        Tango3DR_Pose t3dr_pose;
        if (Tango3DR_getPoseAtTime(trajectory, dataset.GetPoseTime(index, pose), &t3dr_pose) != TANGO_3DR_SUCCESS) {
            dataset.GetPose(index, pose, t3dr_pose.translation, t3dr_pose.orientation);
            return t3dr_pose;
        }
        glm::dquat quat;
        quat.x = t3dr_pose.orientation[0];
        quat.y = t3dr_pose.orientation[2];
        quat.z =-t3dr_pose.orientation[1];
        quat.w = t3dr_pose.orientation[3];
        if (pose == COLOR_CAMERA) {
            quat = glm::rotate(quat, glm::radians(180.0), glm::dvec3(0, 0, 1));
            quat = glm::rotate(quat, glm::radians(270.0), glm::dvec3(1, 0, 0));
            if (buggyDevice) {
                quat.x *= -1.0;
                quat.y *= -1.0;
            }
        } else if (pose == DEPTH_CAMERA) {
            quat = glm::rotate(quat, glm::radians(90.0), glm::dvec3(1, 0, 0));
        }
        Tango3DR_Pose t3dr_fixed_pose;
        t3dr_fixed_pose.orientation[0] = quat.x;
        t3dr_fixed_pose.orientation[1] = quat.y;
        t3dr_fixed_pose.orientation[2] = quat.z;
        t3dr_fixed_pose.orientation[3] = quat.w;
        t3dr_fixed_pose.translation[0] = t3dr_pose.translation[0];
        t3dr_fixed_pose.translation[1] = t3dr_pose.translation[2];
        t3dr_fixed_pose.translation[2] = -t3dr_pose.translation[1];
        if (buggyDevice) {
            if (pose == COLOR_CAMERA) {
                t3dr_fixed_pose.translation[0] *= -1.0;
                t3dr_fixed_pose.translation[1] *= -1.0;
            }
            glm::dmat4 temp = Convert(t3dr_fixed_pose);
            temp = glm::rotate(temp, glm::radians(90.0), glm::dvec3(0, 0, 1));
            t3dr_fixed_pose = Convert(temp);
        }
        return t3dr_fixed_pose;
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

    void TangoScan::SavePointCloud(Dataset dataset, int index, Tango3DR_PointCloud t3dr_depth) {
        FILE* file = fopen(dataset.GetFileName(index, ".pcl").c_str(), "w");
        fprintf(file, "%d\n", t3dr_depth.num_points);
        for (int i = 0; i < t3dr_depth.num_points; i++) {
            fprintf(file, "%f %f %f %f\n", t3dr_depth.points[i][0], t3dr_depth.points[i][1], t3dr_depth.points[i][2], t3dr_depth.points[i][3]);
        }
        fclose(file);
    }
}
