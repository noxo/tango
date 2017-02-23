#include <stdio.h>
#include <png.h>
#include <cstdlib>
#include <tango_3d_reconstruction_api.h>
#include <string>
#include <sstream>
#include <tango-gl/util.h>
#include "texture_processor.h"
#include "mask_processor.h"

FILE* temp;
void png_read_file(png_structp, png_bytep data, png_size_t length)
{
    fread(data, length, 1, temp);
}

namespace mesh_builder {
    TextureProcessor::TextureProcessor() : lastTextureIndex(-1) {}

    TextureProcessor::~TextureProcessor() {
        for (RGBImage t : images) {
            delete[] t.data;
        }
        for (unsigned int i : textureMap) {
            glDeleteTextures(1, &i);
        }
    }

    void TextureProcessor::Add(Tango3DR_ImageBuffer t3dr_image) {
        RGBImage t = YUV2RGB(t3dr_image, 4);
        mutex.lock();
        int found = -1;
        for (unsigned int i = 0; i < instances.size(); i++)
            if (instances[i].empty())
                found = i;
        if (found >= 0) {
            delete[] images[found].data;
            images[found] = t;
            lastTextureIndex = found;
            toLoad[found] = true;
        } else {
            toLoad[images.size()] = true;
            lastTextureIndex = (int) images.size();
            images.push_back(t);
            instances.push_back(std::vector<SingleDynamicMesh*>());
        }
        mutex.unlock();
    }

    void TextureProcessor::Add(std::vector<std::string> pngFiles) {
        for (unsigned int i = 0; i < pngFiles.size(); i++) {
            RGBImage t = ReadPNG(pngFiles[i]);
            mutex.lock();
            toLoad[images.size()] = true;
            images.push_back(t);
            instances.push_back(std::vector<SingleDynamicMesh*>());
            mutex.unlock();
        }
    }

    void TextureProcessor::ApplyInstance(SingleDynamicMesh* mesh) {
        mutex.lock();
        mesh->mesh.texture = lastTextureIndex;
        instances[lastTextureIndex].push_back(mesh);
        toUpdate[lastTextureIndex] = true;
        mutex.unlock();
    }

    void TextureProcessor::RemoveInstance(SingleDynamicMesh* mesh) {
        mutex.lock();
        for (int i = (int) (instances[mesh->mesh.texture].size() - 1); i >= 0; i--)
            if (instances[mesh->mesh.texture][i] == mesh)
                instances[mesh->mesh.texture].erase(instances[mesh->mesh.texture].begin() + i);
        mutex.unlock();
    }

    void TextureProcessor::Save(std::string modelPath) {
        mutex.lock();
        std::string base = (modelPath.substr(0, modelPath.length() - 4));
        for (unsigned int i = 0; i < images.size(); i++) {
            std::ostringstream ostr;
            ostr << base.c_str();
            ostr << "_";
            ostr << i;
            ostr << ".png";
            if (!instances.empty()) {
                LOGI("Saving %s", ostr.str().c_str());
                WritePNG(ostr.str().c_str(), images[i]);
            }
        }
        mutex.unlock();
    }

    std::vector<unsigned int> TextureProcessor::TextureMap() {
        mutex.lock();
        std::vector<unsigned int> output = textureMap;
        mutex.unlock();
        return output;
    }

