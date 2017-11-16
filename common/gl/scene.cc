#include "gl/scene.h"

glm::vec3 baseId;

bool comparator(const std::pair<LOD, id3d>& a, const std::pair<LOD, id3d>& b)
{
    glm::vec3 va = glm::vec3(a.second.x, a.second.y, a.second.z);
    glm::vec3 vb = glm::vec3(b.second.x, b.second.y, b.second.z);
    return glm::length(baseId - va) < glm::length(baseId - vb);
}

//http://stackoverflow.com/questions/23880160/stdmap-key-no-match-for-operator
bool operator<(const id3d& lhs, const id3d& rhs)
{
    return lhs.x < rhs.x ||
           lhs.x == rhs.x && (lhs.y < rhs.y || lhs.y == rhs.y && lhs.z < rhs.z);
}

void oc::GLScene::Clear() {
    for (int i = 0; i < LOD_COUNT; i++) {
        for (std::pair<const id3d, std::map<std::string, Mesh>>& j : models[i])
            for (std::pair<const std::string, Mesh> & k : j.second)
                k.second.image->DelInstance();
        models[i].clear();
    }
}

void oc::GLScene::Load(std::vector<oc::Mesh>& input) {
    glm::vec3 a, b, c;
    int index = 0;
    for (Mesh& m : input) {
        std::string key = m.image->GetName();
        for (unsigned int i = 0; i < m.vertices.size(); i += 3) {

            //get place in grid where vertices belong
            id3d id[3];
            for (int k = 0; k < 3; k++) {
                id[k].x = (int) (m.vertices[i + k].x / CULLING_DST);
                id[k].y = (int) (m.vertices[i + k].y / CULLING_DST);
                id[k].z = (int) (m.vertices[i + k].z / CULLING_DST);
            }

            //try to create triangle from every vertex
            for (int k = 0; k < 3; k++) {

                //do not process same triangle more times if it is not necessary
                bool ok = true;
                for (int l = 0; l < k; l++)
                    if (id[k] == id[l])
                        ok = false;
                if (!ok)
                    break;

                //create data structure
                if (models[LOD_ORIG].find(id[k]) == models[LOD_ORIG].end())
                    for (int j = 0; j < LOD_COUNT; j++)
                        models[j][id[k]] = std::map<std::string, Mesh>();
                if (models[LOD_ORIG][id[k]].find(key) == models[LOD_ORIG][id[k]].end())
                    for (int j = 0; j < LOD_COUNT; j++)
                        models[j][id[k]][key] = Mesh();

                //fill structure with geometry
                for (int j = 0; j < LOD_COUNT; j++) {
                    models[j][id[k]][key].image = m.image;
                    models[j][id[k]][key].imageOwner = false;
                    models[j][id[k]][key].image->AddInstance();
                }
                models[LOD_ORIG][id[k]][key].vertices.push_back(m.vertices[i + 0]);
                models[LOD_ORIG][id[k]][key].vertices.push_back(m.vertices[i + 1]);
                models[LOD_ORIG][id[k]][key].vertices.push_back(m.vertices[i + 2]);
                models[LOD_ORIG][id[k]][key].uv.push_back(m.uv[i + 0]);
                models[LOD_ORIG][id[k]][key].uv.push_back(m.uv[i + 1]);
                models[LOD_ORIG][id[k]][key].uv.push_back(m.uv[i + 2]);

                //lower level of detail
                for (int d = 1; d < LOD_COUNT; d++) {
                    a = Dec(m.vertices[i + 0], (LOD &) d);
                    b = Dec(m.vertices[i + 1], (LOD &) d);
                    c = Dec(m.vertices[i + 2], (LOD &) d);
                    if (!Valid(a, b) || !Valid(b, c) || !Valid(c, a))
                        continue;
                    models[d][id[k]][key].vertices.push_back(a);
                    models[d][id[k]][key].vertices.push_back(b);
                    models[d][id[k]][key].vertices.push_back(c);
                    models[d][id[k]][key].uv.push_back(m.uv[i + 0]);
                    models[d][id[k]][key].uv.push_back(m.uv[i + 1]);
                    models[d][id[k]][key].uv.push_back(m.uv[i + 2]);
                }
            }
        }

        //release memory
        m.image->DelInstance();
        std::vector<glm::vec3>().swap(m.vertices);
        std::vector<glm::vec2>().swap(m.uv);
        LOGI("Composed %d/%d", ++index, (int)input.size());
    }
    input.clear();
}

