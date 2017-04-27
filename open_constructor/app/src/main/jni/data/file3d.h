#ifndef DATA_FILE3D_H
#define DATA_FILE3D_H

#include <map>
#include <mutex>
#include <string>
#ifndef NOTANGO
#include <tango_3d_reconstruction_api.h>
#endif
#include "data/mesh.h"

struct SingleDynamicMesh {
#ifndef NOTANGO
    Tango3DR_Mesh tango_mesh;
#endif
    oc::Mesh mesh;
    std::mutex mutex;
    unsigned long size;
};

namespace oc {
    enum TYPE{OBJ, PLY};

class File3d {
public:
    File3d(std::string filename, bool writeAccess);
    ~File3d();
    TYPE GetType() { return type; }
    void ReadModel(int subdivision, std::vector<oc::Mesh>& output);
    void WriteModel(std::vector<SingleDynamicMesh*> model);

private:
    glm::ivec3 DecodeColor(unsigned int c);
    void ParseOBJ(int subdivision, std::vector<oc::Mesh> &output);
    void ParsePLYFaces(int subdivision, std::vector<oc::Mesh> &output);
    void ReadHeader();
    void ReadPLYVertices();
    unsigned int ScanDec(char *line, int offset);
    bool StartsWith(std::string s, std::string e);
    void WriteHeader(std::vector<SingleDynamicMesh*> model);
    void WritePointCloud(SingleDynamicMesh *mesh, int size);
    void WriteFaces(SingleDynamicMesh *mesh, int offset);

    TYPE type;
    std::string path;
    bool writeMode;
    unsigned int vertexCount;
    unsigned int faceCount;
    FILE* file;
    Mesh data;
    std::map<std::string, std::string> keyToFile;
};
}

#endif
