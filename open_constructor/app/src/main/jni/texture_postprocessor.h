#ifndef TEXTURE_POSTPROCESSOR_H
#define TEXTURE_POSTPROCESSOR_H

#include <map>
#include <tango_3d_reconstruction_api.h>
#include "math_utils.h"
#include "rgb_image.h"

namespace mesh_builder {

    class TexturePostProcessor {
    public:
        TexturePostProcessor(RGBImage* img);
        ~TexturePostProcessor();
        void ApplyTriangle(glm::vec3 &va, glm::vec3 &vb, glm::vec3 &vc,
                           glm::vec2 ta, glm::vec2 tb, glm::vec2 tc, RGBImage* texture,
                           glm::mat4 world2uv, Tango3DR_CameraCalibration calibration,
                           std::map<std::string, std::vector<glm::ivec3> >& vertices);
        void FixTriangle(glm::vec3 &va, glm::vec3 &vb, glm::vec3 &vc,
                         glm::vec2 ta, glm::vec2 tb, glm::vec2 tc,
                         std::map<std::string, std::vector<glm::ivec3> >& vertices);
        void Merge();
    private:
        glm::ivec3 GetPixel(int mem);
        bool Line(int x1, int y1, int x2, int y2, glm::vec3 z1, glm::vec3 z2,
                  std::pair<int, glm::vec3>* fillCache);
        bool Test(double p, double q, double &t1, double &t2);
        void Triangle(glm::vec2 &a, glm::vec2 &b, glm::vec2 &c,
                      glm::vec3 &ta, glm::vec3 &tb, glm::vec3 &tc, RGBImage* frame, bool renderMode);

        unsigned char* buffer;
        unsigned char* render;
        int viewport_width, viewport_height;
    };
} // namespace mesh_builder

#endif