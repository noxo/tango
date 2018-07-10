#ifndef DATA_ARRAY3D_H
#define DATA_ARRAY3D_H

#include <map>
#include <vector>
#include <gl/opengl.h>

namespace oc {

class Array3d {
public:
    Array3d(bool emptyValue);
    void Clear();
    std::map<int, bool> Get(int x, int z);
    bool Get(int x, int y, int z);
    std::vector<std::pair<int, int>> GetAllKeys();
    void Line3D(double x1, double y1, double z1, double x2, double y2, double z2,
                std::vector<double> *output, double& step, bool value);
    void Set(int x, int y, int z, bool value);
    void Unpack(int minx, int minz, int maxx, int maxz, double scale, std::vector<glm::vec3>& coords);
private:
    void Compress(std::pair<int, int> key);
    void Unpack(std::map<std::pair<int, int>, std::vector<int> >::const_iterator it,
                float scale, std::vector<glm::vec3>& coords, int xp, int zp);

    std::map<std::pair<int, int>, std::vector<int> > data;
    bool defaultValue;
};
}

#endif
