///----------------------------------------------------------------------------------------
/**
 * \file       app.cpp
 * \author     Vonasek Lubos
 * \date       2018/06/01
 * \brief      Runable code of project.
 *
 * Dataset for testing can be downloaded from here:
 * https://drive.google.com/open?id=1DAZdt9Jbm2_TK4_priT1z4pmLAclxVaf
**/
///----------------------------------------------------------------------------------------

#include <data/dataset.h>
#include <data/file3d.h>
#include <GL/freeglut.h>

#define MAKE_SURFACE

//shader
const char* kTextureShader[] = {R"glsl(
    #version 100
    precision highp float;
    uniform mat4 u_MVP;
    uniform float u_X;
    uniform float u_Y;
    uniform float u_Z;
    attribute vec4 a_Position;
    attribute vec4 a_Color;
    varying vec4 v_Color;

    void main() {
      v_Color = a_Color;
      vec4 pos = a_Position;
      pos.x += u_X;
      pos.y += u_Y;
      pos.z += u_Z;
      gl_Position = u_MVP * pos;
    })glsl",

    R"glsl(
    #version 100
    precision highp float;
    varying vec4 v_Color;

    void main() {
      gl_FragColor = v_Color;
    })glsl"
};

//shader stuff
GLuint model_program_;
GLint model_position_param_;
GLint model_colors_param_;
GLint model_translatex_param_;
GLint model_translatey_param_;
GLint model_translatez_param_;
GLint model_modelview_projection_param_;

bool mouseActive = true;
float mouseX;          ///< Last mouse X
float mouseY;          ///< Last mouse X
float pitch;           ///< Camera pitch
float yaw;             ///< Camera yaw
glm::vec4 camera;      ///< Camera position
glm::mat4 proj;        ///< Camera projection
glm::mat4 view;        ///< Camera view
bool keys[5];          ///< State of keyboard
glm::ivec2 resolution; ///< Screen resolution
oc::Dataset* dataset;  ///< Path to dataset
float cx, cy, fx, fy;  ///< Calibration
int poseCount;         ///< Pose count
int poseIndex;         ///< Pose index

//render data
std::vector<oc::Mesh> model;
std::vector<glm::vec3> points;
std::vector<unsigned int> colors;
std::vector<unsigned int> indices;

#define KEY_UP 'w'
#define KEY_DOWN 's'
#define KEY_LEFT 'a'
#define KEY_RIGHT 'd'
#define KEY_NITRO ' '

/**
 * @brief display updates display
 */
void display(void)
{
    /// set buffers
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, resolution.x, resolution.y);

    /// set view
    float aspect = resolution.x / (float)resolution.y;
    glm::vec3 eye = glm::vec3(0, 0, 0);
    glm::vec3 center = glm::vec3(sin(yaw), -sin(pitch), -cos(yaw));
    glm::vec3 up = glm::vec3(0, 1, 0);
    proj = glm::perspective(glm::radians(60.0f), aspect, 0.01f, 500.0f);
    view = glm::lookAt(eye, center, up);

    /// bind shader
    glUseProgram(model_program_);
    glUniform1f(model_translatex_param_, camera.x);
    glUniform1f(model_translatey_param_, camera.y);
    glUniform1f(model_translatez_param_, camera.z);
    glUniformMatrix4fv(model_modelview_projection_param_, 1, GL_FALSE, glm::value_ptr(proj * view));

    /// render point cloud
    glPointSize(3);
    glEnableVertexAttribArray((GLuint) model_position_param_);
    glEnableVertexAttribArray((GLuint) model_colors_param_);
    glVertexAttribPointer((GLuint) model_position_param_, 3, GL_FLOAT, GL_FALSE, 0, points.data());;
    glVertexAttribPointer((GLuint) model_colors_param_, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, colors.data());
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#ifdef MAKE_SURFACE
    glDrawElements(GL_TRIANGLES, (GLsizei) indices.size(), GL_UNSIGNED_INT, indices.data());
