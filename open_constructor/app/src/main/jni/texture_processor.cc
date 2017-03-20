#include <cstdlib>
#include <tango_3d_reconstruction_api.h>
#include <string>
#include <sstream>
#include <tango-gl/util.h>
#include "math_utils.h"
#include "texture_processor.h"

namespace mesh_builder {
    TextureProcessor::TextureProcessor() {}

    TextureProcessor::~TextureProcessor() {
        for (RGBImage* t : images) {
            delete t;
        }
        for (unsigned int i : textureMap) {
            glDeleteTextures(1, &i);
        }
    }

    void TextureProcessor::Add(Tango3DR_ImageBuffer t3dr_image) {
        RGBImage* t = new RGBImage(t3dr_image, 4);
        mutex.lock();
        toLoad[images.size()] = true;
        images.push_back(t);
        mutex.unlock();
    }

    void TextureProcessor::Add(std::map<int, std::string> files) {
        std::vector<std::string> pngFiles;
        for (std::pair<const int, std::string> i : files) {
            while(i.first >= pngFiles.size()) {
                pngFiles.push_back("");
            }
            pngFiles[i.first] = i.second;
        }
        for (unsigned int i = 0; i < pngFiles.size(); i++) {
            RGBImage* t;
            if(pngFiles[i].empty()) {
                t = new RGBImage();
            } else
                t = new RGBImage(pngFiles[i]);
            mutex.lock();
            toLoad[images.size()] = true;
            images.push_back(t);
            mutex.unlock();
        }
    }

    void TextureProcessor::GenerateUV(glm::mat4 world2uv, Tango3DR_CameraCalibration calibration,
                                      SingleDynamicMesh* mesh) {
        mesh->mesh.uv.clear();
        for (unsigned int i = 0; i < mesh->mesh.vertices.size(); i++) {
            glm::vec4 v = glm::vec4(mesh->mesh.vertices[i], 1);
            Math::convert2uv(v, world2uv, calibration);
            mesh->mesh.uv.push_back(glm::vec2(v.x, v.y));
        }
    }

    std::vector<unsigned int> TextureProcessor::TextureMap() {
        mutex.lock();
        std::vector<unsigned int> output = textureMap;
        mutex.unlock();
        return output;
    }

    bool TextureProcessor::UpdateGL() {
        mutex.lock();
        bool updated = !toLoad.empty();
        for(std::pair<const int, bool> i : toLoad ) {
            while (i.first >= textureMap.size()) {
                GLuint textureID;
                glGenTextures(1, &textureID);
                textureMap.push_back(textureID);
            }
            RGBImage* t = images[i.first];
            glBindTexture(GL_TEXTURE_2D, textureMap[i.first]);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, t->GetWidth(), t->GetHeight(), 0, GL_RGB,
                         GL_UNSIGNED_BYTE, t->GetData());
        }
        toLoad.clear();
        mutex.unlock();
        return updated;
    }
}
