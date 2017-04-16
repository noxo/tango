#include <tango_3d_reconstruction_api.h>
#include "model_io.h"

namespace oc {

    ModelIO::ModelIO(std::string filename, bool writeAccess) {
        path = filename;
        writeMode = writeAccess;
        vertexCount = 0;
        faceCount = 0;

        if (writeMode)
            LOGI("Writing into %s", filename.c_str());
        else
            LOGI("Loading from %s", filename.c_str());

        std::string ext = filename.substr(filename.size() - 3, filename.size() - 1);
        if (ext.compare("ply") == 0)
            type = PLY;
        else if (ext.compare("obj") == 0)
            type = OBJ;

        if (writeMode)
            file = fopen(filename.c_str(), "w");
        else
            file = fopen(filename.c_str(), "r");
    }

    ModelIO::~ModelIO() {
        fclose(file);
    }

    void ModelIO::ReadModel(int subdivision, std::vector<GLMesh>& output) {
        assert(!writeMode);
        ReadHeader();
        if (type == PLY) {
            ReadPLYVertices();
            ParsePLYFaces(subdivision, output);
        } else if (type == OBJ) {
            ParseOBJ(subdivision, output);
        } else
            assert(false);
    }

    void ModelIO::WriteModel(std::vector<SingleDynamicMesh*> model) {
        assert(writeMode);
        //count vertices and faces
        faceCount = 0;
        vertexCount = 0;
        std::vector<unsigned int> vectorSize;
        for(unsigned int i = 0; i < model.size(); i++) {
            unsigned int max = 0;
            unsigned int value = 0;
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
        if ((type == PLY) || (type == OBJ)) {
            WriteHeader(model);
            for (unsigned int i = 0; i < model.size(); i++)
                WritePointCloud(model[i], vectorSize[i]);
            int offset = 0;
            if (type == OBJ)
                offset++;
            int texture = -1;
            for (unsigned int i = 0; i < model.size(); i++) {
                if (model[i]->mesh.indices.empty()) {
                    offset += vectorSize[i];
                    continue;
                }
                if (type == OBJ) {
                    if (texture != model[i]->mesh.texture) {
                        texture = model[i]->mesh.texture;
                        fprintf(file, "usemtl %d\n", texture);
                    }
                }
                WriteFaces(model[i], offset);
                offset += vectorSize[i];
            }
        } else
            assert(false);
    }

    glm::ivec3 ModelIO::DecodeColor(unsigned int c) {
        glm::ivec3 output;
        output.r = (c & 0x000000FF);
        output.g = (c & 0x0000FF00) >> 8;
        output.b = (c & 0x00FF0000) >> 16;
        return output;
    }

    void ModelIO::ParseOBJ(int subdivision, std::vector<GLMesh> &output) {
        char buffer[1024];
        unsigned long meshIndex = 0;
        glm::vec3 v;
        glm::vec3 n;
        glm::vec2 t;
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;
        bool hasNormals = false;
        bool hasCoords = false;
        unsigned int va, vna, vta, vb, vnb, vtb, vc, vnc, vtc;
        while (true) {
            if (!fgets(buffer, 1024, file))
                break;
            if (buffer[0] == 'u') {
                char key[1024];
                sscanf(buffer, "usemtl %s", key);
                meshIndex = output.size();
                output.push_back(GLMesh());
                output[meshIndex].image = new RGBImage(keyToFile[std::string(key)]);
            } else if ((buffer[0] == 'v') && (buffer[1] == ' ')) {
                sscanf(buffer, "v %f %f %f", &v.x, &v.y, &v.z);
                vertices.push_back(v);
            } else if ((buffer[0] == 'v') && (buffer[1] == 't')) {
                sscanf(buffer, "vt %f %f", &t.x, &t.y);
                uvs.push_back(t);
                hasCoords = true;
            } else if ((buffer[0] == 'v') && (buffer[1] == 'n')) {
                sscanf(buffer, "vn %f %f %f", &n.x, &n.y, &n.z);
                normals.push_back(n);
                hasNormals = true;
            } else if ((buffer[0] == 'f') && (buffer[1] == ' ')) {
                va = 0;
                vb = 0;
                vc = 0;
                if (!hasCoords && !hasNormals)
                    sscanf(buffer, "f %d %d %d", &va, &vb, &vc);
                else if (hasCoords && !hasNormals)
                    sscanf(buffer, "f %d/%d %d/%d %d/%d", &va, &vta, &vb, &vtb, &vc, &vtc);
                else if (hasCoords && hasNormals)
                    sscanf(buffer, "f %d/%d/%d %d/%d/%d %d/%d/%d",
                           &va, &vta, &vna, &vb, &vtb, &vnb, &vc, &vtc, &vnc);
                else if (!hasCoords && hasNormals)
                    sscanf(buffer, "f %d//%d %d//%d %d//%d", &va, &vna, &vb, &vnb, &vc, &vnc);
                //broken topology ignored
                if ((va == vb) || (va == vc) || (vb == vc))
                    continue;
                //incomplete line ignored
                if ((va == 0) || (vb == 0) || (vc == 0))
                    continue;
                output[meshIndex].vertices.push_back(vertices[va - 1]);
                output[meshIndex].vertices.push_back(vertices[vb - 1]);
                output[meshIndex].vertices.push_back(vertices[vc - 1]);
                if (hasCoords) {
                    output[meshIndex].uv.push_back(uvs[vta - 1]);
                    output[meshIndex].uv.push_back(uvs[vtb - 1]);
                    output[meshIndex].uv.push_back(uvs[vtc - 1]);
                }
                if (hasNormals) {
                    output[meshIndex].normals.push_back(normals[vna - 1]);
                    output[meshIndex].normals.push_back(normals[vnb - 1]);
                    output[meshIndex].normals.push_back(normals[vnc - 1]);
                }
                if (output[meshIndex].vertices.size() >= subdivision * 3) {
                    meshIndex = output.size();
                    output.push_back(GLMesh());
                    output[meshIndex].image = output[meshIndex - 1].image;
                    output[meshIndex].imageOwner = false;
                }
            }
        }
    }

    void ModelIO::ParsePLYFaces(int subdivision, std::vector<GLMesh> &output) {
        unsigned int offset = 0;
        int parts = faceCount / subdivision;
        if(faceCount % subdivision > 0)
            parts++;
        unsigned int t, a, b, c;

        //subdivision cycle
        for (int j = 0; j < parts; j++)  {
            int count = subdivision;
            if (j == parts - 1)
                count = faceCount % subdivision;
                unsigned long meshIndex = output.size();
                output.push_back(GLMesh());

                //face cycle
                for (int i = 0; i < count; i++)  {
                    fscanf(file, "%d %d %d %d", &t, &a, &b, &c);
                    //unsupported format
                    if (t != 3)
                        continue;
                    //broken topology ignored
                    if ((a == b) || (a == c) || (b == c))
                        continue;
                    output[meshIndex].vertices.push_back(data.vertices[a]);
                    output[meshIndex].colors.push_back(data.colors[a]);
                    output[meshIndex].vertices.push_back(data.vertices[b]);
                    output[meshIndex].colors.push_back(data.colors[b]);
                    output[meshIndex].vertices.push_back(data.vertices[c]);
                    output[meshIndex].colors.push_back(data.colors[c]);
                }
            offset += count;
        }
    }

    void ModelIO::ReadPLYVertices() {
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

    void ModelIO::ReadHeader() {
        char buffer[1024];
        if (type == PLY) {
            while (true) {
                if (!fgets(buffer, 1024, file))
                    break;
                if (StartsWith(buffer, "element vertex"))
                    vertexCount = ScanDec(buffer, 15);
                else if (StartsWith(buffer, "element face"))
                    faceCount = ScanDec(buffer, 13);
                else if (StartsWith(buffer, "end_header"))
                    break;
            }
        } else if (type == OBJ) {
            char mtlFile[1024];
            while (true) {
                if (!fgets(buffer, 1024, file))
                    break;
                if (StartsWith(buffer, "mtllib")) {
                    sscanf(buffer, "mtllib %s", mtlFile);
                    break;
                }
            }
            unsigned int index = 0;
            for (unsigned long i = 0; i < path.size(); i++) {
                if (path[i] == '/')
                    index = (unsigned int) i;
            }
            char key[1024];
            char pngFile[1024];
            std::string data = path.substr(0, index + 1);
            FILE* mtl = fopen((data + mtlFile).c_str(), "r");
            while (true) {
                if (!fgets(buffer, 1024, mtl))
                    break;
                if (StartsWith(buffer, "newmtl")) {
                    sscanf(buffer, "newmtl %s", key);
                }
                if (StartsWith(buffer, "map_Kd")) {
                    sscanf(buffer, "map_Kd %s", pngFile);
                    keyToFile[std::string(key)] = data + pngFile;
                }
            }
            fclose(mtl);
        } else
            assert(false);
    }

    unsigned int ModelIO::ScanDec(char *line, int offset) {
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

    bool ModelIO::StartsWith(std::string s, std::string e) {
        if (s.size() >= e.size())
        if (s.substr(0, e.size()).compare(e) == 0)
            return true;
        return false;
    }

    void ModelIO::WriteHeader(std::vector<SingleDynamicMesh*> model) {
        if (type == PLY) {
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
        } else if (type == OBJ) {
            std::string base = (path.substr(0, path.length() - 4));
            unsigned long index = 0;
            for (unsigned long i = 0; i < base.size(); i++) {
                if (base[i] == '/')
                    index = i;
            }
            std::string shortBase = base.substr(index + 1, base.length());
            fprintf(file, "mtllib %s.mtl\n", shortBase.c_str());
            FILE* mtl = fopen((base + ".mtl").c_str(), "w");
            for (unsigned int i = 0; i < model.size(); i++) {
                if (model[i]->mesh.indices.empty())
                    continue;

                fprintf(mtl, "newmtl %d\n", i);
                fprintf(mtl, "Ns 96.078431\n");
                fprintf(mtl, "Ka 1.000000 1.000000 1.000000\n");
                fprintf(mtl, "Kd 0.640000 0.640000 0.640000\n");
                fprintf(mtl, "Ks 0.500000 0.500000 0.500000\n");
                fprintf(mtl, "Ke 0.000000 0.000000 0.000000\n");
                fprintf(mtl, "Ni 1.000000\n");
                fprintf(mtl, "d 1.000000\n");
                fprintf(mtl, "illum 2\n");
                if (model[i]->mesh.image)
                    fprintf(mtl, "map_Kd %s_%d.png\n\n", shortBase.c_str(), i);
            }
            fclose(mtl);
        }
    }

    void ModelIO::WritePointCloud(SingleDynamicMesh *mesh, int size) {
        glm::vec3 v;
        glm::vec2 t;
        glm::ivec3 c;
        for(unsigned int j = 0; j < size; j++) {
            v = mesh->mesh.vertices[j];
            if (type == PLY) {
                c = DecodeColor(mesh->mesh.colors[j]);
                fprintf(file, "%f %f %f %d %d %d\n", -v.x, v.z, v.y, c.r, c.g, c.b);
            } else if (type == OBJ) {
                t = mesh->mesh.uv[j];
                fprintf(file, "v %f %f %f\n", v.x, v.y, v.z);
                fprintf(file, "vt %f %f\n", t.x, t.y);
            }
        }
    }

    void ModelIO::WriteFaces(SingleDynamicMesh *mesh, int offset) {
        glm::ivec3 i;
        for (unsigned int j = 0; j < mesh->size; j+=3) {
            i.x = mesh->mesh.indices[j + 0] + offset;
            i.y = mesh->mesh.indices[j + 1] + offset;
            i.z = mesh->mesh.indices[j + 2] + offset;
            if (type == PLY)
                fprintf(file, "3 %d %d %d\n", i.x, i.y, i.z);
            else if (type == OBJ)
                fprintf(file, "f %d/%d %d/%d %d/%d\n", i.x, i.x, i.y, i.y, i.z, i.z);
        }
    }
}