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
#include "recorder.h"
#include "render.h"
#include "shaders.h"
#include "shared.h"
#include "spectrum.h"
#include "textures.h"
#include "window.h"

#define FONTW 32 // letters by dimension
#define FONTH 8
#define TIMEY 0.05f
#define TIMEW 0.2f

#define TEXTSLEN 4
typedef enum
{
  TEXT_FPS = 0,
  TEXT_TIME = 1,
  TEXT_TITLE = 2,
  TEXT_VOLUME = 3
} Text;
typedef struct
{
  int      len;
  GLfloat  *vert;
  GLushort *id;
  GLenum   usage;
} TextInfo;

static const int FPSYPX = -16,
                 FPSWPX = 144,
                 FPSHPX = 16;
static const float FONTLW = 1.0f / FONTW,
                   FONTLH = 1.0f / FONTH,
                   FPSX = 0.0f,
                   TIMEX = 1.0f - TIMEY - TIMEW,
                   TIMEH = 0.03f,
                   TITLEX = 0.2f,
                   TITLETOPY = 0.275f,
                   TITLEMAXW = 0.7f,
                   TITLEMAXH = 0.15f,
                   TITLEMINLWHRATIO = 0.1f,
                   TITLEMAXLWHRATIO = 1.0f,
                   VOLW = 0.04f,
                   VOLH = 0.02f;
static const GLushort VBOID[6] = {
  0, 1, 2, 2, 3, 0
};

static GLuint   vbos[TEXTSLEN], vboids[TEXTSLEN];
static TextInfo texts[TEXTSLEN];

int texts_new()
{
  texts[TEXT_FPS].usage = GL_STREAM_DRAW;
  texts[TEXT_TIME].usage = GL_STREAM_DRAW;
  texts[TEXT_TITLE].usage = GL_STATIC_DRAW;
  texts[TEXT_VOLUME].usage = GL_STATIC_DRAW;

  glGenBuffers(TEXTSLEN, vbos);
  glGenBuffers(TEXTSLEN, vboids);

  return 0;
}

void texts_delete()
{
  glDeleteBuffers(TEXTSLEN, vbos);
  glDeleteBuffers(TEXTSLEN, vboids);
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

static int text_generate(Text t, const char *text,
                         float x, float y, float z,
                         float w, float h, float d)
{
  int     i, isc, len;
  float   lx, lw;
  GLfloat *verttxc;

  len = strlen(text);
  texts[t].len = 0;
  for (i = 0; i < len; ++i)
    if (text[i] != ' ')
      ++texts[t].len;


  texts[t].vert = malloc(4 * (3 + 2) * texts[t].len * sizeof(GLfloat));
  texts[t].id = malloc(6 * texts[t].len * sizeof(GLushort));
  if (!texts[t].vert || !texts[t].id)
    {
      ERROR("Failed to malloc vertices of texts vbo for: '%s'", text);
      return -1;
    }
  verttxc = texts[t].vert + 4 * 3 * texts[t].len;

  lw = w / len;
  lx = x;

  isc = 0;
  for (i = 0; i + isc < len; ++i)
    {
      int j, offset, inc;

      if (text[i + isc] == ' ')
        {
          --i;
          ++isc;
          lx += lw;
          continue;
        }

      add_letter(text[i + isc], i, texts[t].vert, verttxc,
                 lx, y, z,
                 lx + lw, y + h, z + d);
      lx += lw;

      offset = 6 * i;
      inc = 4 * i;
      for (j = 0; j < 6; ++j)
        texts[t].id[offset + j] = VBOID[j] + inc;
    }

  return 0;
}

static void text_bind(Text t)
{
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboids[t]);
  glBindBuffer(GL_ARRAY_BUFFER, vbos[t]);
}

static void text_bind_upload(Text t)
{
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboids[t]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * texts[t].len * sizeof(GLushort),
               texts[t].id, texts[t].usage);
  glBindBuffer(GL_ARRAY_BUFFER, vbos[t]);
  glBufferData(GL_ARRAY_BUFFER, 4 * (3 + 2) * texts[t].len * sizeof(GLfloat),
               texts[t].vert, texts[t].usage);
}

