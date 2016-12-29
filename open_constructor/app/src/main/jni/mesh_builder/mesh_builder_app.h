#ifndef MESH_BUILDER_MESH_BUILDER_APP_H_
#define MESH_BUILDER_MESH_BUILDER_APP_H_

#include <jni.h>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <string>
#include <vector>

#include <tango_client_api.h>  // NOLINT
#include <tango-gl/tango-gl.h>
#include <tango-gl/util.h>
#include <tango_3d_reconstruction_api.h>
#include <tango_support_api.h>

#include "mesh_builder/scene.h"

#define THREAD_COUNT 8

namespace mesh_builder {

    struct GridIndex {
        Tango3DR_GridIndex indices;

        bool operator==(const GridIndex &other) const;
    };

    struct GridIndexHasher {
        std::size_t operator()(const mesh_builder::GridIndex &index) const {
            std::size_t val = std::hash<int>()(index.indices[0]);
            val = hash_combine(val, std::hash<int>()(index.indices[1]));
            val = hash_combine(val, std::hash<int>()(index.indices[2]));
            return val;
        }

        static std::size_t hash_combine(std::size_t val1, std::size_t val2) {
            return (val1 << 1) ^ val2;
        }
    };

    class MeshBuilderApp {
    public:
        MeshBuilderApp();
        ~MeshBuilderApp();
        void ActivityCtor(bool t3dr_is_running);
        void OnCreate(JNIEnv *env, jobject caller_activity);
        void OnPause();
        void OnTangoServiceConnected(JNIEnv *env, jobject binder);
        void onPointCloudAvailable(const TangoPointCloud *point_cloud);
        void onFrameAvailable(TangoCameraId id, const TangoImageBuffer *buffer);
        void OnSurfaceCreated();
        void OnSurfaceChanged(int width, int height);
        void OnDrawFrame();
        void OnToggleButtonClicked(bool t3dr_is_running);
        void OnClearButtonClicked();
        void TangoSetup3DR(double res, double dmin, double dmax);
        void Save(std::string filename);

    private:
        void TangoSetupConfig();
        void TangoConnectCallbacks();
        void TangoConnect();
        void TangoDisconnect();
        void DeleteResources();
        static void* Process(void *ptr);

        glm::mat4 start_service_T_device_;
        Tango3DR_Context t3dr_context_;
        bool t3dr_is_running_;
        Tango3DR_CameraCalibration t3dr_intrinsics_;
        bool point_cloud_available_;
        TangoSupportPointCloudManager *point_cloud_manager_;
        TangoPointCloud *front_cloud_;
        glm::mat4 point_cloud_matrix_;
        std::mutex add_mutex_;
        std::mutex binder_mutex_;
        std::mutex process_mutex_;
        std::mutex render_mutex_;
        Scene main_scene_;
        TangoConfig tango_config_;
        std::vector<GridIndex> updated_indices_binder_thread_;
        std::unordered_map<GridIndex, std::shared_ptr<SingleDynamicMesh>, GridIndexHasher> meshes_;
        bool threadDone[THREAD_COUNT];
        pthread_t threadId[THREAD_COUNT];
        std::mutex threadMutex[THREAD_COUNT];
    };
}  // namespace mesh_builder

#endif  // MESH_BUILDER_MESH_BUILDER_APP_H_
