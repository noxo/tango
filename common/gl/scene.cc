#include "gl/scene.h"

glm::vec3 baseId;

bool comparator(const id3d& a, const id3d& b)
{
    glm::vec3 va = glm::vec3(a.x, a.y, a.z);
    glm::vec3 vb = glm::vec3(b.x, b.y, b.z);
    return glm::length(baseId - va) < glm::length(baseId - vb);
}

//http://stackoverflow.com/questions/23880160/stdmap-key-no-match-for-operator
bool operator<(const id3d& lhs, const id3d& rhs)
{
    return lhs.x < rhs.x ||
           lhs.x == rhs.x && (lhs.y < rhs.y || lhs.y == rhs.y && lhs.z < rhs.z);
}

void oc::GLScene::Clear() {
    models.clear();
}

void oc::GLScene::Load(std::vector<oc::Mesh>& input) {
    id3d id;
    glm::vec3 v;
    int index = 0;
    for (Mesh& m : input) {
        std::string key = m.image->GetName();
        for (unsigned int i = 0; i < m.vertices.size(); i += 3) {
            //get position of triangle
            v = m.vertices[i + 0] + m.vertices[i + 1] + m.vertices[i + 2];
            v /= 3.0f;
            id.x = (int) (v.x / CULLING_DST);
            id.y = (int) (v.y / CULLING_DST);
            id.z = (int) (v.z / CULLING_DST);

            //create data structure
            if (models.find(id) == models.end())
                models[id] = std::map<std::string, Mesh>();
            if (models[id].find(key) == models[id].end())
                models[id][key] = Mesh();

            //fill structure with geometry
            models[id][key].image = m.image;
            models[id][key].imageOwner = false;
            models[id][key].image->AddInstance();
            models[id][key].vertices.push_back(m.vertices[i + 0]);
            models[id][key].vertices.push_back(m.vertices[i + 1]);
            models[id][key].vertices.push_back(m.vertices[i + 2]);
            models[id][key].uv.push_back(m.uv[i + 0]);
            models[id][key].uv.push_back(m.uv[i + 1]);
            models[id][key].uv.push_back(m.uv[i + 2]);
        }

        //release memory
        m.image->DelInstance();
        std::vector<glm::vec3>().swap(m.vertices);
        std::vector<glm::vec2>().swap(m.uv);
        LOGI("Composed %d/%d", ++index, input.size());
    }
    input.clear();
}

void oc::GLScene::Render(glm::vec4 camera, glm::vec4 dir, GLint position_param, GLint uv_param) {
    std::vector<id3d> visible = getVisibility(camera, glm::normalize(glm::vec3(dir.x, dir.y, dir.z)));
    for (id3d& id : visible) {
        if (models.find(id) != models.end()) {
            for (std::pair<const std::string, Mesh>& m : models[id]) {
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

std::vector<id3d> oc::GLScene::getVisibility(glm::vec4 camera, glm::vec3 dir) {
    std::vector<id3d> output;
    int steps = 3;
    int cx = (int) (camera.x / CULLING_DST);
    int cy = (int) (camera.y / CULLING_DST);
    int cz = (int) (camera.z / CULLING_DST);
    id3d id;
    id.x = cx;
    id.y = cy;
    id.z = cz;

    float m = 0.5f;
    for (int x = (dir.x < m ? -steps : -1); x <= (dir.x > -m ? steps : 1); x++)
        for (int y = (dir.y < m ? -steps : -1); y <= (dir.y > -m ? steps : 1); y++)
            for (int z = (dir.z < m ? -steps : -1); z <= (dir.z > -m ? steps : 1); z++)
            {
                id.x = cx + x;
                id.y = cy + y;
                id.z = cz + z;
                output.push_back(id);
            }

    baseId.x = cx;
    baseId.y = cy;
    baseId.z = cz;
    std::sort(output.begin(), output.end(), comparator);
    return output;
}
