#include "editor/rasterizer.h"

namespace oc {

    void Rasterizer::AddUVS(std::vector<glm::vec2> uvs, std::vector<unsigned int> selected) {
        glm::vec3 a, b, c, center;
        for (unsigned long i = 0; i < uvs.size(); i += 3) {
            if (!selected.empty()) {
                if (selected[i + 0] != 0)
                    continue;
                if (selected[i + 1] != 0)
                    continue;
                if (selected[i + 2] != 0)
                    continue;
            }
            //get coordinate
            a = glm::vec3(uvs[i + 0], 0.0f);
            b = glm::vec3(uvs[i + 1], 0.0f);
            c = glm::vec3(uvs[i + 2], 0.0f);
            //mirror y axis
            a.y = 1.0f - a.y;
            b.y = 1.0f - b.y;
            c.y = 1.0f - c.y;
            //scale into raster dimensions
            a.x *= (float)(viewport_width - 1);
            a.y *= (float)(viewport_height - 1);
            b.x *= (float)(viewport_width - 1);
            b.y *= (float)(viewport_height - 1);
            c.x *= (float)(viewport_width - 1);
            c.y *= (float)(viewport_height - 1);
            //padding
            center = (a + b + c) / 3.0f;
            if (center.x > a.x) a.x -= 2; else a.x += 2;
            if (center.y > a.y) a.y -= 2; else a.y += 2;
            if (center.x > b.x) b.x -= 2; else b.x += 2;
            if (center.y > b.y) b.y -= 2; else b.y += 2;
            if (center.x > c.x) c.x -= 2; else c.x += 2;
            if (center.y > c.y) c.y -= 2; else c.y += 2;
            //process
            Triangle(i, a, b, c);
        }
    }

    void Rasterizer::AddVertices(std::vector<glm::vec3>& vertices, glm::mat4 world2screen) {
        glm::vec3 a, b, c;
        glm::vec4 wa, wb, wc;
        for (unsigned long i = 0; i < vertices.size(); i += 3) {
            //get coordinate
            wa = glm::vec4(vertices[i + 0], 1.0f);
            wb = glm::vec4(vertices[i + 1], 1.0f);
            wc = glm::vec4(vertices[i + 2], 1.0f);
            //transform to 2D
            wa = world2screen * wa;
            wb = world2screen * wb;
            wc = world2screen * wc;
            //perspective division
            wa /= glm::abs(wa.w);
            wb /= glm::abs(wb.w);
            wc /= glm::abs(wc.w);
            //convert
            a.x = wa.x;
            a.y = wa.y;
            a.z = wa.z;
            b.x = wb.x;
            b.y = wb.y;
            b.z = wb.z;
            c.x = wc.x;
            c.y = wc.y;
            c.z = wc.z;
            //convert it from -1,1 to 0,1
            a.x = (a.x + 1.0f) * 0.5f;
            a.y = (a.y + 1.0f) * 0.5f;
            b.x = (b.x + 1.0f) * 0.5f;
            b.y = (b.y + 1.0f) * 0.5f;
            c.x = (c.x + 1.0f) * 0.5f;
            c.y = (c.y + 1.0f) * 0.5f;
            //scale into raster dimensions
            a.x *= (float)(viewport_width - 1);
            a.y *= (float)(viewport_height - 1);
            b.x *= (float)(viewport_width - 1);
            b.y *= (float)(viewport_height - 1);
            c.x *= (float)(viewport_width - 1);
            c.y *= (float)(viewport_height - 1);
            //process
            Triangle(i, a, b, c);
        }
    }

    void Rasterizer::SetResolution(int w, int h) {
        viewport_width = w;
        viewport_height = h;

        if (fillCache1.size() != h + 1)
            fillCache1.resize((unsigned long) (h + 1));
        if (fillCache2.size() != h + 1)
            fillCache2.resize((unsigned long) (h + 1));
    }

