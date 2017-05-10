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
        Image(std::string filename);
        ~Image();
        unsigned char* ExtractYUV(int s);
        void Write(std::string filename);

        int GetWidth() { return width; }
        int GetHeight() { return height; }
        unsigned char* GetData() { return data; }
        std::string GetName() { return name; }

    private:
        void ReadJPG(std::string filename);
        void ReadPNG(std::string filename);
        void WriteJPG(std::string filename);
        void WritePNG(std::string filename);

        int width;
        int height;
        unsigned char* data;
        std::string name;
    };
}

#endif
