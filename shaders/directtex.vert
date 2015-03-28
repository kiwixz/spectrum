#version 450

in vec4  position;
in vec4  color;
in vec2  texcoord;
out vec4 vf_color;
out vec2 vf_texcoord;

uniform mat4 matrix;

void main()
{
  vf_color = color;
  vf_texcoord = texcoord;
  gl_Position = matrix * position;
}
