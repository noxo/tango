#include "renderer.h"

static const char* kTextureShader[] = {R"glsl(
    #version 100
    precision highp float;
    uniform mat4 u_MVP;
    uniform float u_X;
    uniform float u_Y;
    uniform float u_Z;
    attribute vec4 a_Position;
    attribute vec2 a_UV;
    varying vec2 v_UV;

    void main() {
      v_UV = a_UV;
      v_UV.t = 1.0 - a_UV.t;
      vec4 pos = a_Position;
      pos.x += u_X;
      pos.y += u_Y;
      pos.z += u_Z;
      gl_Position = u_MVP * pos;
    })glsl",

    R"glsl(
    #version 100
    precision highp float;
    uniform sampler2D u_color_texture;
    varying vec2 v_UV;

    void main() {
      gl_FragColor = texture2D(u_color_texture, v_UV);
      gl_FragColor.a = 1.0;
    })glsl"
};

void Renderer::Load(std::string filename) {
  static_meshes_.clear();
  if (!filename.empty())
  {
    oc::File3d io(filename, false);
    io.ReadModel(20000, static_meshes_);
  }
  cur_position = glm::vec4();
  dst_position = glm::vec4();
}

void Renderer::InitializeGl() {
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
  model_translatex_param_ = glGetUniformLocation(model_program_, "u_X");
  model_translatey_param_ = glGetUniformLocation(model_program_, "u_Y");
  model_translatez_param_ = glGetUniformLocation(model_program_, "u_Z");
}

void Renderer::OnTriggerEvent(float x, float y, float z, float* view) {
  glm::vec4 dir = glm::vec4(x, y, z, 1) * glm::make_mat4(view);
  dir /= fabs(dir.w);
  dst_position += dir;
}

void Renderer::DrawModel(float* view) {
  glUseProgram(model_program_);
  glUniform1i(model_texture_param_, 0);
  glUniform1f(model_translatex_param_, cur_position.x);
  glUniform1f(model_translatey_param_, cur_position.y);
  glUniform1f(model_translatez_param_, cur_position.z);
  glUniformMatrix4fv(model_modelview_projection_param_, 1, GL_FALSE, view);

  glActiveTexture(GL_TEXTURE0);
  for(oc::Mesh& mesh : static_meshes_) {
    if (mesh.image && (mesh.image->GetTexture() == -1)) {
      GLuint textureID;
      glGenTextures(1, &textureID);
      mesh.image->SetTexture(textureID);
      glBindTexture(GL_TEXTURE_2D, textureID);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mesh.image->GetWidth(), mesh.image->GetHeight(),
                             0, GL_RGB, GL_UNSIGNED_BYTE, mesh.image->GetData());
    }
    glBindTexture(GL_TEXTURE_2D, (unsigned int)mesh.image->GetTexture());
    glEnableVertexAttribArray((GLuint) model_position_param_);
    glEnableVertexAttribArray((GLuint) model_uv_param_);
    glVertexAttribPointer((GLuint) model_position_param_, 3, GL_FLOAT, GL_FALSE, 0, mesh.vertices.data());
    glVertexAttribPointer((GLuint) model_uv_param_, 2, GL_FLOAT, GL_FALSE, 0, mesh.uv.data());
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei) mesh.vertices.size());
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  glDisableVertexAttribArray((GLuint) model_position_param_);
  glDisableVertexAttribArray((GLuint) model_uv_param_);
  glUseProgram(0);
}

void Renderer::Update()
{
  cur_position = 0.95f * cur_position + 0.05f * dst_position;
}

std::string jstring2string(JNIEnv* env, jstring name)
{
  const char *s = env->GetStringUTFChars(name,NULL);
  std::string str( s );
  env->ReleaseStringUTFChars(name,s);
  return str;
}

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_lvonasek_daydreamOBJ_MainActivity_##method_name

extern "C" {

Renderer renderer;

JNI_METHOD(void, nativeLoadModel)
(JNIEnv *env, jobject, jstring filename) {
  renderer.Load(jstring2string(env, filename));
}

JNI_METHOD(void, nativeInitializeGl)
(JNIEnv *, jobject) {
  renderer.InitializeGl();
}

JNI_METHOD(void, nativeOnTriggerEvent)
(JNIEnv *env, jobject, jfloat x, jfloat y, jfloat z, jfloatArray matrix_) {
  jfloat *matrix = env->GetFloatArrayElements(matrix_, NULL);
  renderer.OnTriggerEvent(x, y, z, matrix);
  env->ReleaseFloatArrayElements(matrix_, matrix, 0);
}

JNI_METHOD(void, nativeDrawFrame)
(JNIEnv *env, jobject, jfloatArray matrix_) {
  jfloat *matrix = env->GetFloatArrayElements(matrix_, NULL);
  renderer.DrawModel(matrix);
  env->ReleaseFloatArrayElements(matrix_, matrix, 0);
}

JNI_METHOD(void, nativeUpdate)
(JNIEnv *, jobject) {
  renderer.Update();
}

}  // extern "C"