#else
    glDrawArrays(GL_POINTS, 0, (GLsizei) points.size());
#endif

    /// render model
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (oc::Mesh& m : model) {
        if (!m.vertices.empty()) {
            glVertexAttribPointer((GLuint) model_position_param_, 3, GL_FLOAT, GL_FALSE, 0, m.vertices.data());;
            glVertexAttribPointer((GLuint) model_colors_param_, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, m.colors.data());
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei) m.vertices.size());
        }
    }
    glUseProgram(0);

    /// check if there is an error
    int i = glGetError();
    if (i != 0)
        printf("GL_ERROR %d\n", i);

    /// finish rendering
    glutSwapBuffers();
}

/**
 * @brief idle is non-graphical thread and it is called automatically by GLUT
 * @param v is time information
 */
void idle(int v)
{
    glm::mat4 direction = view;
    direction[3][0] = 0;
    direction[3][1] = 0;
    direction[3][2] = 0;
    glm::vec4 side = glm::vec4(0, 0, 0, 1);
    if (keys[0] && keys[1])
      side = glm::vec4(0, 0, 0, 1);
    else if (keys[0])
      side = glm::vec4(1, 0, 0, 1) * direction;
    else if (keys[1])
      side = glm::vec4(-1, 0, 0, 1) * direction;
    side /= glm::abs(side.w);
    if (keys[4])
      side *= 10.0f;
    else
      side *= 0.1f;
    camera.x += side.x;
    camera.y += side.y;
    camera.z += side.z;
    glm::vec4 forward = glm::vec4(0, 0, 0, 1);
    if (keys[2] && keys[3])
      forward = glm::vec4(0, 0, 0, 1);
    else if (keys[2])
      forward = glm::vec4(0, 0, 1, 1) * direction;
    else if (keys[3])
      forward = glm::vec4(0, 0, -1, 1) * direction;
    else if (keys[4] && !keys[0] && !keys[1])
      forward = glm::vec4(0, 0, 1, 1) * direction;
    forward /= glm::abs(forward.w);
    if (keys[4])
      forward *= 10.0f;
    else
      forward *= 0.1f;
    camera.x += forward.x;
    camera.y += forward.y;
    camera.z += forward.z;

    if (mouseActive) {
        pitch += (mouseY - resolution.y / 2) / (float)resolution.y;
        yaw += (mouseX - resolution.x / 2) / (float)resolution.x;
        glutWarpPointer(resolution.x / 2, resolution.y / 2);
    }
    glutSetCursor(mouseActive ? GLUT_CURSOR_NONE : GLUT_CURSOR_LEFT_ARROW);

    if (pitch < -1.57)
        pitch = -1.57;
    if (pitch > 1.57)
        pitch = 1.57;

    /// call update
    glutPostRedisplay();
    glutTimerFunc(50,idle,0);
}


void initializeGl()
{
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &kTextureShader[0], nullptr);
  glCompileShader(vertex_shader);

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &kTextureShader[1], nullptr);
  glCompileShader(fragment_shader);

  model_program_ = glCreateProgram();
  glAttachShader(model_program_, vertex_shader);
  glAttachShader(model_program_, fragment_shader);
  glLinkProgram(model_program_);
  glUseProgram(model_program_);

  model_position_param_ = glGetAttribLocation(model_program_, "a_Position");
  model_colors_param_ = glGetAttribLocation(model_program_, "a_Color");
  model_modelview_projection_param_ = glGetUniformLocation(model_program_, "u_MVP");
  model_translatex_param_ = glGetUniformLocation(model_program_, "u_X");
  model_translatey_param_ = glGetUniformLocation(model_program_, "u_Y");
  model_translatez_param_ = glGetUniformLocation(model_program_, "u_Z");
}

