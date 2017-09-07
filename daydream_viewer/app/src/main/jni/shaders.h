#ifndef SHADERS_H
#define SHADERS_H

static const char* kTextureVertexShaders[] = {
    R"glsl(
    precision mediump float;
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
    })glsl"
};

static const char* kTextureFragmentShaders[] = {
    R"glsl(
    precision mediump float;
    uniform sampler2D color_texture;
    varying vec2 v_UV;

    void main() {
      gl_FragColor = texture2D(color_texture, v_UV);
    })glsl"
};

#endif
