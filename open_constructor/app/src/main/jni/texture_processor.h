#ifndef OPEN_CONSTRUCTOR_TEXTURE_PROCESSOR_H
#define OPEN_CONSTRUCTOR_TEXTURE_PROCESSOR_H

#include <vector>
#include <mutex>
#include <map>
#include "model_io.h"

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
        void ApplyInstance(SingleDynamicMesh* mesh);
        void RemoveInstance(SingleDynamicMesh* mesh);
        void Save(std::string modelPath);
        std::vector<unsigned int> TextureMap();
        bool UpdateGL();

    private:
        RGBImage ReadPNG(std::string file);
        void WritePNG(const char* filename, int width, int height, unsigned char *buffer);
        RGBImage YUV2RGB(Tango3DR_ImageBuffer t3dr_image, int scale);

        std::vector<std::vector<SingleDynamicMesh*> > instances;
        std::vector<RGBImage> images;
        std::map<int, bool> toLoad;
        std::mutex mutex;
        std::vector<unsigned int> textureMap;
        int lastTextureIndex;
    };
} // namespace mesh_builder

#endif