/*
 * Copyright (c) 2015 kiwixz
 *
 * This file is part of spectrum.
 *
 * spectrum is free software : you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * spectrum is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with spectrum. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "shaders.h"
#include "shared.h"
#include "textures.h"

static GLuint progs[PROGSLEN];

static char *read_file(const char *file)
{
  FILE *f;
  long len;
  char *buffer;

  f = fopen(file, "rb");
  if (!f)
    {
      ERROR("Failed to read shader %s", file);
      return NULL;
    }

  fseek(f, 0, SEEK_END);
  len = ftell(f);
  buffer = (char *)malloc(len + 1);
  fseek(f, 0, SEEK_SET);

  fread(buffer, len, 1, f);
  fclose(f);

  buffer[len] = '\0';

  return buffer;
}

static char *read_shader(const char *file)
{
  char *buffer, *pos;

  buffer = read_file(file);
  if (!buffer)
    return NULL;

  pos = strstr(buffer, "# SSAA");
  if (pos)
    pos[sprintf(pos, "%5d", SSAA)] = ' ';

  return buffer;
}

static GLuint compile_shader(const char *file, GLuint shader)
{
  GLint        done;
  const GLchar *source;

  source = read_shader(file);
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
          ERROR("Failed to malloc log");
          return 0;
        }

      glGetShaderInfoLog(shader, done, &done, log);
      ERROR("Failed to compile shader %s:\n%s", file, log);

      free(log);
      return 0;
    }

  return shader;
}

static int create_program(int index, const char *vertf, const char *fragf)
{
  GLint  done;
  GLuint vert, frag;

  glCreateShader(GL_FRAGMENT_SHADER);

  vert = compile_shader(vertf, glCreateShader(GL_VERTEX_SHADER));
  frag = compile_shader(fragf, glCreateShader(GL_FRAGMENT_SHADER));

  if (!vert || !frag)
    return -1;

  progs[index] = glCreateProgram();
  glAttachShader(progs[index], vert);
  glAttachShader(progs[index], frag);

  glBindAttribLocation(progs[index], POSITION_ATTRIB, "position");
  glBindAttribLocation(progs[index], COLOR_ATTRIB, "color");
  glBindAttribLocation(progs[index], TEXCOORD_ATTRIB, "texcoord");
  glBindAttribLocation(progs[index], PTSIZE_ATTRIB, "ptsize");

  glLinkProgram(progs[index]);
  glGetProgramiv(progs[index], GL_LINK_STATUS, &done);
  if (!done)
    {
      char *log;

      glGetProgramiv(progs[index], GL_INFO_LOG_LENGTH, &done);
      log = malloc(done);
      if (!log)
        {
          ERROR("Failed to malloc log");
          return -1;
        }

      glGetProgramInfoLog(progs[index], done, &done, log);
      ERROR("Failed to link program (%s + %s):\n%s", vertf, fragf, log);

      free(log);
      return -1;
    }

  glDetachShader(progs[index], vert);
  glDetachShader(progs[index], frag);
  glDeleteShader(vert);
  glDeleteShader(frag);

  return 0;
}

int shaders_init()
{
  if (create_program(PROG_BARS, "shaders/pass.vert", "shaders/bars.frag")
      || create_program(PROG_DIRECT,
                        "shaders/direct.vert", "shaders/direct.frag")
      || create_program(PROG_DIRECTPT,
                        "shaders/directpt.vert", "shaders/direct.frag")
      || (create_program(PROG_DIRECTTEX,
                         "shaders/directtex.vert", "shaders/directtex.frag"))
      || (create_program(PROG_PARTICLES,
                         "shaders/pass.vert", "shaders/particles.frag"))
      || (create_program(PROG_PASS, "shaders/pass.vert", "shaders/pass.frag")))
    return -1;

  return 0;
}

void shaders_delete()
{
  int i;

  shaders_use(PROG_NONE);
  for (i = 1; i < PROGSLEN; ++i)
    glDeleteProgram(progs[i]);
}

void shaders_use(Program prog)
{
  glUseProgram(progs[prog]);
}

void shaders_set_uniforms(GLfloat *matrix)
{
  shaders_use(PROG_BARS);
  shaders_set_texture(PROG_BARS, 0);

  shaders_use(PROG_DIRECT);
  shaders_send_matrix(PROG_DIRECT, matrix);

  shaders_use(PROG_DIRECTPT);
  shaders_send_matrix(PROG_DIRECTPT, matrix);

  shaders_use(PROG_DIRECTTEX);
  shaders_send_matrix(PROG_DIRECTTEX, matrix);
  shaders_set_texture(PROG_DIRECTTEX, 0);

  shaders_use(PROG_PARTICLES);
  shaders_set_texture(PROG_PARTICLES, 0);

  shaders_use(PROG_PASS);
  shaders_set_texture(PROG_PASS, 0);
}

void shaders_send_matrix(Program prog, GLfloat *matrix)
{
  glUniformMatrix4fv(glGetUniformLocation(progs[prog], "matrix"),
                     1, GL_FALSE, matrix);
}

void shaders_set_texture(Program prog, GLuint tex)
{
  glUniform1i(glGetUniformLocation(progs[prog], "tex"), tex);
}
