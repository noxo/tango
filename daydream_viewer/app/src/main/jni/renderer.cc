#include "renderer.h"  // NOLINT
#include "shaders.h"  // NOLINT
#include "data/file3d.h"

namespace {
static const uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;

static std::array<float, 16> MatrixToGLArray(const gvr::Mat4f& matrix) {
  std::array<float, 16> result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result[j * 4 + i] = matrix.m[i][j];
    }
  }
  return result;
}

static gvr::Mat4f MatrixMul(const gvr::Mat4f& matrix1, const gvr::Mat4f& matrix2) {
  gvr::Mat4f result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
      for (int k = 0; k < 4; ++k) {
        result.m[i][j] += matrix1.m[i][k] * matrix2.m[k][j];
      }
    }
  }
  return result;
}

static gvr::Mat4f PerspectiveMatrixFromView(const gvr::Rectf& fov, float z_near, float z_far) {
  gvr::Mat4f result;
  const float x_left = -std::tan(fov.left * M_PI / 180.0f) * z_near;
  const float x_right = std::tan(fov.right * M_PI / 180.0f) * z_near;
  const float y_bottom = -std::tan(fov.bottom * M_PI / 180.0f) * z_near;
  const float y_top = std::tan(fov.top * M_PI / 180.0f) * z_near;
  const float zero = 0.0f;

  assert(x_left < x_right && y_bottom < y_top && z_near < z_far && z_near > zero && z_far > zero);
  const float X = (2 * z_near) / (x_right - x_left);
  const float Y = (2 * z_near) / (y_top - y_bottom);
  const float A = (x_right + x_left) / (x_right - x_left);
  const float B = (y_top + y_bottom) / (y_top - y_bottom);
  const float C = (z_near + z_far) / (z_near - z_far);
  const float D = (2 * z_near * z_far) / (z_near - z_far);

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
    }
  }
  result.m[0][0] = X;
  result.m[0][2] = A;
  result.m[1][1] = Y;
  result.m[1][2] = B;
  result.m[2][2] = C;
  result.m[2][3] = D;
  result.m[3][2] = -1;

  return result;
}

static gvr::Rectf ModulateRect(const gvr::Rectf& rect, float width, float height) {
  gvr::Rectf result = {rect.left * width, rect.right * width, rect.bottom * height, rect.top * height};
  return result;
}

static gvr::Recti CalculatePixelSpaceRect(const gvr::Sizei& texture_size, const gvr::Rectf& texture_rect) {
  const float width = static_cast<float>(texture_size.width);
  const float height = static_cast<float>(texture_size.height);
  const gvr::Rectf rect = ModulateRect(texture_rect, width, height);
  const gvr::Recti result = {
      static_cast<int>(rect.left), static_cast<int>(rect.right),
      static_cast<int>(rect.bottom), static_cast<int>(rect.top)};
  return result;
}

static gvr::Sizei HalfPixelCount(const gvr::Sizei& in) {
  // Scale each dimension by sqrt(2)/2 ~= 7/10ths.
  gvr::Sizei out;
  out.width = (7 * in.width) / 10;
  out.height = (7 * in.height) / 10;
  return out;
}
}  // anonymous namespace

Renderer::Renderer(gvr_context* gvr_context, std::string filename)
    : gvr_api_(gvr::GvrApi::WrapNonOwned(gvr_context)),
      viewport_left_(gvr_api_->CreateBufferViewport()),
      viewport_right_(gvr_api_->CreateBufferViewport()),
      reticle_render_size_{128, 128},
      gvr_viewer_type_(gvr_api_->GetViewerType()) {
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_CARDBOARD) {
    LOGI("Viewer type: CARDBOARD");
  } else if (gvr_viewer_type_ == GVR_VIEWER_TYPE_DAYDREAM) {
    LOGI("Viewer type: DAYDREAM");
  } else {
    LOGE("Unexpected viewer type.");
  }

  oc::File3d io(filename, false);
  io.ReadModel(20000, static_meshes_);
}

Renderer::~Renderer() {
}

