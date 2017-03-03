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
        void ApplyInstance(SingleDynamicMesh* mesh);
        void MarkForUpdate(int index) { toUpdate[index] = true; }
        void RemoveInstance(SingleDynamicMesh* mesh);
        void Save(std::string modelPath);
        std::vector<unsigned int> TextureMap();
        bool UpdateGL();
        void UpdateTextures();

        RGBImage* GetLastImage() { return images[lastTextureIndex]; }

    private:
        glm::ivec4 FindAABB(int index);
        glm::ivec4 GetAABB(int index);
        void MaskUnused(int index);
        void Merge(int dst, int src);
        void Translate(int index, int mx, int my);

        std::map<int, glm::ivec4> boundaries;
        std::map<int, glm::ivec4> holes;
        std::vector<std::vector<SingleDynamicMesh*> > instances;
        std::vector<RGBImage*> images;
        std::map<int, bool> toLoad;
        std::map<int, bool> toUpdate;
        std::mutex mutex;
        std::vector<unsigned int> textureMap;
        int lastTextureIndex;
    };
} // namespace mesh_builder

#endif