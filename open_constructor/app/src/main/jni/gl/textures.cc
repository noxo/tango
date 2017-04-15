#include "gl/opengl.h"
#include "gl/textures.h"

namespace oc {
    GLTextures::GLTextures() {}

    GLTextures::~GLTextures() {
        for (RGBImage* t : images) {
            delete t;
        }
        for (unsigned int i : textureMap) {
            glDeleteTextures(1, &i);
        }
    }

    void GLTextures::Add(std::map<int, std::string> files) {
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

    RGBImage* GLTextures::GetTexture(unsigned int index) {
        mutex.lock();
        RGBImage* output = images[index];
        mutex.unlock();
        return output;
    }

    unsigned int GLTextures::TextureCount() {
        mutex.lock();
        unsigned int output = images.size();
        mutex.unlock();
        return output;
    }

    std::vector<unsigned int> GLTextures::TextureMap() {
        mutex.lock();
        std::vector<unsigned int> output = textureMap;
        mutex.unlock();
        return output;
    }

    bool GLTextures::UpdateGL() {
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

    void GLTextures::UpdateTexture(int index) {
        mutex.lock();
        toLoad[index] = true;
        mutex.unlock();
    }
}
