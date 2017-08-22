#include "data/file3d.h"

namespace oc {

    File3d::File3d(std::string filename, bool writeAccess) {
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
        else
            assert(false);

        if (writeMode)
            file = fopen(filename.c_str(), "w");
        else
            file = fopen(filename.c_str(), "r");
    }

    File3d::~File3d() {
        fclose(file);
    }

    void File3d::ReadModel(int subdivision, std::vector<Mesh>& output) {
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

    void File3d::WriteModel(std::vector<Mesh>& model) {
        assert(writeMode);
        //count vertices and faces
        faceCount = 0;
        vertexCount = 0;
        std::vector<unsigned int> vectorSize;
        for(unsigned int i = 0; i < model.size(); i++) {
            unsigned int max = model[i].vertices.size() + 1;
            faceCount += model[i].vertices.size() / 3;
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
                if (model[i].vertices.empty()) {
                    offset += vectorSize[i];
                    continue;
                }
                if (type == OBJ) {
                    fprintf(file, "usemtl %d\n", fileToIndex[model[i].image->GetName()]);
                }
                WriteFaces(model[i], offset);
                offset += vectorSize[i];
            }
        } else
            assert(false);
    }

    glm::ivec3 File3d::DecodeColor(unsigned int c) {
        glm::ivec3 output;
        output.r = (c & 0x000000FF);
        output.g = (c & 0x0000FF00) >> 8;
        output.b = (c & 0x00FF0000) >> 16;
        return output;
    }

    void File3d::ParseOBJ(int subdivision, std::vector<Mesh> &output) {
        char buffer[1024];
        unsigned long meshIndex = 0;
        glm::vec3 v;
        glm::vec3 n;
        glm::vec2 t;
        std::string lastKey;
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;
        bool hasNormals = false;
        bool hasCoords = false;
        unsigned int va, vna, vta, vb, vnb, vtb, vc, vnc, vtc;
        std::map<std::string, Image*> images;
        while (true) {
            if (!fgets(buffer, 1024, file))
                break;
            std::string sbuf = buffer;
            while(!sbuf.empty() && isspace(sbuf[0])) {
                sbuf = sbuf.substr(1);
            }
            if (sbuf[0] == 'u') {
                char key[1024];
                sscanf(sbuf.c_str(), "usemtl %s", key);
                if (lastKey.compare(key) != 0) {
                    meshIndex = output.size();
                    output.push_back(Mesh());
                    if (images.find(key) == images.end()) {
                        std::string imagefile = keyToFile[std::string(key)];
                        if (imagefile.empty())
                        {
                            glm::vec3 color = keyToColor[std::string(key)];
                            images[key] = new Image(1, 1);
                            images[key]->GetData()[0] = (unsigned char) (255 * color.r);
                            images[key]->GetData()[1] = (unsigned char) (255 * color.g);
                            images[key]->GetData()[2] = (unsigned char) (255 * color.b);
                        }
                        else
                          images[key] = new Image(imagefile);
                        output[meshIndex].imageOwner = true;
                    } else
                        output[meshIndex].imageOwner = false;
                    output[meshIndex].image = images[key];
                    lastKey = key;
                }
            } else if ((sbuf[0] == 'v') && (sbuf[1] == ' ')) {
                sscanf(sbuf.c_str(), "v %f %f %f", &v.x, &v.y, &v.z);
                vertices.push_back(v);
            } else if ((sbuf[0] == 'v') && (sbuf[1] == 't')) {
                sscanf(sbuf.c_str(), "vt %f %f", &t.x, &t.y);
                uvs.push_back(t);
                hasCoords = true;
            } else if ((sbuf[0] == 'v') && (sbuf[1] == 'n')) {
                sscanf(sbuf.c_str(), "vn %f %f %f", &n.x, &n.y, &n.z);
                normals.push_back(n);
                hasNormals = true;
            } else if ((sbuf[0] == 'f') && (sbuf[1] == ' ')) {
                va = 0;
                vb = 0;
                vc = 0;
                if (!hasCoords && !hasNormals)
                    sscanf(sbuf.c_str(), "f %d %d %d", &va, &vb, &vc);
                else if (hasCoords && !hasNormals)
                    sscanf(sbuf.c_str(), "f %d/%d %d/%d %d/%d", &va, &vta, &vb, &vtb, &vc, &vtc);
                else if (hasCoords && hasNormals)
                    sscanf(sbuf.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d",
                           &va, &vta, &vna, &vb, &vtb, &vnb, &vc, &vtc, &vnc);
                else if (!hasCoords && hasNormals)
                    sscanf(buffer, "f %d//%d %d//%d %d//%d", &va, &vna, &vb, &vnb, &vc, &vnc);
                //broken topology ignored
                if ((va == vb) || (va == vc) || (vb == vc))
                    continue;
                //incomplete line ignored
                if ((va == 0) || (vb == 0) || (vc == 0))
                    continue;
                //vertices
                output[meshIndex].vertices.push_back(vertices[va - 1]);
                output[meshIndex].vertices.push_back(vertices[vb - 1]);
                output[meshIndex].vertices.push_back(vertices[vc - 1]);
                //selector
                output[meshIndex].colors.push_back(0);
                output[meshIndex].colors.push_back(0);
                output[meshIndex].colors.push_back(0);
                //uvs
                if (hasCoords) {
                    output[meshIndex].uv.push_back(uvs[vta - 1]);
                    output[meshIndex].uv.push_back(uvs[vtb - 1]);
                    output[meshIndex].uv.push_back(uvs[vtc - 1]);
                }
                //normals
                if (hasNormals) {
                    output[meshIndex].normals.push_back(normals[vna - 1]);
                    output[meshIndex].normals.push_back(normals[vnb - 1]);
                    output[meshIndex].normals.push_back(normals[vnc - 1]);
                }
                //create new model if it is already too big
                if (output[meshIndex].vertices.size() >= subdivision * 3) {
                    meshIndex = output.size();
                    output.push_back(Mesh());
                    output[meshIndex].image = images[lastKey];
                    output[meshIndex].imageOwner = false;
                }
            }
        }
        std::vector<glm::vec3>().swap(vertices);
        std::vector<glm::vec3>().swap(normals);
        std::vector<glm::vec2>().swap(uvs);
    }

