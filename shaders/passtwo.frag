#version 450

in vec2 vf_texcoord;
layout(location = 0) out vec4 color;

uniform sampler2D tex;

void main()
{
  color = texture(tex, vf_texcoord);
}
