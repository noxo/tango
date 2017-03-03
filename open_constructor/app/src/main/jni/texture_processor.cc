#include <stdio.h>
#include <cstdlib>
#include <tango_3d_reconstruction_api.h>
#include <string>
#include <sstream>
#include <tango-gl/util.h>
#include "texture_processor.h"
#include "mask_processor.h"

namespace mesh_builder {
    TextureProcessor::TextureProcessor() : lastTextureIndex(-1) {}

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
        int found = -1;
        for (unsigned int i = 0; i < instances.size(); i++)
            if (instances[i].empty())
                found = i;
        if (found >= 0) {
            delete images[found];
            images[found] = t;
            lastTextureIndex = found;
            toLoad[found] = true;
        } else {
            toLoad[images.size()] = true;
            lastTextureIndex = (int) images.size();
            images.push_back(t);
            instances.push_back(std::vector<SingleDynamicMesh*>());
        }
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
            instances.push_back(std::vector<SingleDynamicMesh*>());
            mutex.unlock();
        }
    }

    void TextureProcessor::ApplyInstance(SingleDynamicMesh* mesh) {
        mutex.lock();
        mesh->mesh.texture = lastTextureIndex;
        instances[lastTextureIndex].push_back(mesh);
        toUpdate[lastTextureIndex] = true;
        mutex.unlock();
    }

    void TextureProcessor::RemoveInstance(SingleDynamicMesh* mesh) {
        mutex.lock();
        for (int i = (int) (instances[mesh->mesh.texture].size() - 1); i >= 0; i--)
            if (instances[mesh->mesh.texture][i] == mesh)
                instances[mesh->mesh.texture].erase(instances[mesh->mesh.texture].begin() + i);
        mutex.unlock();
    }

    void TextureProcessor::Save(std::string modelPath) {
        mutex.lock();
        std::string base = (modelPath.substr(0, modelPath.length() - 4));
        for (unsigned int i = 0; i < images.size(); i++) {
            std::ostringstream ostr;
            ostr << base.c_str();
            ostr << "_";
            ostr << i;
            ostr << ".png";
            if (!instances.empty()) {
                LOGI("Saving %s", ostr.str().c_str());
                images[i]->Write(ostr.str().c_str());
            }
        }
        mutex.unlock();
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

    void TextureProcessor::UpdateTextures() {
        //Merging textures by bounding boxes
        for (std::pair<const int, bool> i : toUpdate) {
            MaskUnused(i.first);
            boundaries[i.first] = GetAABB(i.first);
            holes[i.first] = FindAABB(i.first);
        }
        for (std::pair<const int, bool> i : toUpdate) {
            for (std::pair<const int, glm::ivec4> j : boundaries) {
                if (i.first == j.first)
                    continue;
                if (images[i.first]->GetWidth() != images[j.first]->GetWidth())
                    continue;
                if (images[i.first]->GetHeight() != images[j.first]->GetHeight())
                    continue;
                glm::ivec4 bi = boundaries[i.first];
                glm::ivec4 bj = j.second;
                int h = images[i.first]->GetWidth() - (bi.z - bi.x + bj.z - bj.x);
                int v = images[i.first]->GetHeight() - (bi.w - bi.y + bj.w - bj.y);
                if ((h > 0) && (h > v)) {
                    Translate(i.first, -bi.x, -bi.y);
                    Translate(j.first, -bj.x + bi.z - bi.x, -bj.y);
                    Merge(i.first, j.first);
                    toUpdate.erase(j.first);
                    break;
                } else if ((v > 0) && (v > h)) {
                    Translate(i.first, -bi.x, -bi.y);
                    Translate(j.first, -bj.x, -bj.y + bi.w - bi.y);
                    Merge(i.first, j.first);
                    toUpdate.erase(j.first);
                    break;
                }
            }
        }
        //Merging textures by holes
        for (std::pair<const int, bool> i : toUpdate) {
            mutex.lock();
            bool empty = instances[i.first].empty();
            mutex.unlock();
            if (empty)
                continue;
            for (std::pair<const int, glm::ivec4> j : holes) {
                mutex.lock();
                empty = instances[j.first].empty();
                mutex.unlock();
                if (empty)
                    continue;
                if (i.first == j.first)
                    continue;
                if (images[i.first]->GetWidth() != images[j.first]->GetWidth())
                    continue;
                if (images[i.first]->GetHeight() != images[j.first]->GetHeight())
                    continue;

                glm::ivec4 bi = boundaries[i.first];
                glm::ivec4 bj = j.second;
                if (bi.z - bi.x <= bj.z - bj.x) {
                    if (bi.w - bi.y <= bj.w - bj.y) {
                        Translate(i.first, bj.x - bi.x, bj.y - bi.y);
                        Merge(j.first, i.first);
                    }
                }
            }
        }
        toUpdate.clear();
    }

    glm::ivec4 TextureProcessor::FindAABB(int index) {
        int w = images[index]->GetWidth();
        int h = images[index]->GetHeight() ;
        unsigned char* data = images[index]->GetData();
        int i = 0;
        int r, g, b;
        std::map<int, bool> limits;
        std::vector<glm::ivec2> spaces;
        for (int y = 0; y < h; y++) {
            int minx = INT_MIN;
            int maxx = INT_MIN;
            int tminx = INT_MIN;
            int tmaxx = INT_MIN;
            bool space = false;
            for (int x = 0; x < w; x++) {
                r = data[i++];
                g = data[i++];
                b = data[i++];
                if ((r == 255) && (g == 0) && (b == 255)) {
                    if (!space)
                        tminx = x;
                    tmaxx = x;
                    space = true;
                }
                else
                    space = false;
                if (maxx - minx < tmaxx - tminx) {
                    minx = tminx;
                    maxx = tmaxx;
                }
            }
            limits[minx] = true;
            limits[maxx] = true;
            spaces.push_back(glm::ivec2(minx, maxx));
        }
        long area;
        long maxArea = 0;
        int yp, ym;
        glm::ivec2 d, p;
        glm::ivec4 output;
        for (int y = 0; y < h; y++) {
            p = spaces[y];
            if (p.x == INT_MIN)
                continue;
            for (std::pair<const int, bool> & x : limits) {
                if ((x.first < p.x) || (x.first > p.y))
                    continue;
                for (ym = y; ym > 0;) {
                    d = spaces[ym];
                    if ((d.x == INT_MIN) || (d.x > p.x) || (d.y < x.first))
                        break;
                    else
                        ym--;
                }
                for (yp = y; yp < h - 1;) {
                    d = spaces[yp];
                    if ((d.x == INT_MIN) || (d.x > p.x) || (d.y < x.first))
                        break;
                    else
                        yp++;
                }
                area = (yp - ym) * (x.first - p.x);
                if (maxArea < area) {
                    maxArea = area;
                    output.x = p.x;
                    output.y = ym;
                    output.z = x.first;
                    output.w = yp;
                }
            }
        }
        return output;
    }

    glm::ivec4 TextureProcessor::GetAABB(int index) {
        int w = images[index]->GetWidth();
        int h = images[index]->GetHeight();
        unsigned char* data = images[index]->GetData();

        glm::ivec4 output;
        output.x = INT_MAX;
        output.y = INT_MAX;
        output.z = INT_MIN;
        output.w = INT_MIN;

        int i = 0;
        int r, g, b;
        bool empty = true;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                r = data[i++];
                g = data[i++];
                b = data[i++];
                if ((r != 255) && (g != 0) && (b != 255)) {
                    if (output.x > x)
                        output.x = x;
                    if (output.y > y)
                        output.y = y;
                    if (output.z < x)
                        output.z = x;
                    if (output.w < y)
                        output.w = y;
                    empty = false;
                }
            }
        }
        if (empty)
            return glm::ivec4(0, 0, 0, 0);
        else
            return output;
    }

    void TextureProcessor::MaskUnused(int index) {
        int w = images[index]->GetWidth();
        int h = images[index]->GetHeight();
        unsigned char* data = images[index]->GetData();
        MaskProcessor mp(instances[index], w, h);
        int i = 0;
        for (int y = h - 1; y >= 0; y--) {
            for (int x = 0; x < w; x++) {
                if(mp.GetMask(x, y) > 1000) {
                    data[i++] = 255;
                    data[i++] = 0;
                    data[i++] = 255;
                } else
                    i+=3;
            }
        }
        mutex.lock();
        toLoad[index] = true;
        mutex.unlock();
    }

    void TextureProcessor::Merge(int dst, int src) {
        //texture merging
        int r, g, b;
        for (int i = 0; i < images[dst]->GetWidth() * images[dst]->GetHeight() * 3; i+=3) {
            r = images[dst]->GetData()[i + 0];
            g = images[dst]->GetData()[i + 1];
            b = images[dst]->GetData()[i + 2];
            if ((r == 255) && (g == 0) && (b == 255)) {
                images[dst]->GetData()[i + 0] = images[src]->GetData()[i + 0];
                images[dst]->GetData()[i + 1] = images[src]->GetData()[i + 1];
                images[dst]->GetData()[i + 2] = images[src]->GetData()[i + 2];
            }
        }

        //update meshes
        for (SingleDynamicMesh* mesh : instances[src]) {
            mesh->mutex.lock();
            mesh->mesh.texture = dst;
            mesh->mutex.unlock();
            instances[dst].push_back(mesh);
        }
        instances[src].clear();

        //update texture variables
        MaskUnused(dst);
        boundaries[dst] = GetAABB(dst);
        holes[dst] = FindAABB(dst);
        mutex.lock();
        toLoad[dst] = true;
        mutex.unlock();
    }

    void TextureProcessor::Translate(int index, int mx, int my) {
        int w = images[index]->GetWidth();
        int h = images[index]->GetHeight();
        unsigned char* data = images[index]->GetData();
        //right and left shifting
        for (int y = 0; y < h; y++) {
            if (mx > 0) {
                for (int x = w - 1; x >= mx; x--) {
                    data[(y * w + x) * 3 + 0] = data[(y * w + x - mx) * 3 + 0];
                    data[(y * w + x) * 3 + 1] = data[(y * w + x - mx) * 3 + 1];
                    data[(y * w + x) * 3 + 2] = data[(y * w + x - mx) * 3 + 2];
                }
                for (int x = mx - 1; x >= 0; x--) {
                    data[(y * w + x) * 3 + 0] = 255;
                    data[(y * w + x) * 3 + 1] = 0;
                    data[(y * w + x) * 3 + 2] = 255;
                }
            } else if (mx < 0) {
                for (int x = 0; x < w + mx; x++) {
                    data[(y * w + x) * 3 + 0] = data[(y * w + x - mx) * 3 + 0];
                    data[(y * w + x) * 3 + 1] = data[(y * w + x - mx) * 3 + 1];
                    data[(y * w + x) * 3 + 2] = data[(y * w + x - mx) * 3 + 2];
                }
                for (int x = w + mx; x < w; x++) {
                    data[(y * w + x) * 3 + 0] = 255;
                    data[(y * w + x) * 3 + 1] = 0;
                    data[(y * w + x) * 3 + 2] = 255;
                }
            }
        }
        //up and down shifting
        for (int x = 0; x < w; x++) {
            if (my > 0) {
                for (int y = h - 1; y >= my; y--) {
                    data[(y * w + x) * 3 + 0] = data[((y - my) * w + x) * 3 + 0];
                    data[(y * w + x) * 3 + 1] = data[((y - my) * w + x) * 3 + 1];
                    data[(y * w + x) * 3 + 2] = data[((y - my) * w + x) * 3 + 2];
                }
                for (int y = my - 1; y >= 0; y--) {
                    data[(y * w + x) * 3 + 0] = 255;
                    data[(y * w + x) * 3 + 1] = 0;
                    data[(y * w + x) * 3 + 2] = 255;
                }
            } else if (my < 0) {
                for (int y = 0; y < h + my; y++) {
                    data[(y * w + x) * 3 + 0] = data[((y - my) * w + x) * 3 + 0];
                    data[(y * w + x) * 3 + 1] = data[((y - my) * w + x) * 3 + 1];
                    data[(y * w + x) * 3 + 2] = data[((y - my) * w + x) * 3 + 2];
                }
                for (int y = h + my; y < h; y++) {
                    data[(y * w + x) * 3 + 0] = 255;
                    data[(y * w + x) * 3 + 1] = 0;
                    data[(y * w + x) * 3 + 2] = 255;
                }
            }
        }
        //update texture coordinates
        mutex.lock();
        glm::vec2 offset = glm::vec2(mx / (float)(w - 1), -my / (float)(h - 1));
        for (SingleDynamicMesh* mesh : instances[index]) {
            mesh->mutex.lock();
            for (unsigned int i = 0; i < mesh->mesh.uv.size(); i++)
                mesh->mesh.uv[i] += offset;
            mesh->mutex.unlock();
        }
        toLoad[index] = true;
        mutex.unlock();
    }
}
