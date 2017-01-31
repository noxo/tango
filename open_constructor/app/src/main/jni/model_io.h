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
    int size;
};

struct TextureToLoad {
    int width;
    int height;
    unsigned char* data;
};

namespace mesh_builder {

class ModelIO {
public:
    ModelIO(std::string filename, bool writeAccess);
    ~ModelIO();
    void setDataset(std::string path) { dataset = path; }
    std::vector<TextureToLoad> readModel(int subdivision, std::vector<tango_gl::StaticMesh>& output);
    void writeModel(std::vector<SingleDynamicMesh*> model);

    enum TYPE{OBJ, PLY};

private:
    glm::ivec3 decodeColor(unsigned int c);
    void parseOBJ(std::vector<tango_gl::StaticMesh> &output);
    void parsePLYFaces(int subdivision, std::vector<tango_gl::StaticMesh> &output);
    std::vector<TextureToLoad> readHeader();
    void readPLYVertices();
    unsigned int scanDec(char *line, int offset);
    bool startsWith(std::string s, std::string e);
    void writeHeader(std::vector<SingleDynamicMesh*> model);
    void writePointCloud(SingleDynamicMesh *mesh, int size);
    void writeFaces(SingleDynamicMesh *mesh, int offset);
    TextureToLoad readPNG(std::string file);

    TYPE type;
    std::string dataset;
    std::string path;
    bool writeMode;
    unsigned int vertexCount;
    unsigned int faceCount;
    FILE* file;
    tango_gl::StaticMesh data;
};
} // namespace mesh_builder


#endif //OPEN_CONSTRUCTOR_MODEL_IO_H