void Renderer::InitializeGl() {
  gvr_api_->InitializeGl();

  const int vertex_shader = LoadGLShader(GL_VERTEX_SHADER, &kTextureVertexShaders[0]);
  const int fragment_shader = LoadGLShader(GL_FRAGMENT_SHADER, &kTextureFragmentShaders[0]);

  model_program_ = glCreateProgram();
  glAttachShader(model_program_, vertex_shader);
  glAttachShader(model_program_, fragment_shader);
  glLinkProgram(model_program_);
  glUseProgram(model_program_);

  model_position_param_ = glGetAttribLocation(model_program_, "a_Position");
  model_uv_param_ = glGetAttribLocation(model_program_, "a_UV");
  model_modelview_projection_param_ = glGetUniformLocation(model_program_, "u_MVP");
  model_translatex_param_ = glGetUniformLocation(model_program_, "u_X");
  model_translatey_param_ = glGetUniformLocation(model_program_, "u_Y");
  model_translatez_param_ = glGetUniformLocation(model_program_, "u_Z");

  // Object first appears directly in front of user.
  model_model_ = {{{100.0f, 0.0f, 0.0f, 0.0f},
                   {0.0f, 100.0f, 0.0f, 0.0},
                   {0.0f, 0.0f, 100.0f, 0.0f},
                   {0.0f, 0.0f, 0.0f, 1.0f}}};

  // Because we are using 2X MSAA, we can render to half as many pixels and
  // achieve similar quality.
  render_size_ = HalfPixelCount(gvr_api_->GetMaximumEffectiveRenderTargetSize());
  std::vector<gvr::BufferSpec> specs;

  specs.push_back(gvr_api_->CreateBufferSpec());
  specs[0].SetColorFormat(GVR_COLOR_FORMAT_RGBA_8888);
  specs[0].SetDepthStencilFormat(GVR_DEPTH_STENCIL_FORMAT_DEPTH_16);
  specs[0].SetSamples(2);
  specs[0].SetSize(render_size_);

  specs.push_back(gvr_api_->CreateBufferSpec());
  specs[1].SetSize(reticle_render_size_);
  specs[1].SetColorFormat(GVR_COLOR_FORMAT_RGBA_8888);
  specs[1].SetDepthStencilFormat(GVR_DEPTH_STENCIL_FORMAT_NONE);
  specs[1].SetSamples(1);
  swapchain_.reset(new gvr::SwapChain(gvr_api_->CreateSwapChain(specs)));

  viewport_list_.reset(
      new gvr::BufferViewportList(gvr_api_->CreateEmptyBufferViewportList()));
}

void Renderer::DrawFrame() {
  PrepareFramebuffer();
  gvr::Frame frame = swapchain_->AcquireFrame();

  // A client app does its rendering here.
  gvr::ClockTimePoint target_time = gvr::GvrApi::GetTimePointNow();
  target_time.monotonic_system_time_nanos += kPredictionTimeWithoutVsyncNanos;
  gvr::BufferViewport* viewport[2] = { &viewport_left_, &viewport_right_, };
  head_view_ = gvr_api_->GetHeadSpaceFromStartSpaceRotation(target_time);
  viewport_list_->SetToRecommendedBufferViewports();

  gvr::Mat4f eye_views[2];
  for (int eye = 0; eye < 2; ++eye) {
    const gvr::Eye gvr_eye = eye == 0 ? GVR_LEFT_EYE : GVR_RIGHT_EYE;
    const gvr::Mat4f eye_from_head = gvr_api_->GetEyeFromHeadMatrix(gvr_eye);
    eye_views[eye] = MatrixMul(eye_from_head, head_view_);

    viewport_list_->GetBufferViewport(eye, viewport[eye]);
    modelview_model_[eye] = MatrixMul(eye_views[eye], model_model_);
    const gvr_rectf fov = viewport[eye]->GetSourceFov();
    const gvr::Mat4f perspective = PerspectiveMatrixFromView(fov, 1, 10000);
    modelview_projection_model_[eye] = MatrixMul(perspective, modelview_model_[eye]);
  }

  cur_position = 0.95f * cur_position + 0.05f * dst_position;

  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_BLEND);

  // Draw the world.
  frame.BindBuffer(0);
  glClearColor(0.1f, 0.1f, 0.1f, 0.5f);  // Dark background so text shows up.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  DrawWorld(kLeftView);
  DrawWorld(kRightView);
  frame.Unbind();

  frame.BindBuffer(1);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // Transparent background.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  frame.Unbind();

  // Submit frame.
  frame.Submit(*viewport_list_, head_view_);
}

