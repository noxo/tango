#include "model_io.h"

namespace mesh_builder {

    ModelIO::ModelIO(std::string filename, bool writeAccess) {
        writeMode = writeAccess;
        vertexCount = 0;
        faceCount = 0;

        if (writeMode) {
            LOGI("Writing into %s", filename.c_str());
            file = fopen(filename.c_str(), "w");
        } else {
            LOGI("Loading from %s", filename.c_str());
            file = fopen(filename.c_str(), "r");

            //read header
            char buffer[1024];
            while (true) {
                if (!fgets(buffer, 1024, file))
                    break;
                if (startsWith(buffer, "element vertex"))
                    vertexCount = scanDec(buffer, 15);
                else if (startsWith(buffer, "element face"))
                    faceCount = scanDec(buffer, 13);
                else if (startsWith(buffer, "end_header"))
                    break;
            }
        }
    }

    ModelIO::~ModelIO() {
        fclose(file);
    }

    void ModelIO::parseFaces(int subdivision, std::vector<tango_gl::StaticMesh>& output) {
        int parts = faceCount / subdivision;
        if(faceCount % subdivision > 0)
            parts++;
        unsigned int t, a, b, c;
        for (int j = 0; j < parts; j++)  {
            tango_gl::StaticMesh static_mesh;
            static_mesh.render_mode = GL_TRIANGLES;
            int count = subdivision;
            if (j == parts - 1)
                count = faceCount % subdivision;
            for (int i = 0; i < count; i++)  {
                fscanf(file, "%d %d %d %d", &t, &a, &b, &c);
                //unsupported format
                if (t != 3)
                    continue;
                //broken topology ignored
                if ((a == b) || (a == c) || (b == c))
                    continue;
                static_mesh.vertices.push_back(data.vertices[a]);
                static_mesh.colors.push_back(data.colors[a]);
                static_mesh.vertices.push_back(data.vertices[b]);
                static_mesh.colors.push_back(data.colors[b]);
                static_mesh.vertices.push_back(data.vertices[c]);
                static_mesh.colors.push_back(data.colors[c]);
            }
            output.push_back(static_mesh);
        }
    }

    void ModelIO::parseFacesFiltered(int passes, std::vector<glm::ivec3>& indices) {
        //parse indices
        unsigned int t, a, b, c;
        std::vector<int> nodeLevel;
        for (unsigned int i = 0; i < vertexCount; i++)
            nodeLevel.push_back(0);
        for (unsigned int i = 0; i < faceCount; i++)  {
            fscanf(file, "%d %d %d %d", &t, &a, &b, &c);
            //unsupported format
            if (t != 3)
                continue;
            //broken topology ignored
            if ((a == b) || (a == c) || (b == c))
                continue;
            //reindex
            if (index2index.find(a) != index2index.end())
                a = index2index[a];
            if (index2index.find(b) != index2index.end())
                b = index2index[b];
            if (index2index.find(c) != index2index.end())
                c = index2index[c];
            //get node levels
            nodeLevel[a]++;
            nodeLevel[b]++;
            nodeLevel[c]++;
            //store indices
            indices.push_back(glm::ivec3(a, b, c));
        }

        //filter indices
        glm::ivec3 ci;
        std::vector<glm::ivec3> decrease;
        for (int pass = 0; pass < passes; pass++) {
            LOGI("Processing noise filter pass %d/%d", pass + 1, passes);
            for (long i = indices.size() - 1; i >= 0; i--) {
                ci = indices[i];
                if ((nodeLevel[ci.x] < 3) || (nodeLevel[ci.y] < 3) || (nodeLevel[ci.z] < 3)) {
                    indices.erase(indices.begin() + i);
                    decrease.push_back(ci);
                }
            }
            for (glm::ivec3 i : decrease) {
                nodeLevel[i.x]--;
                nodeLevel[i.y]--;
                nodeLevel[i.z]--;
            }
            decrease.clear();
        }
    }

    void ModelIO::readVertices() {
        assert(!writeMode);
        unsigned int a, b, c;
        glm::vec3 v;
        for (int i = 0; i < vertexCount; i++) {
            fscanf(file, "%f %f %f %d %d %d", &v.x, &v.z, &v.y, &a, &b, &c);
            v.x *= -1.0f;
            data.vertices.push_back(v);
            data.colors.push_back(a + (b << 8) + (c << 16));
        }
    }

