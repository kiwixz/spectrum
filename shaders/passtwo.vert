#version 450

in vec2  position;
in vec2  texcoord;
out vec2 vf_texcoord;

void main()
{
  vf_texcoord = texcoord;
  gl_Position = vec4(position, 0.0f, 1.0f);
}