void Renderer::PrepareFramebuffer() {
  // Because we are using 2X MSAA, we can render to half as many pixels and
  // achieve similar quality.
  const gvr::Sizei recommended_size = HalfPixelCount(gvr_api_->GetMaximumEffectiveRenderTargetSize());
  if (render_size_.width != recommended_size.width || render_size_.height != recommended_size.height) {
    // We need to resize the framebuffer.
    swapchain_->ResizeBuffer(0, recommended_size);
    render_size_ = recommended_size;
  }
}

void Renderer::OnTriggerEvent(float value) {
  glm::mat4 left = glm::make_mat4(MatrixToGLArray(modelview_model_[kLeftView]).data());
  glm::mat4 right = glm::make_mat4(MatrixToGLArray(modelview_model_[kRightView]).data());
  glm::vec4 lv = glm::vec4(0, 0, 1, 1) * left;
  glm::vec4 rv = glm::vec4(0, 0, 1, 1) * right;
  lv /= fabs(lv.w);
  rv /= fabs(rv.w);
  lv = glm::normalize(lv);
  rv = glm::normalize(rv);
  dst_position += (lv + rv) * 0.5f * value;
}

void Renderer::OnPause() {
  gvr_api_->PauseTracking();
}

void Renderer::OnResume() {
  gvr_api_->ResumeTracking();
  gvr_api_->RefreshViewerProfile();
  gvr_viewer_type_ = gvr_api_->GetViewerType();
}

int Renderer::LoadGLShader(int type, const char** shadercode) {
  int shader = glCreateShader(type);
  glShaderSource(shader, 1, shadercode, nullptr);
  glCompileShader(shader);

  const unsigned int BUFFER_SIZE = 512;
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  GLsizei length = 0;
  glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);
  if (length > 0)
    LOGI("GLSL compile log: %s\n%s", buffer, *shadercode);

  // Get the compilation status.
  int compileStatus;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

  // If the compilation failed, delete the shader.
  if (compileStatus == 0) {
    glDeleteShader(shader);
    shader = 0;
  }

  return shader;
}

void Renderer::DrawWorld(ViewType view) {
    const gvr::BufferViewport& viewport = view == kLeftView ? viewport_left_ : viewport_right_;
    const gvr::Recti pixel_rect = CalculatePixelSpaceRect(render_size_, viewport.GetSourceUv());
    glViewport(pixel_rect.left, pixel_rect.bottom,
               pixel_rect.right - pixel_rect.left,
               pixel_rect.top - pixel_rect.bottom);
    DrawModel(view);
}

void Renderer::DrawModel(ViewType view) {
  glUseProgram(model_program_);
  glUniform1f(model_translatex_param_, cur_position.x);
  glUniform1f(model_translatey_param_, cur_position.y);
  glUniform1f(model_translatez_param_, cur_position.z);
  glUniformMatrix4fv(model_modelview_projection_param_, 1, GL_FALSE, MatrixToGLArray(modelview_projection_model_[view]).data());

  glEnableVertexAttribArray(model_position_param_);
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
    glVertexAttribPointer(model_position_param_, 3, GL_FLOAT, false, 0, mesh.vertices.data());
    glBindTexture(GL_TEXTURE_2D, (unsigned int)mesh.image->GetTexture());
    glVertexAttribPointer(model_uv_param_, 2, GL_FLOAT, false, 0, mesh.uv.data());
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertices.size());
  }
  glDisableVertexAttribArray(model_position_param_);
}
