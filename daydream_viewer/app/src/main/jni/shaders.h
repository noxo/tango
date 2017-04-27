#ifndef SHADERS_H
#define SHADERS_H


static const char* kTextureVertexShaders[] = {
    R"glsl(
    uniform mat4 u_MVP;
    uniform float u_X;
    uniform float u_Y;
    uniform float u_Z;
    attribute vec4 a_Position;
    attribute vec2 a_UV;
    varying vec2 v_UV;

    void main() {
      v_UV.x = a_UV.x;
      v_UV.y = 1.0 - a_UV.y;
      vec4 pos = a_Position;
      pos.x += u_X;
      pos.y += u_Y;
      pos.z += u_Z;
      gl_Position = u_MVP * pos;
    })glsl",
    R"glsl(
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
    })glsl"
};

static const char* kTextureFragmentShaders[] = {
    R"glsl(
    precision mediump float;
    uniform sampler2D color_texture;
    varying vec2 v_UV;

    void main() {
      gl_FragColor = texture2D(color_texture, v_UV);
    })glsl",
    R"glsl(
    precision mediump float;
    varying vec4 v_Color;

    void main() {
      gl_FragColor = v_Color;
    })glsl"
};

static const char* kPassthroughFragmentShaders[] = {
    R"glsl(
    precision mediump float;
    varying vec4 v_Color;

    void main() {
      gl_FragColor = v_Color;
    })glsl"
};

static const char* kReticleVertexShaders[] = { R"glsl(
    uniform mat4 u_MVP;
    attribute vec4 a_Position;
    varying vec2 v_Coords;

    void main() {
      v_Coords = a_Position.xy;
      gl_Position = u_MVP * a_Position;
    })glsl"
};

static const char* kReticleFragmentShaders[] = { R"glsl(
    precision mediump float;

    varying vec2 v_Coords;

    void main() {
      float r = length(v_Coords);
      float alpha = smoothstep(0.5, 0.6, r) * (1.0 - smoothstep(0.8, 0.9, r));
      if (alpha == 0.0) discard;
      gl_FragColor = vec4(alpha);
    })glsl"
};

#endif  // TREASUREHUNT_APP_SRC_MAIN_JNI_TREASUREHUNTSHADERS_H_ // NOLINT
