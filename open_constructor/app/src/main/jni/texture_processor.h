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

        void Add(Tango3DR_ImageBuffer t3dr_image);
        void Add(std::map<int, std::string> files);
        void GenerateUV(glm::mat4 world2uv, Tango3DR_CameraCalibration c, SingleDynamicMesh* mesh);
        RGBImage* GetTexture(int index) { return images[index]; }
        unsigned int TextureCount() { images.size(); }
        std::vector<unsigned int> TextureMap();
        bool UpdateGL();

    private:
        std::vector<RGBImage*> images;
        std::map<int, bool> toLoad;
        std::mutex mutex;
        std::vector<unsigned int> textureMap;
    };
} // namespace mesh_builder

#endif