#ifndef DATA_IMAGE_H
#define DATA_IMAGE_H

#ifndef NOTANGO
#include <tango_3d_reconstruction_api.h>
#endif
#include <string>

namespace oc {

    class Image {
    public:
        Image();
#ifndef NOTANGO
        Image(Tango3DR_ImageBuffer t3dr_image, int scale);
#endif
        Image(std::string file);
        ~Image();
        unsigned char* ExtractYUV(int s);
        void Write(const char* filename);

        int GetWidth() { return width; }
        int GetHeight() { return height; }
        unsigned char* GetData() { return data; }
        std::string GetName() { return name; }

    private:
        int width;
        int height;
        unsigned char* data;
        std::string name;
    };
}

#endif
