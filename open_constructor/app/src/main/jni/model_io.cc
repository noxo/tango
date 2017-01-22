#include <tango_3d_reconstruction_api.h>
#include "model_io.h"

void onProgressRouter(int progress, void* pass) {
    LOGI("%ld Save progress %d", (long)pass, progress);
}

namespace mesh_builder {

    ModelIO::ModelIO(std::string filename, bool writeAccess) {
        path = filename;
        writeMode = writeAccess;
        vertexCount = 0;
        faceCount = 0;
        tango_mesh = nullptr;

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
        } else if (ext.compare("obj") == 0)
            type = OBJ;
    }

    ModelIO::~ModelIO() {
        if (type != ModelIO::OBJ)
            fclose(file);
    }

    void ModelIO::parseFaces(int subdivision, std::vector<tango_gl::StaticMesh>& output) {
        unsigned int index = 0;
        int parts = faceCount / subdivision;
        if(faceCount % subdivision > 0)
            parts++;
        unsigned int t, a, b, c;
        glm::vec3 vec;
        for (int j = 0; j < parts; j++)  {
            tango_gl::StaticMesh static_mesh;
            static_mesh.render_mode = GL_TRIANGLES;
            int count = subdivision;
            if (j == parts - 1)
                count = faceCount % subdivision;
            for (int i = 0; i < count; i++)  {
                if (type == PLY)
                    fscanf(file, "%d %d %d %d", &t, &a, &b, &c);
                else if (type == OBJ) {
                    t = 3;
                    a = tango_mesh->faces[index][0];
                    b = tango_mesh->faces[index][1];
                    c = tango_mesh->faces[index][2];
                    index++;
                } else
                    assert(false);
                //unsupported format
                if (t != 3)
                    continue;
                //broken topology ignored
                if ((a == b) || (a == c) || (b == c))
                    continue;
                if (type == PLY) {
                    static_mesh.vertices.push_back(data.vertices[a]);
                    static_mesh.colors.push_back(data.colors[a]);
                    static_mesh.vertices.push_back(data.vertices[b]);
                    static_mesh.colors.push_back(data.colors[b]);
                    static_mesh.vertices.push_back(data.vertices[c]);
                    static_mesh.colors.push_back(data.colors[c]);
                } else if (type == OBJ) {
                    vec.x = tango_mesh->vertices[a][0];
                    vec.y = tango_mesh->vertices[a][1];
                    vec.z = tango_mesh->vertices[a][2];
                    static_mesh.vertices.push_back(vec);
                    vec.x = tango_mesh->vertices[b][0];
                    vec.y = tango_mesh->vertices[b][1];
                    vec.z = tango_mesh->vertices[b][2];
                    static_mesh.vertices.push_back(vec);
                    vec.x = tango_mesh->vertices[c][0];
                    vec.y = tango_mesh->vertices[c][1];
                    vec.z = tango_mesh->vertices[c][2];
                    static_mesh.vertices.push_back(vec);
                    //TODO: load and render UVs and textures
                    static_mesh.colors.push_back(0xFFFFFFFF);
                    static_mesh.colors.push_back(0xFFFFFFFF);
                    static_mesh.colors.push_back(0xFFFFFFFF);
                } else
                    assert(false);
            }
            output.push_back(static_mesh);
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

    void ModelIO::setTangoObjects(std::string dataset, Tango3DR_ConfigH config) {
        dataset_ = dataset;
        textureConfig = config;
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
        if (type == PLY) {
            writePLYHeader();
            for (unsigned int i = 0; i < model.size(); i++)
                writePLYColorMesh(model[i], vectorSize[i]);
            int offset = 0;
            for (unsigned int i = 0; i < model.size(); i++) {
                writePLYFaces(model[i], offset);
                offset += vectorSize[i];
            }
        } else if (type == OBJ) {
            Tango3DR_TrajectoryH tr;
            Tango3DR_Status ret;
            ret = Tango3DR_createTrajectoryFromDataset(dataset_.c_str(), &tr, onProgressRouter, (void*)1);
            if (ret != TANGO_3DR_SUCCESS)
                return;

            tango_gl::StaticMesh fullMesh{};
            unsigned int offset = 0;
            for (unsigned int j = 0; j < model.size(); j++) {
                SingleDynamicMesh* mesh = model[j];
                mesh->mutex.lock();
                for (unsigned int i = 0; i < mesh->size; i+=3) {
                    fullMesh.indices.push_back(mesh->mesh.indices[i + 0] + offset);
                    fullMesh.indices.push_back(mesh->mesh.indices[i + 1] + offset);
                    fullMesh.indices.push_back(mesh->mesh.indices[i + 2] + offset);
                }
                offset += vectorSize[j];
                std::vector<uint32_t>().swap(mesh->mesh.indices);
                //vertices and colors
                for(unsigned int i = 0; i < vectorSize[j]; i++)
                    fullMesh.vertices.push_back(mesh->mesh.vertices[i]);
                std::vector<glm::vec3>().swap(mesh->mesh.vertices);
                for(unsigned int i = 0; i < vectorSize[j]; i++)
                    fullMesh.colors.push_back(mesh->mesh.colors[i]);
                std::vector<uint32_t>().swap(mesh->mesh.colors);
                fullMesh.render_mode = mesh->mesh.render_mode;
                mesh->mutex.unlock();
            }
            Tango3DR_Mesh meshIn = {
                    /* timestamp */ 0.0,
                    /* num_vertices */ static_cast<uint32_t>(fullMesh.vertices.size()),
                    /* num_faces */ static_cast<uint32_t>(fullMesh.indices.size() / 3),
                    /* num_textures */ 0u,
                    /* max_num_vertices */ static_cast<uint32_t>(fullMesh.vertices.size()),
                    /* max_num_faces */ static_cast<uint32_t>(fullMesh.indices.size() / 3),
                    /* max_num_textures */ 0u,
                    /* vertices */ reinterpret_cast<Tango3DR_Vector3 *>(fullMesh.vertices.data()),
                    /* faces */ reinterpret_cast<Tango3DR_Face *>(fullMesh.indices.data()),
                    /* normals */ nullptr,
                    /* colors */ reinterpret_cast<Tango3DR_Color *>(fullMesh.colors.data()),
                    /* texture_coords */ nullptr,
                    /* texture_ids */ nullptr,
                    /* textures */ nullptr};

            ret = Tango3DR_textureMeshFromDataset(textureConfig, dataset_.c_str(), tr, &meshIn,
                                                  &tango_mesh, onProgressRouter, (void*)2);
            if (ret != TANGO_3DR_SUCCESS)
                std::exit(EXIT_SUCCESS);
            Tango3DR_Mesh_saveToObj(tango_mesh, path.c_str());

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

    void ModelIO::writePLYHeader() {
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

    void ModelIO::writePLYColorMesh(SingleDynamicMesh *mesh, int size) {
        glm::vec3 v;
        glm::ivec3 c;
        for(unsigned int j = 0; j < size; j++) {
            v = mesh->mesh.vertices[j];
            c = decodeColor(mesh->mesh.colors[j]);
            writePLYColorVertex(v, c);
        }
    }

    void ModelIO::writePLYColorVertex(glm::vec3 v, glm::ivec3 c) {
        fprintf(file, "%f %f %f %d %d %d\n", -v.x, v.z, v.y, c.r, c.g, c.b);
    }

    void ModelIO::writePLYFace(glm::ivec3 i) {
        fprintf(file, "3 %d %d %d\n", i.x, i.y, i.z);
    }

    void ModelIO::writePLYFaces(SingleDynamicMesh *mesh, int offset) {
        glm::ivec3 i;
        for (unsigned int j = 0; j < mesh->size; j+=3) {
            i.x = mesh->mesh.indices[j + 0] + offset;
            i.y = mesh->mesh.indices[j + 1] + offset;
            i.z = mesh->mesh.indices[j + 2] + offset;
            writePLYFace(i);
        }
    }
} // namespace mesh_builder