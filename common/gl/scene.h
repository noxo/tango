#ifndef SCENE_H
#define SCENE_H

#include <set>
#include <vector>
#include "data/file3d.h"

struct Plane {
    float a, b, c, d;
};

struct Frustum {
    Plane l,r,b,t,n,f;

    Frustum(float* mat) {
        // Left Plane
        // col4 + col1
        l.a = mat[3] + mat[0];
        l.b = mat[7] + mat[4];
        l.c = mat[11] + mat[8];
        l.d = mat[15] + mat[12];

        // Right Plane
        // col4 - col1
        r.a = mat[3] - mat[0];
        r.b = mat[7] - mat[4];
        r.c = mat[11] - mat[8];
        r.d = mat[15] - mat[12];

        // Bottom Plane
        // col4 + col2
        b.a = mat[3] + mat[1];
        b.b = mat[7] + mat[5];
        b.c = mat[11] + mat[9];
        b.d = mat[15] + mat[13];

        // Top Plane
        // col4 - col2
        t.a = mat[3] - mat[1];
        t.b = mat[7] - mat[5];
        t.c = mat[11] - mat[9];
        t.d = mat[15] - mat[13];

        // Near Plane
        // col4 + col3
        n.a = mat[3] + mat[2];
        n.b = mat[7] + mat[6];
        n.c = mat[11] + mat[10];
        n.d = mat[15] + mat[14];

        // Far Plane
        // col4 - col3
        f.a = mat[3] - mat[2];
        f.b = mat[7] - mat[6];
        f.c = mat[11] - mat[10];
        f.d = mat[15] - mat[14];
    }
};

struct RenderNode {
    std::vector<glm::vec3> aabb;
    float distance;
    std::vector<oc::Mesh*> meshes;
};

namespace oc {
    class GLScene {
    public:
        GLScene();
        ~GLScene();
        unsigned int AmountOfBinding();
        unsigned long AmountOfVertices();
        void Clear();
        void Load(std::vector<oc::Mesh>& input);
        void Process();
        void Render(GLint position_param, GLint uv_param);
        void UpdateVisibility(glm::mat4 mvp, glm::vec4 translate);
    private:
        int BasicTest(glm::mat4& mvp, glm::vec4& translate);
        float DistanceToAABB(glm::vec3 p);
        std::vector<RenderNode> GetRenderable();
        glm::vec4 GetTransform(int i);
        bool IntersectAABB(const Frustum &f);
        void PatchAABB();
        void ProcessRecursive();
        void SetVisibility(bool on, bool childs);
        void UpdateAABB(bool x, bool y, bool z);

        glm::vec3 aabb[2];
        glm::vec4 camera;
        GLScene* child[8];
        std::vector<Mesh> models;
        float size;
    };
}

#endif