void oc::GLScene::Render(glm::vec4 camera, glm::vec4 dir, GLint position_param, GLint uv_param) {
    std::vector<std::pair<LOD, id3d> > visible;
    visible = GetVisibility(camera, glm::normalize(glm::vec3(dir.x, dir.y, dir.z)));
    for (std::pair<LOD, id3d> & param : visible) {
        if (models[param.first].find(param.second) != models[param.first].end()) {
            for (std::pair<const std::string, Mesh>& m : models[param.first][param.second]) {
                if (m.second.vertices.empty())
                    continue;
                if (m.second.image && (m.second.image->GetTexture() == -1)) {
                    GLuint textureID;
                    glGenTextures(1, &textureID);
                    m.second.image->SetTexture(textureID);
                    glBindTexture(GL_TEXTURE_2D, textureID);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m.second.image->GetWidth(),
                                 m.second.image->GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                 m.second.image->GetData());
                }
                glBindTexture(GL_TEXTURE_2D, (unsigned int)m.second.image->GetTexture());
                glEnableVertexAttribArray((GLuint) position_param);
                glEnableVertexAttribArray((GLuint) uv_param);
                glVertexAttribPointer((GLuint) position_param, 3, GL_FLOAT, GL_FALSE, 0, m.second.vertices.data());
                glVertexAttribPointer((GLuint) uv_param, 2, GL_FLOAT, GL_FALSE, 0, m.second.uv.data());
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei) m.second.vertices.size());
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }
    }
    glDisableVertexAttribArray((GLuint) position_param);
    glDisableVertexAttribArray((GLuint) uv_param);
}

glm::vec3 oc::GLScene::Dec(glm::vec3 &v, LOD& level) {
    float a = 0, b = 0;
    switch(level) {
        case LOD_COUNT:
        case LOD_ORIG:
            return v;
        case LOD_HQ:
            a = 1.0f;
            b = 1.0f;
            break;
        case LOD_MQ:
            a = 0.25f;
            b = 4.0f;
            break;
        case LOD_LQ:
            a = 0.1f;
            b = 10.0f;
            break;
    }
    float x = ((int)(v.x * a)) * b;
    float y = ((int)(v.y * a)) * b;
    float z = ((int)(v.z * a)) * b;
    return glm::vec3(x, y, z);
}

std::vector<std::pair<LOD, id3d> > oc::GLScene::GetVisibility(glm::vec4 camera, glm::vec3 dir) {
    std::vector<std::pair<LOD, id3d> > output;
    std::pair<LOD, id3d> p;

    int steps = 5;
    int cx = (int) (camera.x / CULLING_DST);
    int cy = (int) (camera.y / CULLING_DST);
    int cz = (int) (camera.z / CULLING_DST);
    float m = 0.5f;
    for (int x = (dir.x < m ? -steps : -1); x <= (dir.x > -m ? steps : 1); x++)
        for (int y = (dir.y < m ? -steps : -1); y <= (dir.y > -m ? steps : 1); y++)
            for (int z = (dir.z < m ? -steps : -1); z <= (dir.z > -m ? steps : 1); z++) {
                p.first = LOD_ORIG;
                if ((abs(x) > 1) || (abs(y) > 1) || (abs(z) > 1))
                    p.first = LOD_HQ;
                if ((abs(x) > 2) || (abs(y) > 2) || (abs(z) > 2))
                    p.first = LOD_MQ;
                if ((abs(x) > 4) || (abs(y) > 4) || (abs(z) > 4))
                    p.first = LOD_LQ;
                p.second.x = cx + x;
                p.second.y = cy + y;
                p.second.z = cz + z;
                output.push_back(p);
            }

    baseId.x = cx;
    baseId.y = cy;
    baseId.z = cz;
    std::sort(output.begin(), output.end(), comparator);
    return output;
}

bool oc::GLScene::Valid(glm::vec3 a, glm::vec3 &b) {
    return (fabs(a.x - b.x) > 0.005) || (fabs(a.y - b.y) > 0.005) || (fabs(a.z - b.z) > 0.005);
}
