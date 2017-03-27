#ifndef TEXTURE_PROCESSOR_H
#define TEXTURE_PROCESSOR_H

#include <vector>
#include <mutex>
#include <map>
#include "model_io.h"
#include "rgb_image.h"

namespace mesh_builder {
    class TextureProcessor {
    public:
        TextureProcessor();
        ~TextureProcessor();

        void Add(std::map<int, std::string> files);
        RGBImage* GetTexture(unsigned int index);
        unsigned int TextureCount();
        std::vector<unsigned int> TextureMap();
        bool UpdateGL();
        void UpdateTexture(int index);

    private:
        std::vector<RGBImage*> images;
        std::map<int, bool> toLoad;
        std::mutex mutex;
        std::vector<unsigned int> textureMap;
    };
} // namespace mesh_builder

#endif