    bool TextureProcessor::UpdateGL() {
        mutex.lock();
        bool updated = !toLoad.empty();
        for(std::pair<const int, bool> i : toLoad ) {
            while (i.first >= textureMap.size()) {
                GLuint textureID;
                glGenTextures(1, &textureID);
                textureMap.push_back(textureID);
            }
            RGBImage t = images[i.first];
            glBindTexture(GL_TEXTURE_2D, textureMap[i.first]);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t.width, t.height, 0, GL_RGB, GL_UNSIGNED_BYTE, t.data);
        }
        toLoad.clear();
        mutex.unlock();
        return updated;
    }

    void TextureProcessor::UpdateTextures() {
        //Merging textures by bounding boxes
        for (std::pair<const int, bool> i : toUpdate) {
            MaskUnused(i.first);
            boundaries[i.first] = GetAABB(i.first);
            holes[i.first] = FindAABB(i.first);
        }
        for (std::pair<const int, bool> i : toUpdate) {
            for (std::pair<const int, glm::ivec4> j : boundaries) {
                if (i.first == j.first)
                    continue;
                if (images[i.first].width != images[j.first].width)
                    continue;
                if (images[i.first].height != images[j.first].height)
                    continue;
                glm::ivec4 bi = boundaries[i.first];
                glm::ivec4 bj = j.second;
                int h = images[i.first].width - (bi.z - bi.x + bj.z - bj.x);
                int v = images[i.first].height - (bi.w - bi.y + bj.w - bj.y);
                if ((h > 0) && (h > v)) {
                    Translate(i.first, -bi.x, -bi.y);
                    Translate(j.first, -bj.x + bi.z - bi.x, -bj.y);
                    Merge(i.first, j.first);
                    toUpdate.erase(j.first);
                    break;
                } else if ((v > 0) && (v > h)) {
                    Translate(i.first, -bi.x, -bi.y);
                    Translate(j.first, -bj.x, -bj.y + bi.w - bi.y);
                    Merge(i.first, j.first);
                    toUpdate.erase(j.first);
                    break;
                }
            }
        }
        //Merging textures by holes
        for (std::pair<const int, bool> i : toUpdate) {
            mutex.lock();
            bool empty = instances[i.first].empty();
            mutex.unlock();
            if (empty)
                continue;
            for (std::pair<const int, glm::ivec4> j : holes) {
                mutex.lock();
                empty = instances[j.first].empty();
                mutex.unlock();
                if (empty)
                    continue;
                if (i.first == j.first)
                    continue;
                if (images[i.first].width != images[j.first].width)
                    continue;
                if (images[i.first].height != images[j.first].height)
                    continue;

                glm::ivec4 bi = boundaries[i.first];
                glm::ivec4 bj = j.second;
                if (bi.z - bi.x <= bj.z - bj.x) {
                    if (bi.w - bi.y <= bj.w - bj.y) {
                        Translate(i.first, bj.x - bi.x, bj.y - bi.y);
                        Merge(j.first, i.first);
                    }
                }
            }
        }
        toUpdate.clear();
    }

    glm::ivec4 TextureProcessor::FindAABB(int index) {
        int w = images[index].width;
        int h = images[index].height;
        unsigned char* data = images[index].data;
        int i = 0;
        int r, g, b;
        std::vector<glm::ivec2> spaces;
        for (int y = 0; y < h; y++) {
            int minx = INT_MIN;
            int maxx = INT_MIN;
            int tminx = INT_MIN;
            int tmaxx = INT_MIN;
            bool space = false;
            for (int x = 0; x < w; x++) {
                r = data[i++];
                g = data[i++];
                b = data[i++];
                if ((r == 255) && (g == 0) && (b == 255)) {
                    if (!space)
                        tminx = x;
                    tmaxx = x;
                    space = true;
                }
                else
                    space = false;
                if (maxx - minx < tmaxx - tminx) {
                    minx = tminx;
                    maxx = tmaxx;
                }
            }
            spaces.push_back(glm::ivec2(minx, maxx));
        }
        long area;
        long maxArea = 0;
        int yp, ym;
        glm::ivec2 d, p;
        glm::ivec4 output;
        for (int y = 0; y < h; y++) {
            p = spaces[y];
            if (p.x == INT_MIN)
                continue;
            for (int x = p.x; x < p.y; x++) {
                for (ym = y; ym > 0;) {
                    d = spaces[ym];
                    if ((d.x == INT_MIN) || (d.x > p.x) || (d.y < x))
                        break;
                    else
                        ym--;
                }
                for (yp = y; yp < h - 1;) {
                    glm::ivec2 d = spaces[yp];
                    if ((d.x == INT_MIN) || (d.x > p.x) || (d.y < x))
                        break;
                    else
                        yp++;
                }
                area = (yp - ym) * (x - p.x);
                if (maxArea < area) {
                    maxArea = area;
                    output.x = p.x;
                    output.y = ym;
                    output.z = x;
                    output.w = yp;
                }
            }
        }
        return output;
    }

    glm::ivec4 TextureProcessor::GetAABB(int index) {
        int w = images[index].width;
        int h = images[index].height;
        unsigned char* data = images[index].data;

        glm::ivec4 output;
        output.x = INT_MAX;
        output.y = INT_MAX;
        output.z = INT_MIN;
        output.w = INT_MIN;

        int i = 0;
        int r, g, b;
        bool empty = true;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                r = data[i++];
                g = data[i++];
                b = data[i++];
                if ((r != 255) && (g != 0) && (b != 255)) {
                    if (output.x > x)
                        output.x = x;
                    if (output.y > y)
                        output.y = y;
                    if (output.z < x)
                        output.z = x;
                    if (output.w < y)
                        output.w = y;
                    empty = false;
                }
            }
        }
        if (empty)
            return glm::ivec4(0, 0, 0, 0);
        else
            return output;
    }

    void TextureProcessor::MaskUnused(int index) {
        int w = images[index].width;
        int h = images[index].height;
        unsigned char* data = images[index].data;
        MaskProcessor mp(instances[index], w, h);
        int i = 0;
        for (int y = h - 1; y >= 0; y--) {
            for (int x = 0; x < w; x++) {
                if(mp.isMasked(x, y)) {
                    data[i++] = 255;
                    data[i++] = 0;
                    data[i++] = 255;
                } else
                    i+=3;
            }
        }
        mutex.lock();
        toLoad[index] = true;
        mutex.unlock();
    }

    void TextureProcessor::Merge(int dst, int src) {
        //texture merging
        int r, g, b;
        for (int i = 0; i < images[dst].width * images[dst].height * 3; i+=3) {
            r = images[dst].data[i + 0];
            g = images[dst].data[i + 1];
            b = images[dst].data[i + 2];
            if ((r == 255) && (g == 0) && (b == 255)) {
                images[dst].data[i + 0] = images[src].data[i + 0];
                images[dst].data[i + 1] = images[src].data[i + 1];
                images[dst].data[i + 2] = images[src].data[i + 2];
            }
        }

        //update meshes
        for (SingleDynamicMesh* mesh : instances[src]) {
            mesh->mutex.lock();
            mesh->mesh.texture = dst;
            mesh->mutex.unlock();
            instances[dst].push_back(mesh);
        }
        instances[src].clear();

        //update texture variables
        MaskUnused(dst);
        boundaries[dst] = GetAABB(dst);
        holes[dst] = FindAABB(dst);
        mutex.lock();
        toLoad[dst] = true;
        mutex.unlock();
    }

    RGBImage TextureProcessor::ReadPNG(std::string file)
    {
        temp = fopen(file.c_str(), "r");
        RGBImage texture;
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

    void TextureProcessor::Translate(int index, int mx, int my) {
        int w = images[index].width;
        int h = images[index].height;
        unsigned char* data = images[index].data;
        //right and left shifting
        for (int y = 0; y < h; y++) {
            if (mx > 0) {
                for (int x = w - 1; x >= mx; x--) {
                    data[(y * w + x) * 3 + 0] = data[(y * w + x - mx) * 3 + 0];
                    data[(y * w + x) * 3 + 1] = data[(y * w + x - mx) * 3 + 1];
                    data[(y * w + x) * 3 + 2] = data[(y * w + x - mx) * 3 + 2];
                }
                for (int x = mx - 1; x >= 0; x--) {
                    data[(y * w + x) * 3 + 0] = 255;
                    data[(y * w + x) * 3 + 1] = 0;
                    data[(y * w + x) * 3 + 2] = 255;
                }
            } else if (mx < 0) {
                for (int x = 0; x < w + mx; x++) {
                    data[(y * w + x) * 3 + 0] = data[(y * w + x - mx) * 3 + 0];
                    data[(y * w + x) * 3 + 1] = data[(y * w + x - mx) * 3 + 1];
                    data[(y * w + x) * 3 + 2] = data[(y * w + x - mx) * 3 + 2];
                }
                for (int x = w + mx; x < w; x++) {
                    data[(y * w + x) * 3 + 0] = 255;
                    data[(y * w + x) * 3 + 1] = 0;
                    data[(y * w + x) * 3 + 2] = 255;
                }
            }
        }
        //up and down shifting
        for (int x = 0; x < w; x++) {
            if (my > 0) {
                for (int y = h - 1; y >= my; y--) {
                    data[(y * w + x) * 3 + 0] = data[((y - my) * w + x) * 3 + 0];
                    data[(y * w + x) * 3 + 1] = data[((y - my) * w + x) * 3 + 1];
                    data[(y * w + x) * 3 + 2] = data[((y - my) * w + x) * 3 + 2];
                }
                for (int y = my - 1; y >= 0; y--) {
                    data[(y * w + x) * 3 + 0] = 255;
                    data[(y * w + x) * 3 + 1] = 0;
                    data[(y * w + x) * 3 + 2] = 255;
                }
            } else if (my < 0) {
                for (int y = 0; y < h + my; y++) {
                    data[(y * w + x) * 3 + 0] = data[((y - my) * w + x) * 3 + 0];
                    data[(y * w + x) * 3 + 1] = data[((y - my) * w + x) * 3 + 1];
                    data[(y * w + x) * 3 + 2] = data[((y - my) * w + x) * 3 + 2];
                }
                for (int y = h + my; y < h; y++) {
                    data[(y * w + x) * 3 + 0] = 255;
                    data[(y * w + x) * 3 + 1] = 0;
                    data[(y * w + x) * 3 + 2] = 255;
                }
            }
        }
        //update texture coordinates
        glm::vec2 offset = glm::vec2(mx / (float)(w - 1), -my / (float)(h - 1));
        for (SingleDynamicMesh* mesh : instances[index]) {
            mesh->mutex.lock();
            for (unsigned int i = 0; i < mesh->mesh.uv.size(); i++)
                mesh->mesh.uv[i] += offset;
            mesh->mutex.unlock();
        }
        toLoad[index] = true;
    }

    void TextureProcessor::WritePNG(const char* filename, RGBImage t) {
        // Open file for writing (binary mode)
        FILE *fp = fopen(filename, "wb");

        // init PNG library
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info_ptr = png_create_info_struct(png_ptr);
        setjmp(png_jmpbuf(png_ptr));
        png_init_io(png_ptr, fp);
        png_set_IHDR(png_ptr, info_ptr, (png_uint_32) t.width, (png_uint_32) t.height,
                     8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(png_ptr, info_ptr);

        // write image data
        png_bytep row = (png_bytep) malloc(3 * t.width * sizeof(png_byte));
        for (int y = 0; y < t.height; y++) {
            for (int x=0; x < t.width; x++) {
                row[x * 3 + 0] = t.data[(y * t.width + x) * 3 + 0];
                row[x * 3 + 1] = t.data[(y * t.width + x) * 3 + 1];
                row[x * 3 + 2] = t.data[(y * t.width + x) * 3 + 2];
            }
            png_write_row(png_ptr, row);
        }
        png_write_end(png_ptr, NULL);

        /// close all
        if (fp != NULL) fclose(fp);
        if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
        if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        if (row != NULL) free(row);
    }

    RGBImage TextureProcessor::YUV2RGB(Tango3DR_ImageBuffer t3dr_image, int scale) {
        if (t3dr_image.format != TANGO_3DR_HAL_PIXEL_FORMAT_YCrCb_420_SP)
            std::exit(EXIT_SUCCESS);
        unsigned char* rgb = new unsigned char[(t3dr_image.width / scale) * (t3dr_image.height / scale) * 3];
        int index = 0;
        int frameSize = t3dr_image.width * t3dr_image.height;
        for (int y = t3dr_image.height - 1; y >= 0; y-=scale) {
            for (int x = 0; x < t3dr_image.width; x+=scale) {
                int xby2 = x/2;
                int yby2 = y/2;
                int UVIndex = frameSize + 2*xby2 + yby2*t3dr_image.width;
                int Y = t3dr_image.data[y*t3dr_image.width + x] & 0xff;
                float U = (float)(t3dr_image.data[UVIndex + 0] & 0xff) - 128.0f;
                float V = (float)(t3dr_image.data[UVIndex + 1] & 0xff) - 128.0f;

                // Do the YUV -> RGB conversion
                float Yf = 1.164f*((float)Y) - 16.0f;
                int R = (int)(Yf + 1.596f*V);
                int G = (int)(Yf - 0.813f*V - 0.391f*U);
                int B = (int)(Yf            + 2.018f*U);

                // Clip rgb values to 0-255
                R = R < 0 ? 0 : R > 255 ? 255 : R;
                G = G < 0 ? 0 : G > 255 ? 255 : G;
                B = B < 0 ? 0 : B > 255 ? 255 : B;

                // Put that pixel in the buffer
                rgb[index++] = (unsigned char) B;
                rgb[index++] = (unsigned char) G;
                rgb[index++] = (unsigned char) R;
            }
        }
        RGBImage t;
        t.width = t3dr_image.width / scale;
        t.height = t3dr_image.height / scale;
        t.data = rgb;
        return t;
    }
}
