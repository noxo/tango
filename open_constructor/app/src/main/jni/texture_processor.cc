#include <stdio.h>
#include <png.h>
#include <cstdlib>
#include <tango_3d_reconstruction_api.h>
#include <string>
#include <sstream>
#include <tango-gl/util.h>
#include "texture_processor.h"

FILE* temp;
void png_read_file(png_structp, png_bytep data, png_size_t length)
{
    fread(data, length, 1, temp);
}

namespace mesh_builder {
    TextureProcessor::TextureProcessor() {
    }

    TextureProcessor::~TextureProcessor() {

    }

    void TextureProcessor::Add(Tango3DR_ImageBuffer t3dr_image) {
        RGBImage t = YUV2RGB(t3dr_image, 4);
        mutex.lock();
        toLoad[images.size()] = true;
        images.push_back(t);
        mutex.unlock();
    }

    void TextureProcessor::Add(std::vector<std::string> pngFiles) {
        for (unsigned int i = 0; i < pngFiles.size(); i++) {
            toLoad[images.size()] = true;
            images.push_back(ReadPNG(pngFiles[i]));
        }
    }

    void TextureProcessor::Save(std::string modelPath) {
        std::string base = (modelPath.substr(0, modelPath.length() - 4));
        for (unsigned int i = 0; i < images.size(); i++) {
            std::ostringstream ostr;
            ostr << base.c_str();
            ostr << "_";
            ostr << i;
            ostr << ".png";
            LOGI("Saving %s", ostr.str().c_str());
            RGBImage t = images[i];
            WritePNG(ostr.str().c_str(), t.width, t.height, t.data);
        }
    }

    bool TextureProcessor::UpdateGL() {
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
        return updated;
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

    void TextureProcessor::WritePNG(const char* filename, int width, int height, unsigned char *buffer) {
        // Open file for writing (binary mode)
        FILE *fp = fopen(filename, "wb");

        // init PNG library
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info_ptr = png_create_info_struct(png_ptr);
        setjmp(png_jmpbuf(png_ptr));
        png_init_io(png_ptr, fp);
        png_set_IHDR(png_ptr, info_ptr, (png_uint_32) width, (png_uint_32) height,
                     8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(png_ptr, info_ptr);

        // write image data
        png_bytep row = (png_bytep) malloc(3 * width * sizeof(png_byte));
        for (int y = 0; y < height; y++) {
            for (int x=0; x < width; x++) {
                row[x * 3 + 0] = buffer[(y * width + x) * 3 + 0];
                row[x * 3 + 1] = buffer[(y * width + x) * 3 + 1];
                row[x * 3 + 2] = buffer[(y * width + x) * 3 + 2];
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
