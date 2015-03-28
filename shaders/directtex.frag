#version 450

in vec4  vf_color;
in vec2  vf_texcoord;
layout(location = 0) out vec4 color;

uniform sampler2D tex;

void main()
{
  color = texture(tex, vf_texcoord) * vf_color;
}
