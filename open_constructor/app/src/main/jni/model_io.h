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
    void readVertices();
    void setTangoObjects(std::string dataset, Tango3DR_ConfigH config, Tango3DR_Mesh* mesh);
    void writeModel(std::vector<SingleDynamicMesh*> model);

    enum TYPE{OBJ, PLY};

private:
    glm::ivec3 decodeColor(unsigned int c);
    glm::vec3 decodeColorF(unsigned int c);
    unsigned int scanDec(char *line, int offset);
    bool startsWith(std::string s, std::string e);
    void writePLYHeader();
    void writePLYColorMesh(SingleDynamicMesh *mesh, int size);
    void writePLYColorVertex(glm::vec3 v, glm::ivec3 c);
    void writePLYFace(glm::ivec3 i);
    void writePLYFaces(SingleDynamicMesh *mesh, int offset);

    TYPE type;
    std::string path;
    bool writeMode;
    unsigned int vertexCount;
    unsigned int faceCount;
    FILE* file;
    tango_gl::StaticMesh data;
    std::string dataset_;
    Tango3DR_ConfigH textureConfig;
    Tango3DR_Mesh* tango_mesh;
};
} // namespace mesh_builder


#endif //OPEN_CONSTRUCTOR_MODEL_IO_H
