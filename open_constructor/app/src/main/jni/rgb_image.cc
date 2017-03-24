#include <png.h>
#include <tango-gl/util.h>
#include "rgb_image.h"

FILE* temp;
void png_read_file(png_structp, png_bytep data, png_size_t length)
{
    fread(data, length, 1, temp);
}

namespace mesh_builder {

    RGBImage::RGBImage() {
        width = 1;
        height = 1;
        data = new unsigned char[3];
        name = "";
    }

    RGBImage::RGBImage(Tango3DR_ImageBuffer t3dr_image, int scale) {
        data = new unsigned char[(t3dr_image.width / scale) * (t3dr_image.height / scale) * 3];
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
                data[index++] = (unsigned char) B;
                data[index++] = (unsigned char) G;
                data[index++] = (unsigned char) R;
            }
        }
        width = t3dr_image.width / scale;
        height = t3dr_image.height / scale;
        name = "photo";
    }

    RGBImage::RGBImage(std::string file) {
        LOGI("Reading %s", file.c_str());
        temp = fopen(file.c_str(), "r");
        unsigned int sig_read = 0;

        /// init PNG library
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
        data = new unsigned char[row_bytes * height];
        png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
        for (unsigned int i = 0; i < height; i++)
            memcpy(data+(row_bytes * i), row_pointers[i], row_bytes);

        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(temp);
        name = file;
    }

    RGBImage::RGBImage(int w, int h, double *buffer) {
        width = w;
        height = h;
        data = new unsigned char[w * h * 3];
        name = "buffer";
        int index = 0;
        for(int y = h - 1; y >= 0; y--) {
            for(int x = 0; x < w; x++) {
                data[index++] = (unsigned char) glm::clamp(buffer[y * w + x] * 128.0, 0.0, 255.0);
                data[index++] = (unsigned char) glm::clamp(buffer[y * w + x] * 128.0, 0.0, 255.0);
                data[index++] = (unsigned char) glm::clamp(buffer[y * w + x] * 512.0, 0.0, 255.0);
            }
        }
    }

    RGBImage::~RGBImage() {
        delete[] data;
    }

    glm::vec3 RGBImage::GetValue(int x, int y) {
      if (x < 0)
        x = 0;
      if (y < 0)
        y = 0;
      if (x >= width)
        x = width - 1;
      if (y >= height)
        y = height - 1;
      int index = (y * width + x) * 3;
      glm::vec3 output;
      output.r = data[index++];
      output.g = data[index++];
      output.b = data[index++];
      return output;
    }

    glm::vec3 RGBImage::GetValue(glm::vec2 coord) {
      float x = coord.x;
      float y = coord.y;
      int ix = (int) x;
      int iy = (int) y;
      float dx = x - ix;
      float dy = y - iy;
      glm::vec3 v = glm::vec3(0, 0, 0);
      v += GetValue(ix, iy) * (1 - dx) * (1 - dy);
      v += GetValue(ix, iy + 1) * (1 - dx) * dy;
      v += GetValue(ix + 1, iy) * dx * (1 - dy);
      v += GetValue(ix + 1, iy + 1) * dx * dy;
      return v;
    }

    void RGBImage::Write(const char *filename) {
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
                row[x * 3 + 0] = data[(y * width + x) * 3 + 0];
                row[x * 3 + 1] = data[(y * width + x) * 3 + 1];
                row[x * 3 + 2] = data[(y * width + x) * 3 + 2];
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
}