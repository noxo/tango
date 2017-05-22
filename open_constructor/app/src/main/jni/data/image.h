#ifndef DATA_IMAGE_H
#define DATA_IMAGE_H

#include <string>

namespace oc {

    class Image {
    public:
        Image();
        Image(unsigned char* src, int w, int h, int scale);
        Image(std::string filename);
        ~Image();
        unsigned char* ExtractYUV(unsigned int s);
        void UpdateYUV(unsigned char* src, int w, int h, int scale);
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
