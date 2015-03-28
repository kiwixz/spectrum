#include <stdlib.h>
#include <stdio.h>
#include "shader.h"
#include "shared.h"
#include "texture.h"

static GLuint progs[PROGRAMS_LENGTH];

static const char *read_file(const char *file)
{
  FILE *f;
  long len;
  char *buf;

  f = fopen(file, "rb");
  if (!f)
    {
      ERROR("Failed to read shader %s.", file);
      return NULL;
    }

  fseek(f, 0, SEEK_END);
  len = ftell(f);
  buf = (char *)malloc(len + 1);
  fseek(f, 0, SEEK_SET);

  fread(buf, len, 1, f);
  fclose(f);

  buf[len] = '\0';

  return buf;
}

static GLuint compile_shader(const char *file, GLuint shader)
{
  GLint        done;
  const GLchar *source;

  source = read_file(file);
  if (!source)
    return 0;

  glShaderSource(shader, 1, &source, 0);
  glCompileShader(shader);

  glGetShaderiv(shader, GL_COMPILE_STATUS, &done);
  if (!done)
    {
      char *log;

      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &done);
      log = malloc(done);
      if (!log)
        {
          ERROR("Failed to malloc log.");
          return 0;
        }

      glGetShaderInfoLog(shader, done, &done, log);
      ERROR("Failed to link program.\n%s", log);

      free(log);
      return 0;
    }

  return shader;
}

static int create_program(int index, const char *vertf, const char *fragf)
{
  GLint  done;
  GLuint prog, vert, frag;

  glCreateShader(GL_FRAGMENT_SHADER);

  vert = compile_shader(vertf, glCreateShader(GL_VERTEX_SHADER));
  frag = compile_shader(fragf, glCreateShader(GL_FRAGMENT_SHADER));

  if (!vert || !frag)
    return -1;

  prog = glCreateProgram();
  glAttachShader(prog, vert);
  glAttachShader(prog, frag);

  glBindAttribLocation(prog, POSITION_ATTRIB, "position");
  glBindAttribLocation(prog, COLOR_ATTRIB,    "color");
  glBindAttribLocation(prog, TEXCOORD_ATTRIB, "texcoord");
  glLinkProgram(prog);

  glGetProgramiv(prog, GL_LINK_STATUS, &done);
  if (!done)
    {
      char *log;

      glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &done);
      log = malloc(done);
      if (!log)
        {
          ERROR("Failed to malloc log.");
          return -1;
        }

      glGetProgramInfoLog(prog, done, &done, log);
      ERROR("Failed to link program.\n%s", log);

      free(log);
      return -1;
    }

  progs[index] = prog;
  return 0;
}

int shader_init()
{
  if (create_program(PROG_DIRECT, "shaders/direct.vert", "shaders/direct.frag")
      || (create_program(PROG_DIRECTTEX,
                         "shaders/directtex.vert", "shaders/directtex.frag"))
      || (create_program(PROG_BARSONE,
                         "shaders/barsone.vert", "shaders/barsone.frag"))
      || (create_program(PROG_TEXT,
                         "shaders/text.vert", "shaders/text.frag"))
      || (create_program(PROG_PASSTWO,
                         "shaders/passtwo.vert", "shaders/passtwo.frag"))
      || (create_program(PROG_BARSTWO,
                         "shaders/passtwo.vert", "shaders/barstwo.frag")))
    return -1;

  return 0;
}

void shader_delete()
{
  ERROR("shader_delete isn't implemented yet.");
}

void shader_use(Program prog)
{
  glUseProgram(progs[prog]);
}

void shader_set_uniforms(GLfloat *matrix)
{
  shader_use(PROG_DIRECT);
  shader_send_matrix(PROG_DIRECT, matrix);

  shader_use(PROG_DIRECTTEX);
  shader_send_matrix(PROG_DIRECTTEX, matrix);
  shader_set_texture(PROG_DIRECTTEX, 0);

  shader_use(PROG_TEXT);
  shader_send_matrix(PROG_TEXT, matrix);
  shader_set_texture(PROG_TEXT, 1);

  shader_use(PROG_BARSONE);
  shader_send_matrix(PROG_BARSONE, matrix);
  shader_set_texture(PROG_BARSONE, 0);

  shader_use(PROG_PASSTWO);
  shader_set_texture(PROG_PASSTWO, 0);

  shader_use(PROG_BARSTWO);
  shader_set_texture(PROG_BARSTWO, 0);
}

void shader_send_matrix(Program prog, GLfloat *matrix)
{
  glUniformMatrix4fv(glGetUniformLocation(progs[prog], "matrix"),
                     1, GL_FALSE, matrix);
}

void shader_set_texture(Program prog, GLuint tex)
{
  glUniform1i(glGetUniformLocation(progs[prog], "tex"), tex);
}
