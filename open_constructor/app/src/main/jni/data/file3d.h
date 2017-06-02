#ifndef DATA_FILE3D_H
#define DATA_FILE3D_H

#include <map>
#include <string>
#include "data/mesh.h"

namespace oc {
    enum TYPE{OBJ, PLY};

class File3d {
public:
    File3d(std::string filename, bool writeAccess);
    ~File3d();
    void ReadModel(int subdivision, std::vector<oc::Mesh>& output);
    void WriteModel(std::vector<Mesh>& model);

private:
    glm::ivec3 DecodeColor(unsigned int c);
    void ParseOBJ(int subdivision, std::vector<oc::Mesh> &output);
    void ParsePLYFaces(int subdivision, std::vector<oc::Mesh> &output);
    void ReadHeader();
    void ReadPLYVertices();
    unsigned int ScanDec(char *line, int offset);
    bool StartsWith(std::string s, std::string e);
    void WriteHeader(std::vector<Mesh>& model);
    void WritePointCloud(Mesh& mesh, int size);
    void WriteFaces(Mesh& mesh, int offset);

    TYPE type;
    std::string path;
    bool writeMode;
    unsigned int vertexCount;
    unsigned int faceCount;
    FILE* file;
    Mesh data;
    std::map<std::string, int> fileToIndex;
    std::map<std::string, std::string> keyToFile;
};
}

#endif
