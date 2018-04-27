#include <png.h>
#include <turbojpeg.h>
#include "data/image.h"
#include "gl/opengl.h"

#define JPEG_QUALITY 85

FILE* temp;
void png_read_file(png_structp, png_bytep data, png_size_t length)
{
    fread(data, length, 1, temp);
}

tjhandle jpegC = tjInitCompress();
tjhandle jpegD = tjInitDecompress();
unsigned char* srcPlanes[3] = {0, 0, 0};
unsigned char* graySrcPlanes[3] = {0, 0, 0};

std::vector<long> image_textureToDelete;

namespace oc {

    Image::Image(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
        width = 1;
        height = 1;
        data = new unsigned char[4];
        data[0] = r;
        data[1] = g;
        data[2] = b;
        data[3] = a;
        char buffer[1024];
        sprintf(buffer, "%d-%d-%d-%d.png", r, g, b, a);
        name = std::string(buffer);
        instances = 1;
        texture = -1;
    }

    Image::Image(unsigned char* src, int w, int h, int scale) {
        data = new unsigned char[(w / scale) * (h / scale) * 4];
        width = w / scale;
        height = h / scale;
        name = "photo";
        instances = 1;
        texture = -1;
        UpdateYUV(src, w, h, scale);
    }

    Image::Image(std::string filename) {
        LOGI("Reading %s", filename.c_str());
        name = filename;
        instances = 1;
        texture = -1;

        std::string ext = filename.substr(filename.size() - 3, filename.size() - 1);
        if (ext.compare("jpg") == 0)
            ReadJPG(filename);
        else if (ext.compare("png") == 0)
            ReadPNG(filename);
        else {
            data = new unsigned char[4];
            data[0] = 255;
            data[1] = 0;
            data[2] = 255;
            data[3] = 255;
        }
    }

    Image::~Image() {
        delete[] data;
        if (srcPlanes[1])
            delete srcPlanes[1];
        if (srcPlanes[2])
            delete srcPlanes[2];
        srcPlanes[1] = 0;
        srcPlanes[2] = 0;
        if (graySrcPlanes[1])
            delete graySrcPlanes[1];
        if (graySrcPlanes[2])
            delete graySrcPlanes[2];
        graySrcPlanes[1] = 0;
        graySrcPlanes[2] = 0;
    }

    unsigned char* Image::ExtractYUV(unsigned int s) {
        int yIndex = 0;
        unsigned int uvIndex = width * s * height * s;
        int R, G, B, Y, U, V;
        int index = 0;
        unsigned int xRGBIndex, yRGBIndex;
        unsigned char* output = new unsigned char[uvIndex * 2];
        for (int y = 0; y < height * s; y++) {
            xRGBIndex = 0;
            yRGBIndex = (y / s) * width * 4;
            for (unsigned int x = 0; x < width; x++) {
                B = data[yRGBIndex + xRGBIndex++];
                G = data[yRGBIndex + xRGBIndex++];
                R = data[yRGBIndex + xRGBIndex++];
                xRGBIndex++;

                //RGB to YUV algorithm
                Y = ( (  66 * R + 129 * G +  25 * B + 128) >> 8) +  16;
                V = ( ( -38 * R -  74 * G + 112 * B + 128) >> 8) + 128;
                U = ( ( 112 * R -  94 * G -  18 * B + 128) >> 8) + 128;

                for (int xs = 0; xs < s; xs++) {
                    output[yIndex++] = (uint8_t) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
                    if (y % 2 == 0 && index % 2 == 0) {
                        output[uvIndex++] = (uint8_t)((V<0) ? 0 : ((V > 255) ? 255 : V));
                        output[uvIndex++] = (uint8_t)((U<0) ? 0 : ((U > 255) ? 255 : U));
                    }
                    index++;
                }
            }
        }
        return output;
    }

    void Image::UpdateTexture() {
        image_textureToDelete.push_back(texture);
        texture = -1;
    }

    void Image::UpdateYUV(unsigned char *src, int w, int h, int scale) {
        int index = 0;
        int frameSize = w * h;
        for (int y = 0; y < h; y+=scale) {
            for (int x = 0; x < w; x+=scale) {
                int UVIndex = frameSize + 2*(x/2) + (y/2) * w;
                int Y = src[y*w + x] & 0xff;
                float U = (float)(src[UVIndex + 0] & 0xff) - 128.0f;
                float V = (float)(src[UVIndex + 1] & 0xff) - 128.0f;

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
                data[index++] = (unsigned char) B;
                data[index++] = (unsigned char) G;
                data[index++] = (unsigned char) R;
                data[index++] = 255;
            }
        }
    }

    void Image::Write(std::string filename) {
        std::string ext = filename.substr(filename.size() - 3, filename.size() - 1);
        if (ext.compare("jpg") == 0)
            WriteJPG(filename);
        else if (ext.compare("png") == 0)
            WritePNG(filename);
        else
            assert(false);
    }

