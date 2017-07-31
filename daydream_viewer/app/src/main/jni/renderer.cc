#include "renderer.h"  // NOLINT
#include "shaders.h"  // NOLINT
#include "data/file3d.h"

#include <android/log.h>
#include <assert.h>
#include <stdlib.h>
#include <cmath>
#include <random>

namespace {
static const float kReticleDistance = 3.0f;
static const int kCoordsPerVertex = 3;
static const uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;
static const float kAngleLimit = 0.12f;
static const float kPitchLimit = 0.12f;
static const float kYawLimit = 0.12f;

float reticle_vertices_[] = {
        -1.f, 1.f, 0.0f,
        -1.f, -1.f, 0.0f,
        1.f, 1.f, 0.0f,
        -1.f, -1.f, 0.0f,
        1.f, -1.f, 0.0f,
        1.f, 1.f, 0.0f,
};

static std::array<float, 16> MatrixToGLArray(const gvr::Mat4f& matrix) {
  std::array<float, 16> result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result[j * 4 + i] = matrix.m[i][j];
    }
  }
  return result;
}

static std::array<float, 4> MatrixVectorMul(const gvr::Mat4f& matrix, const std::array<float, 4>& vec) {
  std::array<float, 4> result;
  for (int i = 0; i < 4; ++i) {
    result[i] = 0;
    for (int k = 0; k < 4; ++k) {
      result[i] += matrix.m[i][k] * vec[k];
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

static std::array<float, 3> Vec4ToVec3(const std::array<float, 4>& vec) {
  return {vec[0], vec[1], vec[2]};
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

static gvr::Mat4f ControllerQuatToMatrix(const gvr::ControllerQuat& quat) {
  gvr::Mat4f result;
  const float x = quat.qx;
  const float x2 = quat.qx * quat.qx;
  const float y = quat.qy;
  const float y2 = quat.qy * quat.qy;
  const float z = quat.qz;
  const float z2 = quat.qz * quat.qz;
  const float w = quat.qw;
  const float xy = quat.qx * quat.qy;
  const float xz = quat.qx * quat.qz;
  const float xw = quat.qx * quat.qw;
  const float yz = quat.qy * quat.qz;
  const float yw = quat.qy * quat.qw;
  const float zw = quat.qz * quat.qw;

  const float m11 = 1.0f - 2.0f * y2 - 2.0f * z2;
  const float m12 = 2.0f * (xy - zw);
  const float m13 = 2.0f * (xz + yw);
  const float m21 = 2.0f * (xy + zw);
  const float m22 = 1.0f - 2.0f * x2 - 2.0f * z2;
  const float m23 = 2.0f * (yz - xw);
  const float m31 = 2.0f * (xz - yw);
  const float m32 = 2.0f * (yz + xw);
  const float m33 = 1.0f - 2.0f * x2 - 2.0f * y2;

  return {{{m11, m12, m13, 0.0f},
           {m21, m22, m23, 0.0f},
           {m31, m32, m33, 0.0f},
           {0.0f, 0.0f, 0.0f, 1.0f}}};
}
}  // anonymous namespace

Renderer::Renderer(gvr_context* gvr_context, std::string filename)
    : gvr_api_(gvr::GvrApi::WrapNonOwned(gvr_context)),
      viewport_left_(gvr_api_->CreateBufferViewport()),
      viewport_right_(gvr_api_->CreateBufferViewport()),
      reticle_render_size_{128, 128},
      gvr_controller_api_(nullptr),
      gvr_viewer_type_(gvr_api_->GetViewerType()) {
  ResumeControllerApiAsNeeded();
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
  const int pass_through_shader = LoadGLShader(GL_FRAGMENT_SHADER, &kPassthroughFragmentShaders[0]);
  const int reticle_vertex_shader = LoadGLShader(GL_VERTEX_SHADER, &kReticleVertexShaders[0]);
  const int reticle_fragment_shader = LoadGLShader(GL_FRAGMENT_SHADER, &kReticleFragmentShaders[0]);

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

  reticle_program_ = glCreateProgram();
  glAttachShader(reticle_program_, reticle_vertex_shader);
  glAttachShader(reticle_program_, reticle_fragment_shader);
  glLinkProgram(reticle_program_);
  glUseProgram(reticle_program_);

  reticle_position_param_ = glGetAttribLocation(reticle_program_, "a_Position");
  reticle_modelview_projection_param_ = glGetUniformLocation(reticle_program_, "u_MVP");

  // Object first appears directly in front of user.
  model_model_ = {{{100.0f, 0.0f, 0.0f, 0.0f},
                   {0.0f, 100.0f, 0.0f, 0.0},
                   {0.0f, 0.0f, 100.0f, 0.0f},
                   {0.0f, 0.0f, 0.0f, 1.0f}}};
  const float rs = 0.04f;  // Reticle scale.
  model_reticle_ = {{{rs, 0.0f, 0.0f, 0.0f},
                     {0.0f, rs, 0.0f, 0.0f},
                     {0.0f, 0.0f, rs, -kReticleDistance},
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

void Renderer::ResumeControllerApiAsNeeded() {
  switch (gvr_viewer_type_) {
    case GVR_VIEWER_TYPE_CARDBOARD:
      gvr_controller_api_.reset();
      break;
    case GVR_VIEWER_TYPE_DAYDREAM:
      if (!gvr_controller_api_) {
        // Initialized controller api.
        gvr_controller_api_.reset(new gvr::ControllerApi);
        gvr_controller_api_->Init(gvr::ControllerApi::DefaultOptions(), gvr_api_->cobj());
      }
      gvr_controller_api_->Resume();
      break;
    default:
      LOGE("unexpected viewer type.");
      break;
  }
}

void Renderer::ProcessControllerInput() {
  const int old_status = gvr_controller_state_.GetApiStatus();
  const int old_connection_state = gvr_controller_state_.GetConnectionState();

  // Read current controller state.
  gvr_controller_state_.Update(*gvr_controller_api_);
  if (gvr_controller_state_.GetApiStatus() != old_status ||
      gvr_controller_state_.GetConnectionState() != old_connection_state) {
  }

  // Trigger click event if app/click button is clicked.
  if (gvr_controller_state_.GetButtonDown(GVR_CONTROLLER_BUTTON_APP) ||
      gvr_controller_state_.GetButtonDown(GVR_CONTROLLER_BUTTON_CLICK)) {
    OnTriggerEvent();
  }
}

void Renderer::DrawFrame() {
  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_DAYDREAM) {
    ProcessControllerInput();
  }
  PrepareFramebuffer();
  gvr::Frame frame = swapchain_->AcquireFrame();

  // A client app does its rendering here.
  gvr::ClockTimePoint target_time = gvr::GvrApi::GetTimePointNow();
  target_time.monotonic_system_time_nanos += kPredictionTimeWithoutVsyncNanos;
  gvr::BufferViewport* viewport[2] = { &viewport_left_, &viewport_right_, };
  head_view_ = gvr_api_->GetHeadSpaceFromStartSpaceRotation(target_time);
  viewport_list_->SetToRecommendedBufferViewports();
  gvr::BufferViewport reticle_viewport = gvr_api_->CreateBufferViewport();
  reticle_viewport.SetSourceBufferIndex(1);
  reticle_viewport.SetReprojection(GVR_REPROJECTION_NONE);
  const gvr_rectf fullscreen = { 0, 1, 0, 1 };
  reticle_viewport.SetSourceUv(fullscreen);

  gvr::Mat4f controller_matrix = ControllerQuatToMatrix(gvr_controller_state_.GetOrientation());
  model_cursor_ = MatrixMul(controller_matrix, model_reticle_);

  gvr::Mat4f eye_views[2];
  for (int eye = 0; eye < 2; ++eye) {
    const gvr::Eye gvr_eye = eye == 0 ? GVR_LEFT_EYE : GVR_RIGHT_EYE;
    const gvr::Mat4f eye_from_head = gvr_api_->GetEyeFromHeadMatrix(gvr_eye);
    eye_views[eye] = MatrixMul(eye_from_head, head_view_);

    viewport_list_->GetBufferViewport(eye, viewport[eye]);
    reticle_viewport.SetTransform(MatrixMul(eye_from_head, model_reticle_));
    reticle_viewport.SetTargetEye(gvr_eye);
    viewport_list_->SetBufferViewport(2 + eye, reticle_viewport);

    modelview_model_[eye] = MatrixMul(eye_views[eye], model_model_);
    const gvr_rectf fov = viewport[eye]->GetSourceFov();
    const gvr::Mat4f perspective = PerspectiveMatrixFromView(fov, 1, 10000);
    modelview_projection_model_[eye] = MatrixMul(perspective, modelview_model_[eye]);
    modelview_projection_cursor_[eye] = MatrixMul(perspective, MatrixMul(eye_views[eye], model_cursor_));
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

  if (gvr_viewer_type_ == GVR_VIEWER_TYPE_CARDBOARD)
    DrawCardboardReticle();
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

void Renderer::OnTriggerEvent() {
  glm::mat4 left = glm::make_mat4(MatrixToGLArray(modelview_model_[kLeftView]).data());
  glm::mat4 right = glm::make_mat4(MatrixToGLArray(modelview_model_[kRightView]).data());
  glm::vec4 lv = glm::vec4(0, 0, 1, 1) * left;
  glm::vec4 rv = glm::vec4(0, 0, 1, 1) * right;
  lv /= fabs(lv.w);
  rv /= fabs(rv.w);
  dst_position += (lv + rv) * 0.0025f;
}

void Renderer::OnPause() {
  gvr_api_->PauseTracking();
  if (gvr_controller_api_) gvr_controller_api_->Pause();
}

void Renderer::OnResume() {
  gvr_api_->ResumeTracking();
  gvr_api_->RefreshViewerProfile();
  gvr_viewer_type_ = gvr_api_->GetViewerType();
  ResumeControllerApiAsNeeded();
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
    if (gvr_viewer_type_ == GVR_VIEWER_TYPE_DAYDREAM)
      DrawDaydreamCursor(view);
}

void Renderer::DrawModel(ViewType view) {
  glUseProgram(model_program_);
  float* matrix = MatrixToGLArray(modelview_projection_model_[view]).data();
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
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

void Renderer::DrawDaydreamCursor(ViewType view) {
  glUseProgram(reticle_program_);
    glUniformMatrix4fv(reticle_modelview_projection_param_, 1, GL_FALSE, MatrixToGLArray(modelview_projection_cursor_[view]).data());
  glVertexAttribPointer(reticle_position_param_, kCoordsPerVertex, GL_FLOAT, false, 0, reticle_vertices_);
  glEnableVertexAttribArray(reticle_position_param_);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(reticle_position_param_);
}

void Renderer::DrawCardboardReticle() {
  glViewport(0, 0, reticle_render_size_.width, reticle_render_size_.height);
  glUseProgram(reticle_program_);
  const gvr::Mat4f uniform_matrix = {{{1.f, 0.f, 0.f, 0.f},
                                      {0.f, 1.f, 0.f, 0.f},
                                      {0.f, 0.f, 1.f, 0.f},
                                      {0.f, 0.f, 0.f, 1.f}}};
  glUniformMatrix4fv(reticle_modelview_projection_param_, 1, GL_FALSE, MatrixToGLArray(uniform_matrix).data());
  glVertexAttribPointer(reticle_position_param_, kCoordsPerVertex, GL_FLOAT, false, 0, reticle_vertices_);
  glEnableVertexAttribArray(reticle_position_param_);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(reticle_position_param_);
}
