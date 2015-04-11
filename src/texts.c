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
#include "texts.h"
#include "player.h"
#include "render.h"
#include "shaders.h"
#include "shared.h"
#include "textures.h"

#define FONTW 32 // letters by dimension
#define FONTH 8
#define FPSY render_itofy(-16)
#define FPSW render_itofx(40)
#define FPSH render_itofy(16)
#define TIMEW 0.2f

static const float FONTLW = 1.0f / FONTW,
                   FONTLH = 1.0f / FONTH,
                   FPSX = 0.0f,
                   TIMEX = 0.5f - TIMEW / 2,
                   TIMEY = 0.05f,
                   TIMEH = 0.03f,
                   TITLEX = 0.2f,
                   TITLETOPY = 0.275f,
                   TITLEMAXW = 0.7f,
                   TITLEMAXH = 0.15f,
                   TITLEMINLWHRATIO = 0.1f,
                   TITLEMAXLWHRATIO = 1.0f;
static const GLushort VBOID[6] = {
  0, 1, 2, 2, 3, 0
};

static GLuint vbo, vboi;

int texts_new()
{
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &vboi);

  return 0;
}

void texts_delete()
{
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &vboi);
}

static void add_letter(int letter, int index,
                       GLfloat *pos, GLfloat *texcoord,
                       float x, float y, float z,
                       float xw, float yh, float zd)
{
#define POS(a, b) pos[kindex + a] = pos[kindex + b]
#define TEXCOORD(a, b) texcoord[kindex + a] = texcoord[kindex + b]

  int kindex;

  kindex = 4 * 3 * index;
  // x, y, z, xw, yh, zd
  POS(0, 3) = x;
  POS(4, 7) = y;
  POS(2, 5) = z;
  POS(6, 9) = xw;
  POS(1, 10) = yh;
  POS(8, 11) = zd;

  kindex = 4 * 2 * index;
  // x, y, xw, yh
  TEXCOORD(0, 2) = (float)(letter - letter / FONTW * FONTW) * FONTLW;
  TEXCOORD(3, 5) = (float)(FONTH - 1 - letter / FONTW) * FONTLH;
  TEXCOORD(4, 6) = texcoord[kindex] + FONTLW;
  TEXCOORD(1, 7) = texcoord[kindex + 3] + FONTLH;

#undef POS
#undef TEXCOORD
}

static int render_string(const char *text,
                         float x, float y, float z,
                         float w, float h, float d)
{
  int      i, isc, tlen;
  float    lx, lw;
  GLfloat  *vert, *verttxc;
  GLushort *id;

  tlen = 0;
  for (i = 0; text[i] != '\0'; ++i)
    if (text[i] != ' ')
      ++tlen;


  vert = malloc(4 * (3 + 2) * tlen * sizeof(GLfloat));
  id = malloc(6 * tlen * sizeof(GLushort));
  if (!vert || !id)
    {
      ERROR("Failed to malloc vertices of texts vbo for: '%s'", text);
      return -1;
    }
  verttxc = vert + 4 * 3 * tlen;

  lw = w / tlen;
  lx = x;

  isc = 0;
  for (i = 0; text[i + isc] != '\0'; ++i)
    {
      int j, offset, inc;

      if (text[i + isc] == ' ')
        {
          --i;
          ++isc;
          lx += lw;
          continue;
        }

      add_letter(text[i + isc], i, vert, verttxc,
                 lx, y, z,
                 lx + lw, y + h, z + d);
      lx += lw;

      offset = 6 * i;
      inc = 4 * i;
      for (j = 0; j < 6; ++j)
        id[offset + j] = VBOID[j] + inc;
    }

  textures_bind(TEX_FONT);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboi);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * tlen * sizeof(GLushort),
               id, GL_STREAM_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, 4 * (3 + 2) * tlen * sizeof(GLfloat),
               vert, GL_STREAM_DRAW);

  glEnableVertexAttribArray(POSITION_ATTRIB);
  glEnableVertexAttribArray(TEXCOORD_ATTRIB);
  glVertexAttribPointer(POSITION_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribPointer(TEXCOORD_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0,
                        (GLvoid *)(4 * 3 * tlen * sizeof(GLfloat)));

  glDrawElements(GL_TRIANGLES, 6 * tlen, GL_UNSIGNED_SHORT, 0);

  glDisableVertexAttribArray(POSITION_ATTRIB);
  glDisableVertexAttribArray(TEXCOORD_ATTRIB);

  free(vert);
  free(id);

  return 0;
}

int texts_render()
{
  static int  lastsec, fps;
  static char fpsstr[16];
  int         sec, len;
  float       w, h, ratio;
  char        timer[32] = {0};
  const char  *name;

  shaders_use(PROG_DIRECTTEX);

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
  name = player_get_name();
  if (name[0] != '\0')
    {
      len = strlen(name);
      ratio = TITLEMAXW / TITLEMAXH / len;
      if (ratio > TITLEMAXLWHRATIO) // w too large
        {
          w = TITLEMAXH * TITLEMAXLWHRATIO * len;
          h = TITLEMAXH;
        }
      else if (ratio < TITLEMINLWHRATIO) // h too large
        {
          w = TITLEMAXW;
          h = TITLEMAXW / TITLEMINLWHRATIO / len;
        }
      else
        {
          w = TITLEMAXW;
          h = TITLEMAXH;
        }

      glVertexAttrib4f(COLOR_ATTRIB, 1.0f, 1.0f, 1.0f, 1.0f);
      if (render_string(name, TITLEX, TITLETOPY - h, 0.0f, w, h, 0.0f))
        return -1;
    }

  // time
  player_get_time(timer, 32);
  if (timer[0] != '\0')
    if (render_string(timer, TIMEX, TIMEY, 0.0f,
                      TIMEW, TIMEH, 0.0f))
      return -1;


  return 0;
}