    void File3d::ParsePLYFaces(int subdivision, std::vector<Mesh> &output) {
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
                output.push_back(Mesh());

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

    void File3d::ReadPLYVertices() {
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

    void File3d::ReadHeader() {
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
                std::string sbuf = buffer;
                while(!sbuf.empty() && isspace(sbuf[0])) {
                    sbuf = sbuf.substr(1);
                }
                if (StartsWith(sbuf, "mtllib")) {
                    sscanf(sbuf.c_str(), "mtllib %s", mtlFile);
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
                std::string sbuf = buffer;
                while(!sbuf.empty() && isspace(sbuf[0])) {
                    sbuf = sbuf.substr(1);
                }
                if (StartsWith(sbuf, "newmtl")) {
                    sscanf(sbuf.c_str(), "newmtl %s", key);
                }
                if (StartsWith(sbuf, "Kd")) {
                    glm::vec3 color;
                    sscanf(sbuf.c_str(), "Kd %f %f %f", &color.r, &color.g, &color.b);
                    keyToColor[std::string(key)] = color;
                }
                if (StartsWith(sbuf, "map_Kd")) {
                    sscanf(sbuf.c_str(), "map_Kd %s", pngFile);
                    keyToFile[std::string(key)] = data + pngFile;
                }
            }
            fclose(mtl);
        } else
            assert(false);
    }

    unsigned int File3d::ScanDec(char *line, int offset) {
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

    bool File3d::StartsWith(std::string s, std::string e) {
        if (s.size() >= e.size())
        if (s.substr(0, e.size()).compare(e) == 0)
            return true;
        return false;
    }

    void File3d::WriteHeader(std::vector<Mesh>& model) {
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
                if (model[i].vertices.empty())
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
                //write texture name
                std::string name = model[i].image->GetName();
                fileToIndex[name] = i;

                int start = 0;
                for (unsigned int j = 0; j < name.size(); j++)
                {
                    char c = name[j];
                    if (c == '/')
                        start = j + 1;
                }
                fprintf(mtl, "map_Kd %s\n\n", name.substr(start, name.size()).c_str());
            }
            fclose(mtl);
        }
    }

    void File3d::WritePointCloud(Mesh& mesh, int size) {
        glm::vec3 v;
        glm::vec3 n;
        glm::vec2 t;
        glm::ivec3 c;
        for(unsigned int j = 0; j < size; j++) {
            v = mesh.vertices[j];
            if (type == PLY) {
                c = DecodeColor(mesh.colors[j]);
                fprintf(file, "%f %f %f %d %d %d\n", -v.x, v.z, v.y, c.r, c.g, c.b);
            } else if (type == OBJ) {
                n = mesh.normals[j];
                t = mesh.uv[j];
                fprintf(file, "v %f %f %f\n", v.x, v.y, v.z);
                fprintf(file, "vn %f %f %f\n", n.x, n.y, n.z);
                fprintf(file, "vt %f %f\n", t.x, t.y);
            }
        }
    }

    void File3d::WriteFaces(Mesh& mesh, int offset) {
        glm::ivec3 i;
        for (unsigned int j = 0; j < mesh.vertices.size(); j+=3) {
            i.x = j + 0 + offset;
            i.y = j + 1 + offset;
            i.z = j + 2 + offset;
            if (type == PLY)
                fprintf(file, "3 %d %d %d\n", i.x, i.y, i.z);
            else if (type == OBJ)
                fprintf(file, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", i.x, i.x, i.x, i.y, i.y, i.y, i.z, i.z, i.z);
        }
    }
}