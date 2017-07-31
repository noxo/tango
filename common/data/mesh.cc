#include "data/mesh.h"

namespace oc {

    Mesh::Mesh() : aabbUpdate(0), image(NULL), imageOwner(true) {}

    void Mesh::Destroy() {
        if (image && imageOwner) {
            delete image;
            image = 0;
        }
    }

    float Mesh::GetFloorLevel(glm::vec3 pos) {
        //detect collision with mesh boundary
        if (aabbUpdate != vertices.size()) {
            aabbMin = glm::vec3(INT_MAX, INT_MAX, INT_MAX);
            aabbMax = glm::vec3(INT_MIN, INT_MIN, INT_MIN);
            aabbUpdate = vertices.size();
            for (glm::vec3& v : vertices)
                UpdateAABB(v, aabbMin, aabbMax);
        }
        if (!IsInAABB(pos, aabbMin, aabbMax))
            return INT_MIN;

        //detect collision with triangle boundary
        glm::vec3 min, max;
        std::vector<Triangle> colliding;
        for (unsigned int i = 0; i < vertices.size(); i += 3) {
            min = vertices[i];
            max = vertices[i];
            UpdateAABB(vertices[i + 1], min, max);
            UpdateAABB(vertices[i + 2], min, max);
            if (IsInAABB(pos, min, max)) {
                Triangle t;
                t.a = vertices[i + 0];
                t.b = vertices[i + 1];
                t.c = vertices[i + 2];
                colliding.push_back(t);
            }
        }
        if (colliding.empty())
            return INT_MIN;

        //return some approximated value
        double output = INT_MAX;
        double value;
        for (Triangle t : colliding) {
            value = t.a.y + t.b.y + t.c.y;
            if (output > value)
                output = value;
        }
        return (float) (output / 3.0);
    }

    bool Mesh::IsInAABB(glm::vec3 &p, glm::vec3 &min, glm::vec3 &max) {
        return !((p.x < min.x) || (p.z < min.z) || (p.x > max.x) || (p.z > max.z));
    }

    void Mesh::UpdateAABB(glm::vec3& p, glm::vec3& min, glm::vec3& max) {
        if (min.x > p.x)
            min.x = p.x;
        if (min.z > p.z)
            min.z = p.z;
        if (max.x < p.x)
            max.x = p.x;
        if (max.z < p.z)
            max.z = p.z;
    }
}
