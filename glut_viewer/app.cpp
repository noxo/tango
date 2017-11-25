///----------------------------------------------------------------------------------------
/**
 * \file       app.cpp
 * \author     Vonasek Lubos
 * \date       2017/08/01
 * \brief      Runable code of project.
**/
///----------------------------------------------------------------------------------------

#include <gl/scene.h>
#include <GL/freeglut.h>

//shader
const char* kTextureShader[] = {R"glsl(
    #version 100
    precision highp float;
    uniform mat4 u_MVP;
    uniform float u_X;
    uniform float u_Y;
    uniform float u_Z;
    attribute vec4 a_Position;
    attribute vec2 a_UV;
    varying vec2 v_UV;
    varying float v_Z;

    void main() {
      v_UV = a_UV;
      v_UV.t = 1.0 - a_UV.t;
      vec4 pos = a_Position;
      pos.x += u_X;
      pos.y += u_Y;
      pos.z += u_Z;
      gl_Position = u_MVP * pos;
      v_Z = gl_Position.z * 0.0015;
    })glsl",

    R"glsl(
    #version 100
    precision highp float;
    uniform float u_contrast;
    uniform float u_gamma;
    uniform float u_saturation;
    uniform sampler2D u_color_texture;
    varying vec2 v_UV;
    varying float v_Z;

    void main() {
      gl_FragColor = texture2D(u_color_texture, v_UV);
      if (gl_FragColor.a < 0.5)
        discard;

      //contrast
      gl_FragColor.r -= (0.5 - gl_FragColor.r) * u_contrast * 2.0;
      gl_FragColor.g -= (0.5 - gl_FragColor.g) * u_contrast * 2.0;
      gl_FragColor.b -= (0.5 - gl_FragColor.b) * u_contrast * 2.0;
      //gamma
      gl_FragColor.rgb += u_gamma;
      //saturation
      float c = (gl_FragColor.r + gl_FragColor.g + gl_FragColor.b) / 3.0;
      gl_FragColor.r -= (c - gl_FragColor.r) * u_saturation * 2.0;
      gl_FragColor.g -= (c - gl_FragColor.g) * u_saturation * 2.0;
      gl_FragColor.b -= (c - gl_FragColor.b) * u_saturation * 2.0;
      //dark "fog"
      gl_FragColor.r = clamp(gl_FragColor.r - v_Z, 0.0, 1.0);
      gl_FragColor.g = clamp(gl_FragColor.g - v_Z, 0.0, 1.0);
      gl_FragColor.b = clamp(gl_FragColor.b - v_Z, 0.0, 1.0);
      gl_FragColor.a = 1.0;
    })glsl"
};

//shader stuff
GLuint model_program_;
GLint model_position_param_;
GLint model_texture_param_;
GLint model_uv_param_;
GLint model_contrast_param_;
GLint model_gamma_param_;
GLint model_saturation_param_;
GLint model_translatex_param_;
GLint model_translatey_param_;
GLint model_translatez_param_;
GLint model_modelview_projection_param_;

