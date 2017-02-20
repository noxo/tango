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
        void MarkForUpdate(int index) { toUpdate[index] = true; }
        void RemoveInstance(SingleDynamicMesh* mesh);
        void Save(std::string modelPath);
        std::vector<unsigned int> TextureMap();
        bool UpdateGL();
        void UpdateTextures();

    private:
        glm::ivec4 GetAABB(int index);
        RGBImage ReadPNG(std::string file);
        void MaskUnused(int index);
        void Translate(int index, int mx, int my);
        void WritePNG(const char* filename, int width, int height, unsigned char *buffer);
        RGBImage YUV2RGB(Tango3DR_ImageBuffer t3dr_image, int scale);

        std::vector<std::vector<SingleDynamicMesh*> > instances;
        std::vector<RGBImage> images;
        std::map<int, bool> toLoad;
        std::map<int, bool> toUpdate;
        std::mutex mutex;
        std::vector<unsigned int> textureMap;
        int lastTextureIndex;
    };
} // namespace mesh_builder

#endif