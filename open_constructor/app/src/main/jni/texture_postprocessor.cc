#include "texture_postprocessor.h"
#include <queue>

std::vector<std::pair<int, glm::vec2> > fillCache1;
std::vector<std::pair<int, glm::vec2> > fillCache2;


bool comparator(const glm::ivec3& a, const glm::ivec3& b)
{
    return 4 * (a.x - b.x) + 2 * (a.y - b.y) + 2 * (a.z - b.z);
}

namespace mesh_builder {

    TexturePostProcessor::TexturePostProcessor(RGBImage* img) {
        analyze = false;
        analyzePose = -1;
        buffer = img->GetData();
        viewport_width = img->GetWidth();
        viewport_height = img->GetHeight();

        if (fillCache1.empty())
            fillCache1.resize((unsigned long) (viewport_height + 1));
        if (fillCache2.empty())
            fillCache2.resize((unsigned long) (viewport_height + 1));
    }

    void TexturePostProcessor::ApplyTriangle(glm::vec3 &va, glm::vec3 &vb, glm::vec3 &vc,
                                             glm::vec2 ta, glm::vec2 tb, glm::vec2 tc,
                                             RGBImage* texture, glm::mat4 world2uv,
                                             Tango3DR_CameraCalibration calibration) {
        //unwrap coordinates
        glm::vec2 a, b, c;
        glm::vec4 vertex;
        vertex = glm::vec4(va, 1.0f);
        Math::convert2uv(vertex, world2uv, calibration);
        a = glm::vec2(vertex.x, vertex.y);
        a.y = 1.0f - a.y;
        a.x *= texture->GetWidth()  - 1;
        a.y *= texture->GetHeight() - 1;
        vertex = glm::vec4(vb, 1.0f);
        Math::convert2uv(vertex, world2uv, calibration);
        b = glm::vec2(vertex.x, vertex.y);
        b.y = 1.0f - b.y;
        b.x *= texture->GetWidth()  - 1;
        b.y *= texture->GetHeight() - 1;
        vertex = glm::vec4(vc, 1.0f);
        Math::convert2uv(vertex, world2uv, calibration);
        c = glm::vec2(vertex.x, vertex.y);
        c.y = 1.0f - c.y;
        c.x *= texture->GetWidth()  - 1;
        c.y *= texture->GetHeight() - 1;
        //frame coordinate
        ta.y = 1.0f - ta.y;
        tb.y = 1.0f - tb.y;
        tc.y = 1.0f - tc.y;
        ta.x *= viewport_width - 1;
        ta.y *= viewport_height - 1;
        tb.x *= viewport_width - 1;
        tb.y *= viewport_height - 1;
        tc.x *= viewport_width - 1;
        tc.y *= viewport_height - 1;
        //render
        Triangle(ta, tb, tc, a, b, c, texture);
    }

    bool TexturePostProcessor::Line(int x1, int y1, int x2, int y2, glm::vec2 z1, glm::vec2 z2,
                             std::pair<int, glm::vec2>* fillCache) {

        //Liang & Barsky clipping (only top-bottom)
        int h = y2 - y1;
        double t1 = 0, t2 = 1;
        if (Test(-h, y1, t1, t2) && Test(h, viewport_height - 1 - y1, t1, t2) ) {
            glm::vec2 z;
            int c0, c1, xp0, xp1, yp0, yp1, y, p, w;
            bool wp, hp;

            //clip line
            if (t1 > 0) {
                w = x2 - x1;
                z = z2 - z1;
                x1 += t1 * w;
                y1 += t1 * h;
                z1 += (float)t1 * z;
            } else
                t1 = 0;
            if (t2 < 1) {
                w = x2 - x1;
                z = z2 - z1;
                t2 -= t1;
                x2 = (int) (x1 + t2 * w);
                y2 = (int) (y1 + t2 * h);
                z2 = (float)t2 * z + z1;
            }

            //count new line dimensions
            wp = x2 >= x1;
            w = wp ? x2 - x1 : x1 - x2;
            hp = y2 >= y1;
            h = hp ? y2 - y1 : y1 - y2;

            //line in x axis nearby
            if (w > h) {
                //direction from left to right
                xp0 = wp ? 1 : -1;
                yp0 = 0;

                //direction from top to bottom
                xp1 = wp ? 1 : -1;
                yp1 = hp ? 1 : -1;

                //line in y axis nearby
            } else {
                //direction from top to bottom
                xp0 = 0;
                yp0 = hp ? 1 : -1;

                //direction from left to right
                xp1 = wp ? 1 : -1;
                yp1 = hp ? 1 : -1;

                //apply line length
                y = w;
                w = h;
                h = y;
            }

            //count z coordinate step
            z = (z2 - z1) / (float)w;

            //Bresenham's algorithm
            c0 = h + h;
            p = c0 - w;
            c1 = p - w;
            y = y1;
            fillCache[y].first = x1;
            fillCache[y].second = z1;
            for (w--; w >= 0; w--) {

                //interpolate
                if (p < 0) {
                    p += c0;
                    x1 += xp0;
                    y1 += yp0;
                } else {
                    p += c1;
                    x1 += xp1;
                    y1 += yp1;
                }
                z1 += z;

                //write cache info
                if (wp || (y != y1)) {
                    y = y1;
                    fillCache[y].first = x1;
                    fillCache[y].second = z1;
                }
            }
            return true;
        }
        return false;
    }

