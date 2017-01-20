#ifndef OPEN_CONSTRUCTOR_MODEL_IO_H
#define OPEN_CONSTRUCTOR_MODEL_IO_H

#include <glm/glm.hpp>
#include <tango-gl/tango-gl.h>
#include <map>
#include <string>
#include "scene.h"

namespace mesh_builder {

class ModelIO {
public:
    ModelIO(std::string filename, bool writeAccess);
    ~ModelIO();
    void parseFaces(int subdivision, std::vector<tango_gl::StaticMesh>& output);
    void parseFacesFiltered(int passes, std::vector<glm::ivec3>& indices);
    void readVertices();
    void readVerticesAsIndexTable();
    void writeModel(ModelIO& model, std::vector<glm::ivec3>& indices);
    void writeModel(std::vector<SingleDynamicMesh*> model);

private:
    glm::ivec3 decodeColor(unsigned int c);
    unsigned int scanDec(char *line, int offset);
    bool startsWith(std::string s, std::string e);
    void writeHeader();
    void writeColorMesh(SingleDynamicMesh* mesh);
    void writeColorVertex(glm::vec3 v, glm::ivec3 c);
    void writeFace(glm::ivec3 i);
    void writeFaces(SingleDynamicMesh* mesh, int offset);

    bool writeMode;
    unsigned int vertexCount;
    unsigned int faceCount;
    FILE* file;
    std::map<unsigned int, unsigned int> index2index;
    tango_gl::StaticMesh data;
};
} // namespace mesh_builder


#endif //OPEN_CONSTRUCTOR_MODEL_IO_H