void loadPointCloud() {
    std::vector<oc::Mesh> mesh;
    glm::mat4 sensor2world = dataset->GetPose(poseIndex)[1];
    glm::mat4 world2uv = glm::inverse(dataset->GetPose(poseIndex)[0]);
    oc::Image jpg(dataset->GetFileName(poseIndex, ".jpg"));
    oc::File3d pcl(dataset->GetFileName(poseIndex, ".pcl"), false);
    pcl.ReadModel(-1, mesh);

    int mapScale = 10;
    int stride = jpg.GetWidth() / mapScale;
    int height = jpg.GetHeight() / mapScale;
    int map[stride * height];
    for (int i = 0; i < stride * height; i++)
        map[i] = -1;
    for (glm::vec3& v : mesh[0].vertices) {
        glm::vec4 w = sensor2world * glm::vec4(v, 1.0f);
        w /= glm::abs(w.w);
        glm::vec4 t = world2uv * glm::vec4(w.x, w.y, w.z, 1.0f);
        t.x /= glm::abs(t.z * t.w);
        t.y /= glm::abs(t.z * t.w);
        t.x *= fx / (float)jpg.GetWidth();
        t.y *= fy / (float)jpg.GetHeight();
        t.x += cx / (float)jpg.GetWidth();
        t.y += cy / (float)jpg.GetHeight();
        int x = (int)(t.x * jpg.GetWidth());
        int y = (int)(t.y * jpg.GetHeight());
        int index = (y * jpg.GetWidth() + x) * 4;
        if ((index > 0) && (index < jpg.GetWidth() * jpg.GetHeight() * 4 + 4)) {
            map[((int)(y / mapScale)) * stride + (int)(x / mapScale)] = points.size();
            glm::ivec3 color;
            color.r = jpg.GetData()[index + 0];
            color.g = jpg.GetData()[index + 1];
            color.b = jpg.GetData()[index + 2];
            colors.push_back(oc::File3d::CodeColor(color));
            points.push_back(glm::vec3(w.x, w.y, w.z));
        }
    }
#ifdef MAKE_SURFACE
    float minDst = 0.25f;
    int margin = 4;
    for (int x = 1 + margin; x < stride - margin; x++) {
        for (int y = 1 + margin; y < height - margin; y++) {
            int a = map[(y - 1) * stride + x - 1];
            int b = map[(y - 1) * stride + x];
            int c = map[y * stride + x - 1];
            int d = map[y * stride + x];
            if ((a >= 0) && (b >= 0) && (c >= 0) && (a != b) && (b != c) && (c != a)) {
                float dst = 0;
                dst += glm::length(points[a] - points[b]);
                dst += glm::length(points[b] - points[c]);
                dst += glm::length(points[c] - points[a]);
                if (dst < minDst) {
                    indices.push_back(a);
                    indices.push_back(b);
                    indices.push_back(c);
                }
            }
            if ((d >= 0) && (b >= 0) && (c >= 0) && (d != b) && (b != c) && (c != d)) {
                float dst = 0;
                dst += glm::length(points[d] - points[b]);
                dst += glm::length(points[b] - points[c]);
                dst += glm::length(points[c] - points[d]);
                if (dst < minDst) {
                    indices.push_back(d);
                    indices.push_back(c);
                    indices.push_back(b);
                }
            }
        }
    }
#endif
}

/**
 * @brief keyboardDown is called when key pressed
 * @param key is key code
 * @param x is cursor position x
 * @param y is cursor position y
 */
void keyboardDown(unsigned char key, int x, int y)
{
    /// exit
    if (key == 27)
      std::exit(0);
    /// disable upper case
    if ((key >= 'A') & (key <= 'Z'))
        key = key - 'A' + 'a';

    // map keys
    if (key == KEY_LEFT)
        keys[0] = true;
    else if (key == KEY_RIGHT)
        keys[1] = true;
    else if (key == KEY_UP)
        keys[2] = true;
    else if (key == KEY_DOWN)
        keys[3] = true;
    else if (key == KEY_NITRO)
        keys[4] = true;
}

