#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include "data/file3d.h"

struct id3d
{
    int x;
    int y;
    int z;

    bool operator==(id3d v) {
        return (v.x == x) && (v.y == y) && (v.z == z);
    }

    bool operator!=(id3d v) {
        return !(*this==v);
    }
};

bool operator<(const id3d& lhs, const id3d& rhs);

enum LOD{ LOD_ORIG, LOD_HQ, LOD_MQ, LOD_LQ, LOD_COUNT };

#define CULLING_DST 65

namespace oc {
    class GLScene {
    public:
        void Clear();
        void Load(std::vector<oc::Mesh>& input);
        void Render(glm::vec4 camera, glm::vec4 dir, GLint position_param, GLint uv_param);
    private:
        glm::vec3 Dec(glm::vec3 &v, LOD& level);
        std::vector<std::pair<LOD, id3d> > GetVisibility(glm::vec4 camera, glm::vec3 dir);
        bool Valid(glm::vec3 a, glm::vec3 &b);

        std::map<id3d, std::map<std::string, Mesh> > models[LOD_COUNT];
    };
}

#endif
