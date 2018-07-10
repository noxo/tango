#include "data/array3d.h"

#define ARRAY3D_MINY -65536

int edges_index[6][2] = { { 0, 1 }, { 1, 2 }, { 2, 0 }, { 0, 3 }, { 1, 3 }, { 2, 3 } };
int offset[3][8] = { { 0, 1, 1, 0, 0, 1, 1, 0}, { 0, 0, 1, 1, 0, 0, 1, 1},  { 0, 0, 0, 0, 1, 1, 1, 1} };
int ths[6][4] = { {0, 5, 1, 6}, {0, 1, 2, 6}, {0, 2, 3, 6}, {0, 3, 7, 6}, {0, 7, 4, 6}, {0, 4, 5, 6} };
int thTriangles[16][7] = {
        { -1, -1, -1, -1, -1, -1, -1 }, { 0, 3, 2, -1, -1, -1, -1 },
        { 0, 1, 4, -1, -1, -1, -1 }, { 1, 4, 2, 4, 3, 2, -1 },
        { 1, 2, 5, -1, -1, -1, -1 }, { 0, 3, 5, 0, 5, 1, -1 },
        { 0, 2, 5, 0, 5, 4, -1 }, { 5, 4, 3, -1, -1, -1, -1 },
        { 3, 4, 5, -1, -1, -1, -1 }, { 4, 5, 0, 5, 2, 0, -1 },
        { 1, 5, 0, 5, 3, 0, -1 }, { 5, 2, 1, -1, -1, -1, -1 },
        { 2, 3, 4, 2, 4, 1, -1 }, { 4, 1, 0, -1, -1, -1, -1 },
        { 2, 3, 0, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1, -1, -1 }
};

