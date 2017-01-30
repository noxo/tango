#include <tango_3d_reconstruction_api.h>
#include "model_io.h"
#include <sstream>

namespace mesh_builder {

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
        if (ext.compare("ply") == 0) {
            type = PLY;
            if (writeMode)
                file = fopen(filename.c_str(), "w");
            else {
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
        } else if (ext.compare("obj") == 0) {
            type = OBJ;
            if (writeMode)
                file = fopen(filename.c_str(), "w");
        }
    }

    ModelIO::~ModelIO() {
        if (type != ModelIO::OBJ)
            fclose(file);
    }

    void ModelIO::parseFaces(int subdivision, std::vector<tango_gl::StaticMesh>& output) {
        unsigned int offset = 0;
        int parts = faceCount / subdivision;
        if(faceCount % subdivision > 0)
            parts++;
        unsigned int t, a, b, c;
        glm::vec2 uv;
        glm::vec3 vec;

        //subdivision cycle
        for (int j = 0; j < parts; j++)  {
            int count = subdivision;
            if (j == parts - 1)
                count = faceCount % subdivision;

            int textureCount = 0;
            if (type == PLY)
                textureCount = 1;
            else if (type == OBJ)
                textureCount = tango_mesh->num_textures;
            else
                assert(false);

            //texture cycle
            for (int k = 0; k < textureCount; k++)
            {
                output.push_back(tango_gl::StaticMesh());
                unsigned long meshIndex = output.size() - 1;
                output[meshIndex].render_mode = GL_TRIANGLES;
                if (type == PLY)
                    output[meshIndex].texture = -1;
                else if (type == OBJ) {
                    output[meshIndex].texture = k;
                    output[meshIndex].textures = tango_mesh->textures;
                } else
                    assert(false);

                //face cycle
                for (int i = 0; i < count; i++)  {
                    if (type == PLY)
                        fscanf(file, "%d %d %d %d", &t, &a, &b, &c);
                    else if (type == OBJ) {
                        t = 3;
                        a = tango_mesh->faces[i + offset][0];
                        b = tango_mesh->faces[i + offset][1];
                        c = tango_mesh->faces[i + offset][2];
                    } else
                        assert(false);
                    //unsupported format
                    if (t != 3)
                        continue;
                    //broken topology ignored
                    if ((a == b) || (a == c) || (b == c))
                        continue;
                    if (type == PLY) {
                        output[meshIndex].vertices.push_back(data.vertices[a]);
                        output[meshIndex].colors.push_back(data.colors[a]);
                        output[meshIndex].vertices.push_back(data.vertices[b]);
                        output[meshIndex].colors.push_back(data.colors[b]);
                        output[meshIndex].vertices.push_back(data.vertices[c]);
                        output[meshIndex].colors.push_back(data.colors[c]);
                    } else if (type == OBJ) {
                        if(tango_mesh->texture_ids[i + offset] != k)
                            continue;
                        vec.x = tango_mesh->vertices[a][0];
                        vec.y = tango_mesh->vertices[a][1];
                        vec.z = tango_mesh->vertices[a][2];
                        output[meshIndex].vertices.push_back(vec);
                        vec.x = tango_mesh->vertices[b][0];
                        vec.y = tango_mesh->vertices[b][1];
                        vec.z = tango_mesh->vertices[b][2];
                        output[meshIndex].vertices.push_back(vec);
                        vec.x = tango_mesh->vertices[c][0];
                        vec.y = tango_mesh->vertices[c][1];
                        vec.z = tango_mesh->vertices[c][2];
                        output[meshIndex].vertices.push_back(vec);
                        uv.s = tango_mesh->texture_coords[a][0];
                        uv.t = tango_mesh->texture_coords[a][1];
                        output[meshIndex].uv.push_back(uv);
                        uv.s = tango_mesh->texture_coords[b][0];
                        uv.t = tango_mesh->texture_coords[b][1];
                        output[meshIndex].uv.push_back(uv);
                        uv.s = tango_mesh->texture_coords[c][0];
                        uv.t = tango_mesh->texture_coords[c][1];
                        output[meshIndex].uv.push_back(uv);
                    } else
                        assert(false);
                }
            }
            offset += count;
        }
    }

    void ModelIO::readVertices() {
        assert(!writeMode);
        if (type == PLY) {
            unsigned int a, b, c;
            glm::vec3 v;
            for (int i = 0; i < vertexCount; i++) {
                fscanf(file, "%f %f %f %d %d %d", &v.x, &v.z, &v.y, &a, &b, &c);
                v.x *= -1.0f;
                data.vertices.push_back(v);
                data.colors.push_back(a + (b << 8) + (c << 16));
            }
        } else if (type == OBJ) {
            Tango3DR_Mesh_loadFromObj(path.c_str(), &tango_mesh);
            faceCount = tango_mesh->num_faces;
            vertexCount = tango_mesh->num_vertices;
        }
        else
            assert(false);
    }

    void ModelIO::writeModel(std::vector<SingleDynamicMesh*> model) {
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
            writeHeader(model);
            for (unsigned int i = 0; i < model.size(); i++)
                writeMesh(model[i], vectorSize[i]);
            int offset = 0;
            if (type == OBJ)
                offset++;
            for (unsigned int i = 0; i < model.size(); i++) {
                if (type == OBJ)
                    fprintf(file, "usemtl %d\n", i);
                writeFaces(model[i], offset);
                offset += vectorSize[i];
            }
        } else
            assert(false);
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

    void ModelIO::writeHeader(std::vector<SingleDynamicMesh*> model) {
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
                std::ostringstream srcPath;
                srcPath << dataset.c_str();
                srcPath << "/frame_";
                srcPath << i;
                srcPath << ".png";
                std::ostringstream dstPath;
                dstPath << base.c_str();
                dstPath << "_";
                dstPath << i;
                dstPath << ".png";
                LOGI("Moving %s to %s", srcPath.str().c_str(), dstPath.str().c_str());
                rename(srcPath.str().c_str(), dstPath.str().c_str());

                fprintf(mtl, "newmtl %d\n", i);
                fprintf(mtl, "Ns 96.078431\n");
                fprintf(mtl, "Ka 1.000000 1.000000 1.000000\n");
                fprintf(mtl, "Kd 0.640000 0.640000 0.640000\n");
                fprintf(mtl, "Ks 0.500000 0.500000 0.500000\n");
                fprintf(mtl, "Ke 0.000000 0.000000 0.000000\n");
                fprintf(mtl, "Ni 1.000000\n");
                fprintf(mtl, "d 1.000000\n");
                fprintf(mtl, "illum 2\n");
                fprintf(mtl, "map_Kd %s_%d.png\n\n", shortBase.c_str(), i);
            }
            fclose(mtl);
        }
    }

    void ModelIO::writeMesh(SingleDynamicMesh *mesh, int size) {
        glm::vec3 v;
        glm::vec2 t;
        glm::ivec3 c;
        for(unsigned int j = 0; j < size; j++) {
            v = mesh->mesh.vertices[j];
            if (type == PLY) {
                c = decodeColor(mesh->mesh.colors[j]);
                fprintf(file, "%f %f %f %d %d %d\n", -v.x, v.z, v.y, c.r, c.g, c.b);
            } else if (type == OBJ) {
                t = mesh->mesh.uv[j];
                fprintf(file, "v %f %f %f\n", v.x, v.y, v.z);
                fprintf(file, "vt %f %f\n", t.x, t.y);
            }
        }
    }

    void ModelIO::writeFaces(SingleDynamicMesh *mesh, int offset) {
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
} // namespace mesh_builder