/**
 * @brief keyboardUp is called when key released
 * @param key is key code
 * @param x is cursor position x
 * @param y is cursor position y
 */
void keyboardUp(unsigned char key, int x, int y)
{
    /// disable upper case
    bool shift = false;
    if ((key >= 'A') & (key <= 'Z')) {
        key = key - 'A' + 'a';
        shift = true;
    }

    /// map keys
    if (key == KEY_LEFT)
        keys[0] = false;
    else if (key == KEY_RIGHT)
        keys[1] = false;
    else if (key == KEY_UP)
        keys[2] = false;
    else if (key == KEY_DOWN)
        keys[3] = false;
    else if (key == KEY_NITRO)
        keys[4] = false;

    if (key == 'q') {
        if (!shift) {
            indices.clear();
            colors.clear();
            points.clear();
        }
        poseIndex++;
        if (poseIndex >= poseCount)
            poseIndex = 0;
        glutSetWindowTitle(dataset->GetFileName(poseIndex, "").c_str());
        loadPointCloud();
    } else if (key == 'e') {
        if (!shift) {
            indices.clear();
            colors.clear();
            points.clear();
        }
        poseIndex--;
        if (poseIndex < 0)
            poseIndex = poseCount - 1;
        glutSetWindowTitle(dataset->GetFileName(poseIndex, "").c_str());
        loadPointCloud();
    }
}
/**
 * @brief mouseMose is called when the mouse button was clicked
 * @param x is mouse x positon in window
 * @param y is mouse y positon in window
 */
void mouseClick(int button, int status, int x, int y)
{
    if ((button == GLUT_LEFT_BUTTON) && (status == GLUT_DOWN))
        mouseActive = !mouseActive;
    if (mouseActive) {
        mouseX = x;
        mouseY = y;
    }
}

/**
 * @brief mouseMove is called when mouse moved
 * @param x is mouse x positon in window
 * @param y is mouse y positon in window
 */
void mouseMove(int x, int y)
{
    if (mouseActive) {
        mouseX = x;
        mouseY = y;
    }
}

/**
 * @brief reshape rescales window
 * @param w is new window width
 * @param h is new window hegiht
 */
void reshape(int w, int h)
{
   resolution.x = w;
   resolution.y = h;
}

/**
 * @brief main loads data and prepares scene
 * @param argc is amount of arguments
 * @param argv is array of arguments
 * @return exit code
 */
int main(int argc, char** argv)
{
    if (argc < 2) {
        LOGI("Usage: ./dataset_viewer <path to the dataset> [obj model filename]");
        return 0;
    }

    /// get dataset
    dataset = new oc::Dataset(argv[1]);
    dataset->GetCalibration(cx, cy, fx, fy);
    dataset->GetState(poseCount, resolution.x, resolution.y);

    /// init glut
    glutInit(&argc, argv);
    glutInitWindowSize(960,540);
    glutInitContextVersion(3,0);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow(dataset->GetPath().c_str());
    //glutFullScreen();
    initializeGl();

    /// set handlers
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutMouseFunc(mouseClick);
    glutPassiveMotionFunc(mouseMove);

    /// load model
    if (argc > 2) {
        unsigned int color = oc::File3d::CodeColor(glm::ivec3(0, 128, 0));
        oc::File3d obj(dataset->GetPath() + "/" + argv[2], false);
        obj.ReadModel(25000, model);
        for (oc::Mesh& m : model) {
            for (unsigned int i = 0; i < m.vertices.size(); i++) {
                m.colors[i] = color;
            }
        }
    }

    /// load point clouds
    poseIndex = 0;
    //for (poseIndex = poseCount - 1; poseIndex >= 0; poseIndex--)
        loadPointCloud();

    /// init camera
    camera = glm::vec4(0, 1, 0, 1);
    pitch = 0;
    yaw = 0;

    /// start loop
    glutTimerFunc(0, idle, 0);
    glutMainLoop();
    return 0;
}