    bool TexturePostProcessor::Test(double p, double q, double &t1, double &t2) {
        //negative cutting
        if (p < 0) {
            double t = q/p;

            //cut nothing
            if (t > t2)
                return false;
                //cut the first coordinate
            else if (t > t1)
                t1 = t;

            //positive cutting
        } else if (p > 0) {
            double t = q/p;

            //cut nothing
            if (t < t1)
                return false;
                //cut the second coordinate
            else if (t < t2)
                t2 = t;

            //line is right to left(or bottom to top)
        } else if (q < 0)
            return false;
        return true;
    }


    void TexturePostProcessor::Triangle(glm::vec2 &a, glm::vec2 &b, glm::vec2 &c,
                                        glm::vec2 &ta, glm::vec2 &tb, glm::vec2 &tc, RGBImage* frame) {

        //create markers for filling
        int min, max;
        int ab = (int) glm::abs(a.y - b.y);
        int ac = (int) glm::abs(a.y - c.y);
        int bc = (int) glm::abs(b.y - c.y);
        if ((ab >= ac) && (ab >= bc)) {
            Line((int) a.x, (int) a.y, (int) b.x, (int) b.y, ta, tb, &fillCache1[0]);
            Line((int) a.x, (int) a.y, (int) c.x, (int) c.y, ta, tc, &fillCache2[0]);
            Line((int) b.x, (int) b.y, (int) c.x, (int) c.y, tb, tc, &fillCache2[0]);
            min = glm::max(0, (int) glm::min(a.y, b.y));
            max = glm::min((int) glm::max(a.y, b.y), viewport_height - 1);
        } else if ((ac >= ab) && (ac >= bc)) {
            Line((int) a.x, (int) a.y, (int) c.x, (int) c.y, ta, tc, &fillCache1[0]);
            Line((int) a.x, (int) a.y, (int) b.x, (int) b.y, ta, tb, &fillCache2[0]);
            Line((int) b.x, (int) b.y, (int) c.x, (int) c.y, tb, tc, &fillCache2[0]);
            min = glm::max(0, (int) glm::min(a.y, c.y));
            max = glm::min((int) glm::max(a.y, c.y), viewport_height - 1);
        }else {
            Line((int) b.x, (int) b.y, (int) c.x, (int) c.y, tb, tc, &fillCache1[0]);
            Line((int) a.x, (int) a.y, (int) b.x, (int) b.y, ta, tb, &fillCache2[0]);
            Line((int) a.x, (int) a.y, (int) c.x, (int) c.y, ta, tc, &fillCache2[0]);
            min = glm::max(0, (int) glm::min(b.y, c.y));
            max = glm::min((int) glm::max(b.y, c.y), viewport_height - 1);
        }

        //fill triangle
        int memy = min * viewport_width;
        for (int y = min; y <= max; y++) {
            int x1 = fillCache1[y].first;
            int x2 = fillCache2[y].first;
            glm::vec2 z1 = fillCache1[y].second;
            glm::vec2 z2 = fillCache2[y].second;

            //Liang & Barsky clipping
            double t1 = 0;
            double t2 = 1;
            int x = x2 - x1;
            if (Test(-x, x1, t1, t2) && Test(x, viewport_width - 1 - x1, t1, t2)) {

                //clip line
                glm::vec2 z = z2 - z1;
                if (t1 > 0) {
                    x1 += t1 * x;
                    z1 += (float)t1 * z;
                } else
                    t1 = 0;
                if (t2 < 1) {
                    t2 -= t1;
                    x2 = (int) (x1 + t2 * x);
                    z2 = (float)t2 * z + z1;
                }

                //filling algorithm
                x = glm::abs(x2 - x1);
                int step = (x2 >= x1) ? 3 : -3;
                z = (z2 - z1) / (float)x;
                int mem = (x1 + memy) * 3;
                int itx, ity;
                glm::ivec3 color;
                for (; x >= 0; x--) {
                    if ((z1.x >= 0) && (z1.x < frame->GetWidth()))
                        if ((z1.y >= 0) && (z1.y < frame->GetHeight())) {
                            if (analyze) {
                                color = glm::ivec3(buffer[mem + 0], buffer[mem + 1], buffer[mem + 2]);
                                color = RGB2HSV(color);
                                color -= frame->GetValue(z1);
                                while (color.x < -180)
                                    color.x += 360;
                                while (color.x > 180)
                                    color.x -= 360;
                                diff[analyzePose].push_back(color);
                            } else {
                                color = frame->GetValue(z1);
                                color = RGB2HSV(color);
                                //TODO:color correction
                                color = HSV2RGB(color);
                                buffer[mem + 0] = color.r;
                                buffer[mem + 1] = color.g;
                                buffer[mem + 2] = color.b;
                            }
                        }
                    mem += step;
                    z1 += z;
                }
            }
            memy += viewport_width;
        }
    }

