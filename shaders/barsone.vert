#version 450

in vec4  position;
in vec4  color;
out vec4 vf_position;
out vec4 vf_color;

uniform mat4 matrix;

void main()
{
  vf_position = position;
  vf_color = color;
  gl_Position = matrix * position;
}
