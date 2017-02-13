#ifndef OPEN_CONSTRUCTOR_TEXTURE_PROCESSOR_H
#define OPEN_CONSTRUCTOR_TEXTURE_PROCESSOR_H

#include <vector>
#include <mutex>
#include <map>

namespace mesh_builder {

    struct RGBImage {
        int width;
        int height;
        unsigned char* data;
    };

    class TextureProcessor {
    public:
        TextureProcessor();
        ~TextureProcessor();

        void Add(Tango3DR_ImageBuffer t3dr_image);
        void Add(std::vector<std::string> pngFiles);
        void Save(std::string modelPath);
        int TextureId() { return (int) (textureMap.size() - 1); }
        bool UpdateGL();
        std::vector<unsigned int> TextureMap() { return textureMap; };

    private:
        RGBImage ReadPNG(std::string file);
        void WritePNG(const char* filename, int width, int height, unsigned char *buffer);
        RGBImage YUV2RGB(Tango3DR_ImageBuffer t3dr_image, int scale);

        std::vector<RGBImage> images;
        std::map<int, bool> toLoad;
        std::mutex mutex;
        std::vector<unsigned int> textureMap;
    };
} // namespace mesh_builder

#endif