    glm::ivec3 TexturePostProcessor::HSV2RGB(glm::ivec3 hsv)
    {
        //gray-scale
        glm::ivec3 rgb;
        if (hsv.y == 0) {
            rgb.r = hsv.z;
            rgb.g = hsv.z;
            rgb.b = hsv.z;
            return rgb;
        }

        //get parameters
        unsigned char region, remainder, p, q, t;
        region = hsv.x / 43;
        remainder = (hsv.x - (region * 43)) * 6;
        p = (hsv.z * (255 - hsv.y)) >> 8;
        q = (hsv.z * (255 - ((hsv.y * remainder) >> 8))) >> 8;
        t = (hsv.z * (255 - ((hsv.y * (255 - remainder)) >> 8))) >> 8;

        //all cases
        switch (region) {
        case 0:
            rgb.r = hsv.z; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = hsv.z; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = hsv.z; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = hsv.z;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = hsv.z;
            break;
        default:
            rgb.r = hsv.z; rgb.g = p; rgb.b = q;
            break;
        }
        return rgb;
    }

    glm::ivec3 TexturePostProcessor::RGB2HSV(glm::ivec3 rgb)
    {
        //find extremes
        unsigned char rgbMin, rgbMax;
        rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
        rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

        //get value
        glm::ivec3 hsv;
        hsv.z = rgbMax;
        if (hsv.z == 0) {
            hsv.x = 0;
            hsv.y = 0;
            return hsv;
        }

        //get saturation
        hsv.y = 255 * long(rgbMax - rgbMin) / hsv.z;
        if (hsv.y == 0) {
            hsv.x = 0;
            return hsv;
        }

        //get hue
        if (rgbMax == rgb.r)
            hsv.x = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
        else if (rgbMax == rgb.g)
            hsv.x = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
        else
            hsv.x = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);
        return hsv;
    }

    void TexturePostProcessor::SetAnalyze(bool on, int pose) {
        if (on) {
            analyze = true;
            analyzePose = pose;
            diff[pose] = std::vector<glm::ivec3>();
        } else {
            analyze = false;
            analyzePose = diff[pose].size() / 2;
            std::sort(diff[pose].begin(), diff[pose].end(), comparator);
        }
    }
}