    bool Rasterizer::Line(int x1, int y1, int x2, int y2, double z1, double z2,
                          std::pair<int, double>* fillCache) {

        //Liang & Barsky clipping (only top-bottom)
        int h = y2 - y1;
        double t1 = 0, t2 = 1;
        if (Test(-h, y1, t1, t2) && Test(h, viewport_height - 1 - y1, t1, t2) ) {
            double z;
            int c0, c1, xp0, xp1, yp0, yp1, y, p, w;
            bool wp, hp;

            //clip line
            if (t1 > 0) {
                w = x2 - x1;
                z = z2 - z1;
                x1 += t1 * w;
                y1 += t1 * h;
                z1 += t1 * z;
            } else
                t1 = 0;
            if (t2 < 1) {
                w = x2 - x1;
                z = z2 - z1;
                t2 -= t1;
                x2 = (int) (x1 + t2 * w);
                y2 = (int) (y1 + t2 * h);
                z2 = z1 + t2 * z;
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
            z = (z2 - z1) / (double)w;

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

    bool Rasterizer::Test(double p, double q, double &t1, double &t2) {
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

    void Rasterizer::Triangle(unsigned long& index, glm::vec3 &a, glm::vec3 &b, glm::vec3 &c) {

        //create markers for filling
        int min, max;
        int ab = (int) glm::abs(a.y - b.y);
        int ac = (int) glm::abs(a.y - c.y);
        int bc = (int) glm::abs(b.y - c.y);
        if ((ab >= ac) && (ab >= bc)) {
            Line((int) a.x, (int) a.y, (int) b.x, (int) b.y, a.z, b.z, &fillCache1[0]);
            Line((int) a.x, (int) a.y, (int) c.x, (int) c.y, a.z, c.z, &fillCache2[0]);
            Line((int) b.x, (int) b.y, (int) c.x, (int) c.y, b.z, c.z, &fillCache2[0]);
            min = glm::max(0, (int) glm::min(a.y, b.y));
            max = glm::min((int) glm::max(a.y, b.y), viewport_height - 1);
        } else if ((ac >= ab) && (ac >= bc)) {
            Line((int) a.x, (int) a.y, (int) c.x, (int) c.y, a.z, c.z, &fillCache1[0]);
            Line((int) a.x, (int) a.y, (int) b.x, (int) b.y, a.z, b.z, &fillCache2[0]);
            Line((int) b.x, (int) b.y, (int) c.x, (int) c.y, b.z, c.z, &fillCache2[0]);
            min = glm::max(0, (int) glm::min(a.y, c.y));
            max = glm::min((int) glm::max(a.y, c.y), viewport_height - 1);
        }else {
            Line((int) b.x, (int) b.y, (int) c.x, (int) c.y, b.z, c.z, &fillCache1[0]);
            Line((int) a.x, (int) a.y, (int) b.x, (int) b.y, a.z, b.z, &fillCache2[0]);
            Line((int) a.x, (int) a.y, (int) c.x, (int) c.y, a.z, c.z, &fillCache2[0]);
            min = glm::max(0, (int) glm::min(b.y, c.y));
            max = glm::min((int) glm::max(b.y, c.y), viewport_height - 1);
        }

        //fill triangle
        int memy = min * viewport_width;
        for (int y = min; y <= max; y++) {
            int x1 = fillCache1[y].first;
            int x2 = fillCache2[y].first;
            double z1 = fillCache1[y].second;
            double z2 = fillCache2[y].second;

            //Liang & Barsky clipping
            double t1 = 0;
            double t2 = 1;
            int x = x2 - x1;
            if (Test(-x, x1, t1, t2) && Test(x, viewport_width - 1 - x1, t1, t2)) {

                //callback for processing
                if (x2 > x1)
                    Process(index, x1, x2, y, z1, z2);
                else
                    Process(index, x2, x1, y, z2, z1);
            }
            memy += viewport_width;
        }
    }
}