namespace oc {

Array3d::Array3d(bool emptyValue) {
    defaultValue = emptyValue;
}

void Array3d::Clear() {
    data.clear();
}

void Array3d::Compress(std::pair<int, int> key) {
    int size;
    for (unsigned int i = 0; i < data[key].size(); i++) {
        if (data[key][i] == 0)
        {
            i++;
            size = data[key][i];
            data[key].erase(data[key].begin() + i);
            i--;
            data[key].erase(data[key].begin() + i);
            i--;
            data[key][i] += size;
        }
    }
}

std::map<int, bool> Array3d::Get(int x, int z) {
    std::map<int, bool> value;
    std::pair<int, int> key;
    key.first = x;
    key.second = z;
    if (data.find(key) != data.end()) {
        int fy = ARRAY3D_MINY;
        bool output = defaultValue;
        for (std::vector<int>::const_iterator it = data[key].begin(); it != data[key].end(); ++it) {
            if (output != defaultValue) {
                for (int y = fy; y < fy + *it; y++)
                    value[y] = output;
            }
            fy += *it;
            output = !output;
        }
    }
    return value;
}

bool Array3d::Get(int x, int y, int z) {
    std::pair<int, int> key;
    key.first = x;
    key.second = z;
    if (data.find(key) == data.end())
        return defaultValue;
    int fy = ARRAY3D_MINY;
    bool output = defaultValue;
    for (std::vector<int>::const_iterator it = data[key].begin(); it != data[key].end(); ++it) {
        if ((fy <= y) && (fy + *it > y))
            return output;
        fy += *it;
        output = !output;
    }
    return defaultValue;
}

std::vector<std::pair<int, int>> Array3d::GetAllKeys() {
    std::vector<std::pair<int, int> > output;
    std::map<std::pair<int, int>, std::vector<int> >::const_iterator it;
    for (it = data.begin(); it != data.end(); ++it) {
        output.push_back(it->first);
    }
    return output;
}

//https://gist.github.com/yamamushi/5823518
void Array3d::Line3D(double x1, double y1, double z1, double x2, double y2, double z2,
                     std::vector<double> *output, double& step, bool value) {

    double err_1 = 0;
    double err_2 = 0;
    double x = x1;
    double y = y1;
    double z = z1;
    double dx = fabs(x2 - x1);
    double dy = fabs(y2 - y1);
    double dz = fabs(z2 - z1);
    double x_inc = (x2 < x1) ? -step : step;
    double y_inc = (y2 < y1) ? -step : step;
    double z_inc = (z2 < z1) ? -step : step;

    if ((dx >= dy) && (dx >= dz)) {
        while (fabs(x - x2) > step) {
            if(output) {
                output->push_back(x);
                output->push_back(y);
                output->push_back(z);
            } else {
                Set(x / step, y / step, z / step, value);
            }
            if (err_1 > 0) {
                y += y_inc;
                err_1 -= dx;
            }
            if (err_2 > 0) {
                z += z_inc;
                err_2 -= dx;
            }
            err_1 += dy;
            err_2 += dz;
            x += x_inc;
        }
    } else if ((dy >= dx) && (dy >= dz)) {
        while (fabs(y - y2) > step) {
            if(output) {
                output->push_back(x);
                output->push_back(y);
                output->push_back(z);
            } else {
                Set(x / step, y / step, z / step, value);
            }
            if (err_1 > 0) {
                x += x_inc;
                err_1 -= dy;
            }
            if (err_2 > 0) {
                z += z_inc;
                err_2 -= dy;
            }
            err_1 += dx;
            err_2 += dz;
            y += y_inc;
        }
    } else {
        while (fabs(z - z2) > step) {
            if(output) {
                output->push_back(x);
                output->push_back(y);
                output->push_back(z);
            } else {
                Set(x / step, y / step, z / step, value);
            }
            if (err_1 > 0) {
                x += x_inc;
                err_1 -= dz;
            }
            if (err_2 > 0) {
                y += y_inc;
                err_2 -= dz;
            }
            err_1 += dx;
            err_2 += dy;
            z += z_inc;
        }
    }
}

void Array3d::Set(int x, int y, int z, bool value) {
    std::pair<int, int> key;
    key.first = x;
    key.second = z;
    if (data.find(key) == data.end())
        data[key] = std::vector<int>();
    int fy = ARRAY3D_MINY;
    unsigned int i = 0;
    bool output = defaultValue;
    for (std::vector<int>::const_iterator it = data[key].begin(); it != data[key].end(); ++it, i++) {
        if ((fy <= y) && (fy + *it > y)) {
            if (output != value) {
                int old = data[key][i];
                data[key][i] = y - fy;
                data[key].insert(data[key].begin() + i + 1, 1);
                data[key].insert(data[key].begin() + i + 2, old - y + fy - 1);
            }
            return;
        }
        fy += *it;
        output = !output;
    }
    if (output != value) {
        data[key].push_back(y - fy);
        data[key].push_back(1);
    }
}

void Array3d::Unpack(std::map<std::pair<int, int>, std::vector<int> >::const_iterator it,
                     float scale, std::vector<glm::vec3>& coords, int xp, int zp) {
    int i, j, k;
    int x, y, z;
    int thCase;
    bool valid[8];
    glm::vec3 voxels[8];
    Compress(it->first);
    int fy = ARRAY3D_MINY;
    bool output = defaultValue;
    for (std::vector<int>::const_iterator ity = it->second.begin(); ity != it->second.end(); ++ity) {
        if (output != defaultValue) {
            x = it->first.first + xp;
            z = it->first.second + zp;
            for (y = fy - 1; y < fy + *ity; y++) {
                //extract voxels
                for (i = 0; i < 8; i++) {
                    valid[i] = Get(x + offset[0][i], y + offset[1][i], z + offset[2][i]);
                    voxels[i] = glm::vec3(x + offset[0][i], y + offset[1][i], z + offset[2][i]);
                }

                //generate geometry
                for (j = 0; j < 6; j++) {
                    thCase = 0;
                    for (i = 0; i < 4; i++)
                        if (!valid[ths[j][i]])
                            thCase |= 1 << i;

                    if ((thCase != 0) && (thCase != 15))
                        for (i = 0; i < 6; i += 3)
                            if (thTriangles[thCase][i] >= 0)
                                for (k = i; k < i + 3; k++)
                                    coords.push_back((voxels[ths[j][edges_index[thTriangles[thCase][k]][0]]] + voxels[ths[j][edges_index[thTriangles[thCase][k]][1]]]) * scale * 0.5f);
                }
            }
        }
        fy += *ity;
        output = !output;
    }
}

void Array3d::Unpack(int minx, int minz, int maxx, int maxz, double scale, std::vector<glm::vec3>& coords) {
    int x, z;
    std::map<std::pair<int, int>, std::vector<int> >::const_iterator it;
    for (it = data.begin(); it != data.end(); ++it) {
        if ((minx > it->first.first) || (maxx < it->first.first) || (minz > it->first.second) || (maxz < it->first.second))
            continue;
        Unpack(it, scale, coords, 0, 0);
        for (x = -1; x <= 0; x++)
            for (z = -1; z <= 0; z++)
                if (data.find(std::pair<int, int>(it->first.first + x, it->first.second + z)) == data.end())
                    Unpack(it, scale, coords, x, z);
    }
}
}