    void ModelIO::readVerticesAsIndexTable() {
        assert(!writeMode);
        char buffer[1024];
        std::string key;
        glm::vec3 v;
        glm::ivec3 c;
        std::map<std::string, unsigned int> key2index;
        for (unsigned int i = 0; i < vertexCount; i++) {
            if (!fgets(buffer, 1024, file))
                break;
            sscanf(buffer, "%f %f %f %d %d %d", &v.x, &v.y, &v.z, &c.r, &c.g, &c.b);
            sprintf(buffer, "%.3f,%.3f,%.3f", v.x, v.y, v.z);
            key = std::string(buffer);
            if (key2index.find(key) == key2index.end())
                key2index[key] = i;
            else
                index2index[i] = key2index[key];
        }
    }

    void ModelIO::writeModel(ModelIO& model, std::vector<glm::ivec3>& indices) {
        assert(writeMode);
        faceCount = (unsigned int) indices.size();
        vertexCount = (unsigned int) model.data.vertices.size();
        //write
        writeHeader();
        for(unsigned int i = 0; i < model.data.vertices.size(); i++)
            writeColorVertex(model.data.vertices[i], decodeColor(model.data.colors[i]));
        for(unsigned int i = 0; i < indices.size(); i++)
            writeFace(indices[i]);
    }

    void ModelIO::writeModel(std::vector<SingleDynamicMesh*> model) {
        assert(writeMode);
        //count vertices and faces
        faceCount = 0;
        vertexCount = 0;
        std::vector<unsigned int> vectorSize;
        for(unsigned int i = 0; i < model.size(); i++) {
            int max = 0;
            int value = 0;
            for(unsigned int j = 0; j < model[i]->size; j++) {
                value = model[i]->mesh.indices[j];
                if(max < value)
                   max = value;
            }
            max++;
            faceCount += model[i]->size / 3;
            vertexCount += max;
            vectorSize.push_back(max);
        }
        //write
        writeHeader();
        for(unsigned int i = 0; i < model.size(); i++)
            writeColorMesh(model[i], vectorSize[i]);
        int offset = 0;
        for(unsigned int i = 0; i < model.size(); i++) {
            writeFaces(model[i], offset);
            offset += vectorSize[i];
        }
    }

    glm::ivec3 ModelIO::decodeColor(unsigned int c) {
        glm::ivec3 output;
        output.r = (c & 0x000000FF);
        output.g = (c & 0x0000FF00) >> 8;
        output.b = (c & 0x00FF0000) >> 16;
        return output;
    }

    unsigned int ModelIO::scanDec(char *line, int offset) {
        unsigned int number = 0;
        for (int i = offset; i < 1024; i++) {
            char c = line[i];
            if (c != '\n')
                number = number * 10 + c - '0';
            else
                return number;
        }
        return number;
    }

    bool ModelIO::startsWith(std::string s, std::string e) {
        if (s.size() >= e.size())
        if (s.substr(0, e.size()).compare(e) == 0)
            return true;
        return false;
    }

    void ModelIO::writeHeader() {
        fprintf(file, "ply\nformat ascii 1.0\ncomment ---\n");
        fprintf(file, "element vertex %d\n", vertexCount);
        fprintf(file, "property float x\n");
        fprintf(file, "property float y\n");
        fprintf(file, "property float z\n");
        fprintf(file, "property uchar red\n");
        fprintf(file, "property uchar green\n");
        fprintf(file, "property uchar blue\n");
        fprintf(file, "element face %d\n", faceCount);
        fprintf(file, "property list uchar uint vertex_indices\n");
        fprintf(file, "end_header\n");
    }

    void ModelIO::writeColorMesh(SingleDynamicMesh* mesh, int size) {
        glm::vec3 v;
        glm::ivec3 c;
        for(unsigned int j = 0; j < size; j++) {
            v = mesh->mesh.vertices[j];
            c = decodeColor(mesh->mesh.colors[j]);
            writeColorVertex(v, c);
        }
    }

    void ModelIO::writeColorVertex(glm::vec3 v, glm::ivec3 c) {
        fprintf(file, "%f %f %f %d %d %d\n", -v.x, v.z, v.y, c.r, c.g, c.b);
    }

    void ModelIO::writeFace(glm::ivec3 i) {
        fprintf(file, "3 %d %d %d\n", i.x, i.y, i.z);
    }

    void ModelIO::writeFaces(SingleDynamicMesh* mesh, int offset) {
        glm::ivec3 i;
        for (unsigned int j = 0; j < mesh->size; j+=3) {
            i.x = mesh->mesh.indices[j + 0] + offset;
            i.y = mesh->mesh.indices[j + 1] + offset;
            i.z = mesh->mesh.indices[j + 2] + offset;
            writeFace(i);
        }
    }
} // namespace mesh_builder