#ifndef OPEN_CONSTRUCTOR_MODEL_IO_H
#define OPEN_CONSTRUCTOR_MODEL_IO_H

#include <glm/glm.hpp>
#include <tango-gl/tango-gl.h>
#include <map>
#include <mutex>
#include <string>

struct SingleDynamicMesh {
    Tango3DR_Mesh tango_mesh;
    tango_gl::StaticMesh mesh;
    std::mutex mutex;
    unsigned long size;
};

namespace mesh_builder {

class ModelIO {
public:
    ModelIO(std::string filename, bool writeAccess);
    ~ModelIO();
    std::map<int, std::string> ReadModel(int subdivision, std::vector<tango_gl::StaticMesh>& output);
    void WriteModel(std::vector<SingleDynamicMesh*> model);

    enum TYPE{OBJ, PLY};

private:
    glm::ivec3 DecodeColor(unsigned int c);
    void ParseOBJ(int subdivision, std::vector<tango_gl::StaticMesh> &output);
    void ParsePLYFaces(int subdivision, std::vector<tango_gl::StaticMesh> &output);
    std::map<int, std::string> ReadHeader();
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
    tango_gl::StaticMesh data;
};
} // namespace mesh_builder


#endif //OPEN_CONSTRUCTOR_MODEL_IO_H