    void Image::ReadJPG(std::string filename) {
        //get file size
        temp = fopen(filename.c_str(), "rb");
        fseek(temp, 0, SEEK_END);
        unsigned long size = (unsigned long) ftell(temp);
        rewind(temp);

        //read compressed data
        unsigned char* src = new unsigned char[size];
        fread(src, 1, size, temp);
        fclose(temp);

        //read header of compressed data
        int sub;
        tjDecompressHeader2(jpegD, src, size, &width, &height, &sub);
        data = new unsigned char[width * height * 4];

        //decompress data
        tjDecompress2(jpegD, src, size, data, width, 0, height, TJPF_RGBA, TJFLAG_FASTDCT);
        delete[] src;
    }

    void Image::ReadPNG(std::string filename) {

        /// init PNG library
        temp = fopen(filename.c_str(), "r");
        unsigned int sig_read = 0;
        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info_ptr = png_create_info_struct(png_ptr);
        setjmp(png_jmpbuf(png_ptr));
        png_set_read_fn(png_ptr, NULL, png_read_file);
        png_set_sig_bytes(png_ptr, sig_read);
        png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16, NULL);
        int bit_depth, color_type, interlace_type;
        png_uint_32 w, h;
        png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, &interlace_type, NULL, NULL);
        width = (int) w;
        height = (int) h;

        /// load PNG
        png_size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);
        png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
        data = new unsigned char[4 * w * h];
        switch (color_type) {
            case PNG_COLOR_TYPE_RGBA:
                for (int i = 0; i < h; i++)
                    memcpy(data+(row_bytes * i), row_pointers[i], row_bytes);
                break;
            case PNG_COLOR_TYPE_RGB:
                for (unsigned int i = 0; i < height; i++)
                    for (unsigned int j = 0; j < w; j++) {
                        data[4 * (i * w + j) + 0] = row_pointers[i][j * 3 + 0];
                        data[4 * (i * w + j) + 1] = row_pointers[i][j * 3 + 1];
                        data[4 * (i * w + j) + 2] = row_pointers[i][j * 3 + 2];
                        data[4 * (i * w + j) + 3] = 255;
                    }
                break;
            case PNG_COLOR_TYPE_PALETTE:
                int num_palette;
                png_colorp palette;
                png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
                for (unsigned int i = 0; i < h; i++)
                    for (unsigned int j = 0; j < row_bytes; j++) {
                        data[4 * (i * row_bytes + j) + 0] = palette[row_pointers[i][j]].red;
                        data[4 * (i * row_bytes + j) + 1] = palette[row_pointers[i][j]].green;
                        data[4 * (i * row_bytes + j) + 2] = palette[row_pointers[i][j]].blue;
                        data[4 * (i * row_bytes + j) + 3] = 255;
                    }
            break;
            case PNG_COLOR_TYPE_GRAY:
                for (unsigned int i = 0; i < height; i++)
                    for (unsigned int j = 0; j < row_bytes; j++) {
                        for (unsigned int k = 0; k < 3; k++)
                            data[4 * (i * row_bytes + j) + k] = row_pointers[i][j];
                        data[4 * (i * row_bytes + j) + 3] = 255;
                    }
                break;
            case PNG_COLOR_MASK_ALPHA:
                for (int i = 0; i < height; i++)
                    for (unsigned int j = 0; j < row_bytes; j++) {
                        for (unsigned int k = 0; k < 3; k++)
                            data[4 * (i * row_bytes + j) + k] = 0;
                        data[4 * (i * row_bytes + j) + 3] = row_pointers[i][j];
                }
                break;
            default:
                LOGE("Unsupported PNG type: %d", color_type);
                assert(false);
                break;
        }
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(temp);
    }

    void Image::WriteJPG(std::string filename) {
        //compress data
        long unsigned int size = 0;
        unsigned char* dst = NULL;
        tjCompress2(jpegC, data, width, 0, height, TJPF_RGBA, &dst, &size, TJSAMP_444, JPEG_QUALITY, TJFLAG_FASTDCT);

        //write data into file
        temp = fopen(filename.c_str(), "wb");
        fwrite(dst, 1, size, temp);
        fclose(temp);
        tjFree(dst);
    }

    void Image::WritePNG(std::string filename) {
        // Open file for writing (binary mode)
        temp = fopen(filename.c_str(), "wb");

        // init PNG library
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info_ptr = png_create_info_struct(png_ptr);
        setjmp(png_jmpbuf(png_ptr));
        png_init_io(png_ptr, temp);
        png_set_IHDR(png_ptr, info_ptr, (png_uint_32) width, (png_uint_32) height,
                     8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(png_ptr, info_ptr);

        // write image data
        png_bytep row = (png_bytep) malloc(4 * width * sizeof(png_byte));
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                row[x * 4 + 0] = data[(y * width + x) * 4 + 0];
                row[x * 4 + 1] = data[(y * width + x) * 4 + 1];
                row[x * 4 + 2] = data[(y * width + x) * 4 + 2];
                row[x * 4 + 3] = data[(y * width + x) * 4 + 3];
            }
            png_write_row(png_ptr, row);
        }
        png_write_end(png_ptr, NULL);

        /// close all
        if (temp != NULL) fclose(temp);
        if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
        if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        if (row != NULL) free(row);
    }

    void Image::JPG2YUV(std::string filename, unsigned char* data, int width, int height) {
        //get file size
        temp = fopen(filename.c_str(), "rb");
        fseek(temp, 0, SEEK_END);
        int size = (int) ftell(temp);
        rewind(temp);

        //read compressed data
        unsigned char* src = new unsigned char[size];
        fread(src, 1, (size_t) size, temp);
        fclose(temp);

        //decompress data
        int offset, offset2, x;
        tjDecompressToYUV2(jpegD, src, (unsigned long) size, data, width, 4, height, TJFLAG_FASTDCT);
        size = width * height;
        for (unsigned int y = 0; y < height / 2; y++) {
            offset = size + y * width;
            offset2 = size + y * 2 * width;
            memcpy(data + offset, data + offset2, (size_t) width);
            for (x = 0; x < width; x += 2)
                data[offset + x] = data[size + offset2 + x];
        }
        delete[] src;
    }

    void Image::YUV2JPG(unsigned char *data, int width, int height, std::string filename, bool gray) {
        //compress data
        long unsigned int size = (unsigned long) (width * height);
        unsigned char* dst = NULL;
        int strides[3];
        strides[0] = width;
        strides[1] = width;
        strides[2] = width;
        if (gray)
        {
            graySrcPlanes[0] = data;
            if (!graySrcPlanes[1]) {
                graySrcPlanes[1] = new unsigned char[size];
                for (unsigned int i = 0; i < size; i++)
                    graySrcPlanes[1][i] = 0;
            }
            if (!graySrcPlanes[2]) {
                graySrcPlanes[2] = new unsigned char[size];
                for (unsigned int i = 0; i < size; i++)
                    graySrcPlanes[2][i] = 0;
            }
#ifdef ANDROID
            tjCompressFromYUVPlanes(jpegC, graySrcPlanes, width, strides, height, TJ_GRAYSCALE, &dst, &size, JPEG_QUALITY, TJFLAG_FASTDCT);
#else
            tjCompressFromYUVPlanes(jpegC, (const unsigned char**)graySrcPlanes, width, strides, height, TJ_GRAYSCALE, &dst, &size, JPEG_QUALITY, TJFLAG_FASTDCT);
#endif
        } else {
            srcPlanes[0] = data;
            if (!srcPlanes[1])
                srcPlanes[1] = new unsigned char[size];
            if (!srcPlanes[2])
                srcPlanes[2] = new unsigned char[size];
            int len = 0;
            int UV = height - 1;
            for (int y = height / 2 - 1; y >= 0; y--) {
                memcpy(srcPlanes[1] + width * UV, data + size + width * y, (size_t) width);
                memcpy(srcPlanes[2] + width * UV, data + size + width * y, (size_t) width);
                len = width * UV + width;
                for (int x = width * UV; x < len; x += 2) {
                    srcPlanes[1][x] = srcPlanes[1][x + 1];
                    srcPlanes[2][x + 1] = srcPlanes[2][x];
                }
                UV--;
                memcpy(srcPlanes[1] + width * UV, data + size + width * y, (size_t) width);
                memcpy(srcPlanes[2] + width * UV, data + size + width * y, (size_t) width);
                len = width * UV + width;
                for (int x = width * UV; x < len; x += 2) {
                    srcPlanes[1][x] = srcPlanes[1][x + 1];
                    srcPlanes[2][x + 1] = srcPlanes[2][x];
                }
                UV--;
            }
#ifdef ANDROID
            tjCompressFromYUVPlanes(jpegC, srcPlanes, width, strides, height, TJSAMP_444, &dst, &size, JPEG_QUALITY, TJFLAG_FASTDCT);
#else
            tjCompressFromYUVPlanes(jpegC, (const unsigned char**)srcPlanes, width, strides, height, TJSAMP_444, &dst, &size, JPEG_QUALITY, TJFLAG_FASTDCT);
#endif
        }

        //write data into file
        temp = fopen(filename.c_str(), "wb");
        fwrite(dst, 1, size, temp);
        fclose(temp);
        tjFree(dst);
    }


    std::vector<unsigned int> Image::TexturesToDelete() {
        std::vector<unsigned int> output;
        for (long & i : image_textureToDelete)
            output.push_back((const unsigned int &) i);
        image_textureToDelete.clear();
        return output;
    }
}
