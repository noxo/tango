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

namespace oc {

    Image::Image() {
        width = 1;
        height = 1;
        data = new unsigned char[3];
        name = "";
    }
#ifndef NOTANGO
    Image::Image(Tango3DR_ImageBuffer t3dr_image, int scale) {
        data = new unsigned char[(t3dr_image.width / scale) * (t3dr_image.height / scale) * 3];
        int index = 0;
        int frameSize = t3dr_image.width * t3dr_image.height;
        for (int y = 0; y < t3dr_image.height; y+=scale) {
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
                data[index++] = (unsigned char) B;
                data[index++] = (unsigned char) G;
                data[index++] = (unsigned char) R;
            }
        }
        width = t3dr_image.width / scale;
        height = t3dr_image.height / scale;
        name = "photo";
    }
#endif

    Image::Image(std::string filename) {
        LOGI("Reading %s", filename.c_str());
        name = filename;

        std::string ext = filename.substr(filename.size() - 3, filename.size() - 1);
        if (ext.compare("jpg") == 0)
            ReadJPG(filename);
        else if (ext.compare("png") == 0)
            ReadPNG(filename);
        else
            assert(false);
    }

    Image::~Image() {
        delete[] data;
    }

    unsigned char* Image::ExtractYUV(int s) {
        int yIndex = 0;
        int uvIndex = width * s * height * s;
        int R, G, B, Y, U, V;
        int index = 0;
        unsigned int xRGBIndex, yRGBIndex;
        unsigned char* output = new unsigned char[uvIndex * 2];
        for (int y = 0; y < height * s; y++) {
            xRGBIndex = 0;
            yRGBIndex = (y / s) * width * 3;
            for (unsigned int x = 0; x < width; x++) {
                B = data[yRGBIndex + xRGBIndex++];
                G = data[yRGBIndex + xRGBIndex++];
                R = data[yRGBIndex + xRGBIndex++];

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
        long unsigned int size = ftell(temp);
        rewind(temp);

        //read compressed data
        unsigned char* src = new unsigned char[size];
        fread(src, 1, size, temp);
        fclose(temp);

        //read header of compressed data
        int sub;
        tjhandle jpeg = tjInitDecompress();
        tjDecompressHeader2(jpeg, src, size, &width, &height, &sub);
        data = new unsigned char[width * height * 3];

        //decompress data
        tjDecompress2(jpeg, src, size, data, width, 0, height, TJPF_RGB, TJFLAG_FASTDCT);
        tjDestroy(jpeg);
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
        data = new unsigned char[3 * w * h];
        switch (color_type) {
            case PNG_COLOR_TYPE_RGBA:
                for (unsigned int i = 0; i < height; i++)
                    for (unsigned int j = 0; j < w; j++) {
                        data[3 * (i * w + j) + 0] = row_pointers[i][j * 4 + 0];
                        data[3 * (i * w + j) + 1] = row_pointers[i][j * 4 + 1];
                        data[3 * (i * w + j) + 2] = row_pointers[i][j * 4 + 2];
                    }
                break;
            case PNG_COLOR_TYPE_RGB:
                for (int i = 0; i < h; i++)
                    memcpy(data+(row_bytes * i), row_pointers[i], row_bytes);
                break;
            case PNG_COLOR_TYPE_PALETTE:
                int num_palette;
                png_colorp palette;
                png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
                for (unsigned int i = 0; i < h; i++)
                    for (unsigned int j = 0; j < row_bytes; j++) {
                        data[3 * (i * row_bytes + j) + 0] = palette[row_pointers[i][j]].red;
                        data[3 * (i * row_bytes + j) + 1] = palette[row_pointers[i][j]].green;
                        data[3 * (i * row_bytes + j) + 2] = palette[row_pointers[i][j]].blue;
                    }
            break;
            case PNG_COLOR_TYPE_GRAY:
                for (unsigned int i = 0; i < height; i++)
                    for (unsigned int j = 0; j < row_bytes; j++)
                        for (unsigned int k = 0; k < 3; k++)
                            data[3 * (i * row_bytes + j) + k] = row_pointers[i][j];
                break;
        }
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(temp);
    }

    void Image::WriteJPG(std::string filename) {
        //compress data
        long unsigned int size = 0;
        unsigned char* dst = NULL;
        tjhandle jpeg = tjInitCompress();
        tjCompress2(jpeg, data, width, 0, height, TJPF_RGB, &dst, &size, TJSAMP_444, JPEG_QUALITY, TJFLAG_FASTDCT);
        tjDestroy(jpeg);

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
                     8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(png_ptr, info_ptr);

        // write image data
        png_bytep row = (png_bytep) malloc(3 * width * sizeof(png_byte));
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                row[x * 3 + 0] = data[(y * width + x) * 3 + 0];
                row[x * 3 + 1] = data[(y * width + x) * 3 + 1];
                row[x * 3 + 2] = data[(y * width + x) * 3 + 2];
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
}