bool mouseActive = true;
float mod_contrast = 0;
float mod_gamma = 0;
float mod_saturation = 0;
float mouseX;          ///< Last mouse X
float mouseY;          ///< Last mouse X
float pitch, npitch;   ///< Camera pitch
float yaw, nyaw;       ///< Camera yaw
glm::vec4 camera, ncam;///< Camera position
glm::mat4 proj;        ///< Camera projection
glm::mat4 view;        ///< Camera view
bool keys[5];          ///< State of keyboard
glm::ivec2 resolution; ///< Screen resolution
oc::GLScene scene;     ///< Scene

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
    /// interpolate the movement
    ncam = ncam * 0.95f + camera * 0.05f;
    npitch = npitch * 0.95f + pitch * 0.05f;
    nyaw = nyaw * 0.95f + yaw * 0.05f;

    /// set buffers
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, resolution.x, resolution.y);

    /// set view
    float aspect = resolution.x / (float)resolution.y;
    glm::vec3 eye = glm::vec3(0, 0, 0);
    glm::vec3 center = glm::vec3(sin(nyaw), -sin(npitch), -cos(nyaw));
    glm::vec3 up = glm::vec3(0, 1, 0);
    proj = glm::perspective(glm::radians(60.0f), aspect, 0.01f, 500.0f);
    view = glm::lookAt(eye, center, up);

    /// bind shader
    glUseProgram(model_program_);
    glUniform1i(model_texture_param_, 2);
    glUniform1f(model_contrast_param_, mod_contrast);
    glUniform1f(model_gamma_param_, mod_gamma);
    glUniform1f(model_saturation_param_, mod_saturation);
    glUniform1f(model_translatex_param_, ncam.x);
    glUniform1f(model_translatey_param_, ncam.y);
    glUniform1f(model_translatez_param_, ncam.z);
    glUniformMatrix4fv(model_modelview_projection_param_, 1, GL_FALSE, glm::value_ptr(proj * view));

    /// render data
    glActiveTexture(GL_TEXTURE2);
    scene.UpdateVisibility(proj * view, ncam);
    scene.Render(model_position_param_, model_uv_param_);
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(0);

    /// render info
    std::string text;
    char buffer[1024];
    sprintf(buffer, "pos(x=%.1f, y=%.1f, z=%.1f)", camera.x, camera.y, camera.z);
    text += buffer;
    sprintf(buffer, ", mod(c=%.1f, g=%.1f, s=%.1f)", mod_contrast, mod_gamma, mod_saturation);
    text += buffer;
    sprintf(buffer, ", binding=%d, vertices=%ldK", scene.AmountOfBinding(), scene.AmountOfVertices() / 1000);
    text += buffer;
    glColor3f(1, 0, 0);
    glRasterPos2f(-1, 0.9f);
    glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (unsigned char*)text.c_str());

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
      side *= 0.3f;
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
      forward *= 0.3f;
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
  model_uv_param_ = glGetAttribLocation(model_program_, "a_UV");
  model_modelview_projection_param_ = glGetUniformLocation(model_program_, "u_MVP");
  model_texture_param_ = glGetUniformLocation(model_program_, "u_color_texture");
  model_contrast_param_ = glGetUniformLocation(model_program_, "u_contrast");
  model_gamma_param_ = glGetUniformLocation(model_program_, "u_gamma");
  model_saturation_param_ = glGetUniformLocation(model_program_, "u_saturation");
  model_translatex_param_ = glGetUniformLocation(model_program_, "u_X");
  model_translatey_param_ = glGetUniformLocation(model_program_, "u_Y");
  model_translatez_param_ = glGetUniformLocation(model_program_, "u_Z");
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

    // color modification
    if (key == 'u')
        mod_contrast += 0.1f;
    else if (key == 'j')
        mod_contrast -= 0.1f;
    else if (key == 'i')
        mod_gamma += 0.1f;
    else if (key == 'k')
        mod_gamma -= 0.1f;
    else if (key == 'o')
        mod_saturation += 0.1f;
    else if (key == 'l')
        mod_saturation -= 0.1f;
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
    if ((key >= 'A') & (key <= 'Z'))
        key = key - 'A' + 'a';

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
 * @brief Loads obj file into scene
 * @param filename is path to file
 * @param position is model translation
 */
void load(std::string filename, glm::vec3 position)
{
    std::vector<oc::Mesh> meshes;
    oc::File3d io(filename, false);
    io.ReadModel(INT_MAX, meshes);
    if (glm::length(position) > 0.005f)
        for (oc::Mesh& m : meshes)
            for (glm::vec3& v : m.vertices)
                v += position;
    scene.Load(meshes);
}

/**
 * @brief main loads data and prepares scene
 * @param argc is amount of arguments
 * @param argv is array of arguments
 * @return exit code
 */
int main(int argc, char** argv)
{
    if (argc != 2)
        return 0;

    /// init glut
    glutInit(&argc, argv);
    glutInitWindowSize(960,540);
    glutInitContextVersion(3,0);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB);
    glutCreateWindow("GLUT OBJ Viewer");
    //glutFullScreen();
    initializeGl();

    /// set handlers
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutMouseFunc(mouseClick);
    glutPassiveMotionFunc(mouseMove);

    /// load data
    if(strcmp(argv[1], "--demo") == 0) {
        load("3d/church/model.obj", glm::vec3(30, -7.25, 0.25f));
        load("3d/cathedrale/model.obj", glm::vec3(0, 0, 0));
        load("3d/kingshall/model.obj", glm::vec3(-21.75f, -4.5f, -14.5f));
        load("3d/vestibule/model.obj", glm::vec3(-46.25f, -7.75f, 1.35f));
        load("3d/gallery/model.obj", glm::vec3(-1.25f, -8.0f, 38.0f));
    }
    else
        load(argv[1], glm::vec3(0, 0, 0));
    scene.Process();
    ncam = camera = glm::vec4(0, 1, 0, 1);
    npitch = pitch = 0;
    nyaw = yaw = 0;

    /// start loop
    glutTimerFunc(0, idle, 0);
    glutMainLoop();
    return 0;
}