static void text_render(Text t)
{
  glEnableVertexAttribArray(POSITION_ATTRIB);
  glEnableVertexAttribArray(TEXCOORD_ATTRIB);
  glVertexAttribPointer(POSITION_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribPointer(TEXCOORD_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0,
                        (GLvoid *)(4 * 3 * texts[t].len * sizeof(GLfloat)));

  glDrawElements(GL_TRIANGLES, 6 * texts[t].len, GL_UNSIGNED_SHORT, 0);

  glDisableVertexAttribArray(POSITION_ATTRIB);
  glDisableVertexAttribArray(TEXCOORD_ATTRIB);
}

static void render_title()
{
  glVertexAttrib4f(COLOR_ATTRIB, 1.0f, 1.0f, 1.0f, 1.0f);
  text_bind(TEXT_TITLE);
  text_render(TEXT_TITLE);
}

static int render_time()
{
  char buffer[32];

  player_get_time(buffer, sizeof(buffer));
  if (buffer[0] == '\0')
    return 0;

  if (text_generate(TEXT_TIME, buffer, TIMEX, TIMEY, 0.0f,
                    TIMEW, TIMEH, 0.0f))
    return -1;

  glVertexAttrib4f(COLOR_ATTRIB, 1.0f, 1.0f, 1.0f, 1.0f);
  text_bind_upload(TEXT_TIME);
  text_render(TEXT_TIME);

  return 0;
}

static int render_fps()
{
  char buffer[32];

  snprintf(buffer, sizeof(buffer), "fps: %d (%ld \x95s)",
           window_get_fps(), window_get_ftime());
  if (text_generate(TEXT_FPS, buffer, FPSX, render_itofy(FPSYPX), 0.0f,
                    render_itofx(FPSWPX), render_itofy(FPSHPX), 0.0f))
    return -1;

  glVertexAttrib4f(COLOR_ATTRIB, 0.8f, 0.8f, 0.8f, 1.0f);
  text_bind_upload(TEXT_FPS);
  text_render(TEXT_FPS);

  return 0;
}

static void render_volume()
{
  glVertexAttrib4f(COLOR_ATTRIB, 1.0f, 1.0f, 1.0f, 1.0f);
  text_bind(TEXT_VOLUME);
  text_render(TEXT_VOLUME);
}

int texts_render()
{
  shaders_use(PROG_DIRECTTEX);
  textures_bind(TEX_FONT);

  render_title();
  if (render_time() || render_fps())
    return -1;

  if (recorder_isrec())
    return 0;

  render_volume();

  return 0;
}

int texts_refresh_title()
{
  int        len;
  float      w, h, ratio;
  const char *name;

  name = player_get_name();
  if (name[0] == '\0')
    return 0;

  len = strlen(name);
  ratio = TITLEMAXW / TITLEMAXH / len;
  if (ratio > TITLEMAXLWHRATIO) // if w too large
    {
      w = TITLEMAXH * TITLEMAXLWHRATIO * len;
      h = TITLEMAXH;
    }
  else if (ratio < TITLEMINLWHRATIO) // if h too large
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
  if (text_generate(TEXT_TITLE, name,
                    TITLEX, TITLETOPY - h, 0.0f, w, h, 0.0f))
    return -1;

  text_bind_upload(TEXT_TITLE);
  text_render(TEXT_TITLE);

  return 0;
}

int texts_refresh_volume()
{
  char buffer[8];

  snprintf(buffer, sizeof(buffer), "%d %%", player_get_volume());
  if (text_generate(TEXT_VOLUME, buffer, render_itofx(VOLBARXPX),
                    render_itofy(VOLBARYHPX), 0.0f,
                    VOLW, VOLH, 0.0f))
    return -1;

  text_bind_upload(TEXT_VOLUME);
  text_render(TEXT_VOLUME);

  return 0;
}
