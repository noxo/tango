#ifndef MODEL_IO_H
#define MODEL_IO_H

#include <map>
#include <mutex>
#include <string>
#include <tango_3d_reconstruction_api.h>
#include "gl/mesh.h"

struct SingleDynamicMesh {
    Tango3DR_Mesh tango_mesh;
    oc::GLMesh mesh;
    std::mutex mutex;
    unsigned long size;
};

namespace oc {

class ModelIO {
public:
    ModelIO(std::string filename, bool writeAccess);
    ~ModelIO();
    void ReadModel(int subdivision, std::vector<oc::GLMesh>& output);
    void WriteModel(std::vector<SingleDynamicMesh*> model);

    enum TYPE{OBJ, PLY};

private:
    glm::ivec3 DecodeColor(unsigned int c);
    void ParseOBJ(int subdivision, std::vector<oc::GLMesh> &output);
    void ParsePLYFaces(int subdivision, std::vector<oc::GLMesh> &output);
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
    GLMesh data;
    std::map<std::string, std::string> keyToFile;
};
}

#endif
