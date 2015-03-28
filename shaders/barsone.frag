#version 450

in vec4 vf_position;
in vec4 vf_color;
layout(location = 0) out vec4 color;

void main()
{
  color = vf_color;
}
