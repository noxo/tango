#ifndef RGB_IMAGE_H
#define RGB_IMAGE_H

#include <tango_3d_reconstruction_api.h>
#include <string>

namespace mesh_builder {

    class RGBImage {
    public:
        RGBImage();
        RGBImage(Tango3DR_ImageBuffer t3dr_image, int scale);
        RGBImage(std::string file);
        RGBImage(int w, int h, double* buffer);
        ~RGBImage();
        glm::vec3 GetValue(int x, int y);
        glm::vec3 GetValue(glm::vec2 coord);
        void Merge(unsigned char* values);
        void Write(const char* filename);

        int GetWidth() { return width; }
        int GetHeight() { return height; }
        unsigned char* GetData() { return data; }
        std::string GetName() { return name; }

    private:
        int width;
        int height;
        unsigned char* data;
        std::string name;
    };
} // namespace mesh_builder

#endif