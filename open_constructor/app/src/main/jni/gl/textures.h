#ifndef GL_TEXTURES_H
#define GL_TEXTURES_H

#include <vector>
#include <mutex>
#include <map>
#include "rgb_image.h"

namespace oc {
    class GLTextures {
    public:
        GLTextures();
        ~GLTextures();

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
}

#endif