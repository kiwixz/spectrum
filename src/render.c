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
#include "buttons.h"
#include "particles.h"
#include "player.h"
#include "shaders.h"
#include "shared.h"
#include "spectrum.h"
#include "texts.h"
#include "textures.h"
#include "timebar.h"
#include "volbar.h"

static const int     MOTIONBLEND = 40; // lower for more motionblur
static const float   FPSSTABILITY = 0.9f;
static const GLubyte FBOVBOID[6] = {
  0, 1, 2, 2, 3, 0
};
static const GLfloat FBOVBOVERT[2 * 2 * 4] = {
  -1.0f, 1.0f,
  -1.0f, -1.0f,
  1.0f, -1.0f,
  1.0f, 1.0f,
  //
  0.0f, 1.0f,
  0.0f, 0.0f,
  1.0f, 0.0f,
  1.0f, 1.0f
};

static int    fps, nfps, areaw, areah, ssaaw, ssaah;
static GLuint fbovbo, fbovboi;
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

static int generate_fbos()
{
  if (fbos[0])
    delete_fbos();

  glGenFramebuffers(2, fbos);
  glGenTextures(2, fbostex);
  glGenRenderbuffers(2, fbosrbuf);

  generate_fbo(0);
  generate_fbo(1);

  // vbo
  glGenBuffers(1, &fbovbo);
  glBindBuffer(GL_ARRAY_BUFFER, fbovbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(FBOVBOVERT),
               FBOVBOVERT, GL_STATIC_DRAW);
  glGenBuffers(1, &fbovboi);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fbovboi);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(FBOVBOID),
               FBOVBOID, GL_STATIC_DRAW);

  return 0;
}

int render_setup(GtkWidget *area)
{
  static int done;

  float   ratio;
  GLfloat matrix[16];

  areaw = area->allocation.width;
  areah = area->allocation.height;
  ssaaw = SSAA * areaw;
  ssaah = SSAA * areah;
  ratio = (float)areaw / areah;

  // matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // angle = 2 * arctan(planwidth / (2 * cameradistance))
  gluPerspective(53.130102f, ratio, 0.1f, 10.0f);
  gluLookAt(0.5f, 0.5f, 1.0f,
            0.5f, 0.5f, 0.0f,
            0.0f, 1.0f, 0.0f);

  // use 0;1 instead of -1;1
  glTranslatef(0.5f - ratio / 2, 0.0f, 0.0f);
  glScalef(ratio, 1.0f, 1.0f);

  glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
  shaders_set_const(matrix);
  glLoadIdentity();

  if (!done)
    {
      if (bars_new() || buttons_new() || particles_new()
          || texts_new() || timebar_new() || volbar_new())
        return -1;

      glEnable(GL_BLEND);
      glEnable(GL_CULL_FACE);
      glEnable(GL_POINT_SMOOTH);

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      done = 1;
    }

  if (generate_fbos())
    return -1;

  buttons_update();

  return 0;
}

void render_delete()
{
  delete_fbos();
  bars_delete();
  particles_delete();
}

static void render_static_vbo()
{
  glBindBuffer(GL_ARRAY_BUFFER, fbovbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fbovboi);

  glEnableVertexAttribArray(POSITION_ATTRIB);
  glEnableVertexAttribArray(TEXCOORD_ATTRIB);
  glVertexAttribPointer(POSITION_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribPointer(TEXCOORD_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0,
                        (GLvoid *)(sizeof(FBOVBOVERT) / 2));

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(POSITION_ATTRIB);
  glDisableVertexAttribArray(TEXCOORD_ATTRIB);
}

static void render_two_passes(void (*func)(), Program p)
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]);
  glClear(GL_COLOR_BUFFER_BIT);

  (*func)();

  shaders_use(p);
  glBindTexture(GL_TEXTURE_2D, fbostex[0]);
  glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]);
  glBindBuffer(GL_ARRAY_BUFFER, fbovbo);

  render_static_vbo();
}

static int render_frame_fbo()
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // ssaa
  glViewport(0, 0, ssaaw, ssaah);

  render_two_passes(&particles_render, PROG_PARTICLES);
  timebar_render();
  volbar_render();
  if (texts_render())
    return -1;

  render_two_passes(&bars_render, PROG_BARS);
  buttons_render();

  glViewport(0, 0, areaw, areah);

  return 0;
}

int render(int motionblur)
{
  static gint64 lastus;

  gint64 us;

  // fps
  us = g_get_monotonic_time();
  fps = 1000000L / (us - lastus);
  nfps = FPSSTABILITY * nfps + (1.0f - FPSSTABILITY) * fps;
  lastus = us;
  if (fps < 1)
    fps = 1;

  player_set_fps(fps);

  // render
  if (render_frame_fbo())
    return -1;

  shaders_use(PROG_PASS);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, fbostex[1]);
  glVertexAttrib4f(COLOR_ATTRIB, 1.0f, 1.0f, 1.0f,
                   motionblur ? (float)MOTIONBLEND / render_get_fps() : 1.0f);

  render_static_vbo();

  return 0;
}

float render_itofx(int n)
{
  if (n >= 0)
    return (float)n / areaw;
  else
    return 1 + (float)n / areaw;
}

float render_itofy(int n)
{
  if (n >= 0)
    return (float)n / areah;
  else
    return 1 + (float)n / areah;
}

int render_get_norm_fps()
{
  return nfps;
}

int render_get_fps()
{
  return fps;
}
