#ifndef DATA_IMAGE_H
#define DATA_IMAGE_H

#include <string>
#include <vector>

namespace oc {

    class Image {
    public:
        Image(int w, int h);
        Image(unsigned char* src, int w, int h, int scale);
        Image(std::string filename);
        ~Image();
        unsigned char* ExtractYUV(unsigned int s);
        void SetTexture(long value) { texture = value; }
        void UpdateTexture();
        void UpdateYUV(unsigned char* src, int w, int h, int scale);
        void Write(std::string filename);

        int GetWidth() { return width; }
        int GetHeight() { return height; }
        unsigned char* GetData() { return data; }
        std::string GetName() { return name; }
        long GetTexture() { return texture; }

        static void JPG2YUV(std::string filename, unsigned char* data, int width, int height);
        static void YUV2JPG(unsigned char* data, int width, int height, std::string filename);
        static std::vector<unsigned int> TexturesToDelete();

    private:
        void ReadJPG(std::string filename);
        void ReadPNG(std::string filename);
        void WriteJPG(std::string filename);
        void WritePNG(std::string filename);

        int width;
        int height;
        unsigned char* data;
        std::string name;
        long texture;
    };
}

#endif
