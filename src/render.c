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
#include <GL/glew.h>
#include "render.h"
#include "bars.h"
#include "particles.h"
#include "shader.h"
#include "shared.h"
#include "spectrum.h"
#include "text.h"
#include "texture.h"

static const float MOTIONBLUR = 0.7f;
static const GLfloat FBOVBOVERT[2 * 12] = {
  -1.0f, 1.0f,
  -1.0f, -1.0f,
  1.0f, -1.0f,
  1.0f, -1.0f,
  1.0f, 1.0f,
  -1.0f, 1.0f,
  //
  0.0f, 1.0f,
  0.0f, 0.0f,
  1.0f, 0.0f,
  1.0f, 0.0f,
  1.0f, 1.0f,
  0.0f, 1.0f
};

static int    winw, winh, ssaaw, ssaah;
static GLuint fbovbo;
static GLuint fbos[2], fbostex[2], fbosrbuf[2];

static void delete_fbos()
{
  glDeleteFramebuffers(2, fbos);
  glDeleteTextures(2, fbostex);
  glDeleteRenderbuffers(2, fbosrbuf);
  glDeleteBuffers(1, &fbovbo);
}

static int generate_fbo(int i)
{
  GLenum colat;

  colat = GL_COLOR_ATTACHMENT0;

  // texture
  glBindTexture(GL_TEXTURE_2D, fbostex[i]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ssaaw, ssaah, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  // framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, fbos[i]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, colat,
                         GL_TEXTURE_2D, fbostex[i], 0);
  glDrawBuffers(1, &colat);

  // renderbuffer
  glBindRenderbuffer(GL_RENDERBUFFER, fbosrbuf[i]);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, ssaaw, ssaah);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, fbosrbuf[i]);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      ERROR("Failed to setup fbo n°%d", i);
      return -1;
    }

  return 0;
}

static int generate_fbos(int w, int h)
{
  if (fbos[0])
    delete_fbos();

  winw = w;
  winh = h;
  ssaaw = SSAA * w;
  ssaah = SSAA * h;

  glGenFramebuffers(2, fbos);
  glGenTextures(2, fbostex);
  glGenRenderbuffers(2, fbosrbuf);

  generate_fbo(0);
  generate_fbo(1);

  // vbo
  glGenBuffers(1, &fbovbo);
  glBindBuffer(GL_ARRAY_BUFFER, fbovbo);
  glBufferData(GL_ARRAY_BUFFER, 2 * 12 * sizeof(GLfloat), FBOVBOVERT,
               GL_STATIC_DRAW);
  return 0;
}

int render_setup(GtkWidget *area)
{
  float   ratio;
  GLfloat matrix[16];

  ratio = (float)area->allocation.width / area->allocation.height;

  // matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // angle = 2 * arctan(planwidth / (2 * cameradistance))
  gluPerspective(53.130102f, ratio, 0.1f, 10.0f);
  gluLookAt(0.5f, 0.5f, 1.0f,
            0.5f, 0.5f, 0.0f,
            0.0f, 1.0f, 0.0f);

  glTranslatef(0.5f - ratio / 2, 0.0f, 0.0f);
  glScalef(ratio, 1.0f, 1.0f);

  glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
  shader_set_uniforms(matrix);
  glLoadIdentity();

  if (bars_new() || particles_new() || text_new()
      || generate_fbos(area->allocation.width, area->allocation.height))
    return -1;

  glEnable(GL_CULL_FACE);
  glEnable(    GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  return 0;
}

void render_delete()
{
  delete_fbos();
  bars_delete();
  particles_delete();
}

static void render_passtwo()
{
  glBindTexture(GL_TEXTURE_2D, fbostex[0]);
  glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]);
  glBindBuffer(GL_ARRAY_BUFFER, fbovbo);
  render_vbo(2, 12, 6);
}

static void render_bars()
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  shader_use(PROG_DIRECT);

  bars_render();

  shader_use(PROG_BARSTWO);
  render_passtwo();
}

static int render_frame_fbo()
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // ssaa
  glViewport(0, 0, ssaaw, ssaah);
  glEnable(GL_DEPTH_TEST);

  particles_render();

  glDisable(GL_DEPTH_TEST);

  if (text_render())
    return -1;

  render_bars();

  glViewport(0, 0, winw, winh);

  return 0;
}

int render()
{
  if (render_frame_fbo())
    return -1;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, fbostex[1]);
  shader_use(PROG_PASS);
  glVertexAttrib4f(COLOR_ATTRIB, 1.0f, 1.0f, 1.0f, 1.0f - MOTIONBLUR);
  glBindBuffer(GL_ARRAY_BUFFER, fbovbo);
  render_vbo(2, 12, 6);

  return 0;
}

void render_vbo(int posdim, int texcoordoffset, int vertcount)
{
  glEnableVertexAttribArray(POSITION_ATTRIB);
  glEnableVertexAttribArray(TEXCOORD_ATTRIB);
  glVertexAttribPointer(POSITION_ATTRIB, posdim, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribPointer(TEXCOORD_ATTRIB, 2,      GL_FLOAT, GL_FALSE, 0,
                        (GLvoid *)(texcoordoffset * sizeof(GLfloat)));

  glDrawArrays(GL_TRIANGLES, 0, vertcount);

  glDisableVertexAttribArray(POSITION_ATTRIB);
  glDisableVertexAttribArray(TEXCOORD_ATTRIB);
}
