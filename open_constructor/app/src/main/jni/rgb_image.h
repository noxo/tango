#ifndef RGB_IMAGE_H
#define RGB_IMAGE_H

#include <tango_3d_reconstruction_api.h>
#include <string>

namespace mesh_builder {

    class RGBImage {
    public:
        RGBImage();
        RGBImage(Tango3DR_ImageBuffer t3dr_image, int scale);
        RGBImage(std::string file);
        ~RGBImage();
        void Write(const char* filename);

        int GetWidth() { return width; }
        int GetHeight() { return height; }
        unsigned char* GetData() { return data; }

    private:
        int width;
        int height;
        unsigned char* data;
    };
} // namespace mesh_builder

#endif