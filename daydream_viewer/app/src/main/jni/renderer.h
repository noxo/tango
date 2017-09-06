#ifndef RENDERER_H
#define RENDERER_H

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <jni.h>

#include <memory>
#include <string>
#include <thread>  // NOLINT
#include <vector>

#include "data/mesh.h"
#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_controller.h"
#include "vr/gvr/capi/include/gvr_types.h"

class Renderer {
 public:
  Renderer(gvr_context* gvr_context, std::string filename);
  ~Renderer();
  void InitializeGl();
  void DrawFrame();
  void OnTriggerEvent();
  void OnPause();
  void OnResume();

 private:
  void PrepareFramebuffer();
  int LoadGLShader(int type, const char** shadercode);

  enum ViewType {
    kLeftView,
    kRightView
  };
  void DrawWorld(ViewType view);
  void DrawModel(ViewType view);

  std::unique_ptr<gvr::GvrApi> gvr_api_;
  std::unique_ptr<gvr::BufferViewportList> viewport_list_;
  std::unique_ptr<gvr::SwapChain> swapchain_;
  gvr::BufferViewport viewport_left_;
  gvr::BufferViewport viewport_right_;

  int model_program_;
  int model_position_param_;
  int model_translatex_param_;
  int model_translatey_param_;
  int model_translatez_param_;
  int model_uv_param_;
  int model_modelview_projection_param_;
  const gvr::Sizei reticle_render_size_;

  gvr::Mat4f head_view_;
  gvr::Mat4f model_model_;
  gvr::Sizei render_size_;

  gvr::Mat4f modelview_projection_model_[2];
  gvr::Mat4f modelview_model_[2];

  gvr::ViewerType gvr_viewer_type_;
  std::vector<oc::Mesh> static_meshes_;
  glm::vec4 cur_position;
  glm::vec4 dst_position;
};

#endif
