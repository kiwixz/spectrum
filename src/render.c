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
#include <time.h>
#include <GL/glew.h>
#include "render.h"
#include "bars.h"
#include "player.h"
#include "shader.h"
#include "shared.h"
#include "spectrum.h"
#include "texture.h"

#define BACKGROUND 0.1f, 0.1f, 0.1f
#define FONTW 32
#define FONTH 8
#define FPSH 0.025f
#define TITLEY 0.1f
#define TIMEW 0.2f

static const int   ssaa = 2;
static const float FONTLW = 1.0f / FONTW,
                   FONTLH = 1.0f / FONTH,
                   FPSX = 0.001f,
                   FPSY = 1.0f - FPSH,
                   FPSW = 0.05f,
                   TITLEX = 0.2f,
                   TITLEW = 0.7f,
                   TITLEH = BARSY - TITLEY,
                   TIMEX = 0.5f - TIMEW / 2,
                   TIMEY = 0.05f,
                   TIMEH = 0.03f;
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

static int generate_fbos(int w, int h)
{
  GLenum colat;

  if (fbos[0])
    delete_fbos();

  winw = w;
  winh = h;
  ssaaw = ssaa * w;
  ssaah = ssaa * h;

  glGenFramebuffers(2, fbos);
  glGenTextures(2, fbostex);
  glGenRenderbuffers(2, fbosrbuf);

  colat = GL_COLOR_ATTACHMENT0;

  // texture
  glBindTexture(GL_TEXTURE_2D, fbostex[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  // framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, colat,
                         GL_TEXTURE_2D, fbostex[0], 0);
  glDrawBuffers(1, &colat);

  // renderbuffer
  glBindRenderbuffer(GL_RENDERBUFFER, fbosrbuf[0]);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, fbosrbuf[0]);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      ERROR("Failed to setup fbo 1.");
      return -1;
    }

  // texture
  glBindTexture(GL_TEXTURE_2D, fbostex[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ssaaw, ssaah, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  // framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, colat,
                         GL_TEXTURE_2D, fbostex[1], 0);
  glDrawBuffers(1, &colat);

  // renderbuffer
  glBindRenderbuffer(GL_RENDERBUFFER, fbosrbuf[1]);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, ssaaw, ssaah);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, fbosrbuf[1]);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      ERROR("Failed to setup fbo 2.");
      return -1;
    }

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

  glLoadIdentity();
  shader_set_uniforms(matrix);

  if (bars_new()
      || generate_fbos(area->allocation.width, area->allocation.height))
    return -1;

  return 0;
}

void render_delete()
{
  delete_fbos();
  bars_delete();
}

static void add_letter(int letter, int index, GLfloat *pos, GLfloat *texcoord,
                       float x, float y, float z,
                       float xw, float yh, float zd)
{
#define POS(a, b, c) pos[kindex + a] = pos[kindex + b] = pos[kindex + c]
#define TEXCOORD(a, b, c) \
  texcoord[kindex + a] = texcoord[kindex + b] = texcoord[kindex + c]

  int kindex;

  kindex = 18 * index;
  // x, y, z, xw, yh, zd
  POS(0, 3,  15) = x;
  POS(4, 7,  10) = y;
  POS(2, 5,  17) = z;
  POS(6, 9,  12) = xw;
  POS(1, 13, 16) = yh;
  POS(8, 11, 14) = zd;

  kindex = 12 * index;
  // x, y, xw, yh
  TEXCOORD(0, 2, 10) = (float)(letter - letter / FONTW * FONTW) * FONTLW;
  TEXCOORD(3, 5, 7) = (float)(FONTH - 1 - letter / FONTW) * FONTLH;
  TEXCOORD(4, 6, 8) = texcoord[kindex] + FONTLW;
  TEXCOORD(1, 9, 11) = texcoord[kindex + 3] + FONTLH;

#undef POS
#undef TEXCOORD
}

static int render_string(const char *text,
                         float x, float y, float z,
                         float w, float h, float d)
{
  int     i, len;
  float   lx, lw;
  GLfloat *vert, *verttxc;
  GLuint  vbo;

  len = strlen(text);
  vert = malloc((18 + 12) * len * sizeof(GLfloat));
  if (!vert)
    {
      ERROR("Failed to malloc string's vertices.");
      return -1;
    }
  verttxc = vert + 18 * len;

  glActiveTexture(GL_TEXTURE0 + 1);
  texture_bind(TEX_FONT);
  glActiveTexture(GL_TEXTURE0);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  lw = w / len;
  lx = x;
  for (i = 0; i < len; ++i)
    {
      add_letter(text[i], i, vert, verttxc,
                 lx, y, z,
                 lx + lw, y + h, z + d);
      lx += lw;
    }

  glBufferData(GL_ARRAY_BUFFER, (18 + 12) * len * sizeof(GLfloat),
               vert, GL_STREAM_DRAW);
  render_vbo(3, 18 * len, 6 * len);

  glDeleteBuffers(1, &vbo);
  return 0;
}

static int render_text()
{
  static int  lastsec, fps;
  static char fpsstr[16];
  int         sec;
  char        timer[32] = {0};

  shader_use(PROG_TEXT);

  // fps
  sec = time(NULL);
  if (sec == lastsec)
    ++fps;
  else
    {
      snprintf(fpsstr, 16, "fps: %d", fps + 1);

      fps = 0;
      lastsec = sec;
    }

  glVertexAttrib4f(COLOR_ATTRIB, 0.8f, 0.8f, 0.8f, 1.0f);
  if (render_string(fpsstr, FPSX, FPSY, 0.0f, FPSW, FPSH, 0.0f))
    return -1;

  // title
  glVertexAttrib4f(COLOR_ATTRIB, 1.0f, 1.0f, 1.0f, 1.0f);
  if (render_string(player_get_name(), TITLEX, TITLEY, 0.0f,
                    (TITLEW < 1.0f - TITLEX) ? TITLEW : 1.0f, TITLEH, 0.0f))
    return -1;

  // time
  player_get_time(timer, 32);
  if (render_string(timer, TIMEX, TIMEY, 0.0f,
                    TIMEW, TIMEH, 0.0f))
    return -1;

  return 0;
}

static void render_passtwo()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, fbovbo);
  render_vbo(2, 12, 6);
}

static void render_bars()
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  shader_use(PROG_BARSONE);

  bars_render();

  glBindTexture(GL_TEXTURE_2D, fbostex[0]);
  shader_use(PROG_BARSTWO);
  render_passtwo();
}

static void render_ssaatwo()
{
  glBindTexture(GL_TEXTURE_2D, fbostex[1]);
  shader_use(PROG_PASS);
  render_passtwo();
}

int render()
{
  // glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(     GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glClearColor(BACKGROUND, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // ssaa
  glViewport(0, 0, ssaaw, ssaah);

  render_text();

  glViewport(0, 0, winw, winh);
  render_ssaatwo();

  render_bars();

  glDisable(     GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  texture_bind(TEX_NONE);
  // glDisable(GL_CULL_FACE);
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
