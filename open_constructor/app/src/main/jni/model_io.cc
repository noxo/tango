#include <tango_3d_reconstruction_api.h>
#include "model_io.h"
#include <sstream>

FILE* temp;
void png_read_file(png_structp, png_bytep data, png_size_t length)
{
    fread(data, length, 1, temp);
}

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
        if (type != ModelIO::OBJ)
            fclose(file);
    }

    std::vector<TextureToLoad> ModelIO::readModel(int subdivision, std::vector<tango_gl::StaticMesh>& output) {
        assert(!writeMode);
        std::vector<TextureToLoad> textures = readHeader();
        if (type == PLY) {
            readPLYVertices();
            parsePLYFaces(subdivision, output);
        } else if (type == OBJ) {
            parseOBJ(output);
        } else
            assert(false);
        return textures;
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
                writePointCloud(model[i], vectorSize[i]);
            int offset = 0;
            if (type == OBJ)
                offset++;
            int texture = -1;
            for (unsigned int i = 0; i < model.size(); i++) {
                if (type == OBJ) {
                    if (texture != model[i]->mesh.texture) {
                        texture = model[i]->mesh.texture;
                        fprintf(file, "usemtl %d\n", texture);
                    }
                }
                writeFaces(model[i], offset);
                offset += vectorSize[i];
            }
        } else
            assert(false);
    }

    void ModelIO::parseOBJ(std::vector<tango_gl::StaticMesh> &output) {
        char buffer[1024];
        unsigned long meshIndex = 0;
        glm::vec3 v;
        glm::vec2 t;
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec2> uvs;
        unsigned int a, aa, b, bb, c, cc;
        while (true) {
            if (!fgets(buffer, 1024, file))
                break;
            if (buffer[0] == 'u') {
                output.push_back(tango_gl::StaticMesh());
                meshIndex = output.size() - 1;
                output[meshIndex].render_mode = GL_TRIANGLES;
                output[meshIndex].texture = (int32_t) meshIndex;
            } else if ((buffer[0] == 'v') && (buffer[1] == ' ')) {
                sscanf(buffer, "v %f %f %f", &v.x, &v.y, &v.z);
                vertices.push_back(v);
            } else if ((buffer[0] == 'v') && (buffer[1] == 't')) {
                sscanf(buffer, "vt %f %f", &t.x, &t.y);
                uvs.push_back(t);
            } else if ((buffer[0] == 'f') && (buffer[1] == ' ')) {
                sscanf(buffer, "f %d/%d %d/%d %d/%d", &a, &aa, &b, &bb, &c, &cc);
                //broken topology ignored
                if ((a == b) || (a == c) || (b == c))
                    continue;
                output[meshIndex].vertices.push_back(vertices[a - 1]);
                output[meshIndex].uv.push_back(uvs[aa - 1]);
                output[meshIndex].vertices.push_back(vertices[b - 1]);
                output[meshIndex].uv.push_back(uvs[bb - 1]);
                output[meshIndex].vertices.push_back(vertices[c - 1]);
                output[meshIndex].uv.push_back(uvs[cc - 1]);
            }
        }
    }

    void ModelIO::parsePLYFaces(int subdivision, std::vector<tango_gl::StaticMesh> &output) {
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

                output.push_back(tango_gl::StaticMesh());
                unsigned long meshIndex = output.size() - 1;
                output[meshIndex].render_mode = GL_TRIANGLES;
                output[meshIndex].texture = -1;

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

    void ModelIO::readPLYVertices() {
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

    std::vector<TextureToLoad> ModelIO::readHeader() {
        std::vector<TextureToLoad> output;
        char buffer[1024];
        if (type == PLY) {
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
        } else if (type == OBJ) {
            char mtlFile[1024];
            while (true) {
                if (!fgets(buffer, 1024, file))
                    break;
                if (startsWith(buffer, "mtllib")) {
                    sscanf(buffer, "mtllib %s", mtlFile);
                    break;
                }
            }
            unsigned long index = 0;
            for (unsigned long i = 0; i < path.size(); i++) {
                if (path[i] == '/')
                    index = i;
            }
            std::string data = path.substr(0, index + 1);
            FILE* mtl = fopen((data + mtlFile).c_str(), "r");
            while (true) {
                char pngFile[1024];
                if (!fgets(buffer, 1024, mtl))
                    break;
                if (startsWith(buffer, "map_Kd")) {
                    sscanf(buffer, "map_Kd %s", pngFile);
                    output.push_back(readPNG(data + pngFile));
                }
            }
            fclose(mtl);
        } else
            assert(false);
        return output;
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
            int texture = -1;
            for (unsigned int i = 0; i < model.size(); i++) {
                if (texture != model[i]->mesh.texture) {
                    texture = model[i]->mesh.texture;
                } else
                    continue;
                std::ostringstream srcPath;
                srcPath << dataset.c_str();
                srcPath << "/frame_";
                srcPath << texture;
                srcPath << ".png";
                std::ostringstream dstPath;
                dstPath << base.c_str();
                dstPath << "_";
                dstPath << texture;
                dstPath << ".png";
                LOGI("Moving %s to %s", srcPath.str().c_str(), dstPath.str().c_str());
                rename(srcPath.str().c_str(), dstPath.str().c_str());

                fprintf(mtl, "newmtl %d\n", texture);
                fprintf(mtl, "Ns 96.078431\n");
                fprintf(mtl, "Ka 1.000000 1.000000 1.000000\n");
                fprintf(mtl, "Kd 0.640000 0.640000 0.640000\n");
                fprintf(mtl, "Ks 0.500000 0.500000 0.500000\n");
                fprintf(mtl, "Ke 0.000000 0.000000 0.000000\n");
                fprintf(mtl, "Ni 1.000000\n");
                fprintf(mtl, "d 1.000000\n");
                fprintf(mtl, "illum 2\n");
                fprintf(mtl, "map_Kd %s_%d.png\n\n", shortBase.c_str(), texture);
            }
            fclose(mtl);
        }
    }

    void ModelIO::writePointCloud(SingleDynamicMesh *mesh, int size) {
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

    TextureToLoad ModelIO::readPNG(std::string file)
    {
        temp = fopen(file.c_str(), "r");
        TextureToLoad texture;
        unsigned int sig_read = 0;

        /// init PNG library
        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info_ptr = png_create_info_struct(png_ptr);
        setjmp(png_jmpbuf(png_ptr));
        png_set_read_fn(png_ptr, NULL, png_read_file);
        png_set_sig_bytes(png_ptr, sig_read);
        png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16, NULL);
        int bit_depth, color_type, interlace_type;
        png_uint_32 width, height;
        png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

        /// load PNG
        png_size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);
        texture.data = new unsigned char[row_bytes * height];
        png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
        for (unsigned int i = 0; i < height; i++)
            memcpy(texture.data+(row_bytes * i), row_pointers[i], row_bytes);

        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(temp);
        texture.width = (int) width;
        texture.height = (int) height;
        return texture;
    }
} // namespace mesh_builder