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
#include "buttons.h"
#include "player.h"
#include "render.h"
#include "shaders.h"
#include "shared.h"
#include "textures.h"

#define BUTTONSLEN 2
typedef enum
{
  BUTTON_PLAY = 0,
  BUTTON_STOP = 1
} Button;
typedef struct
{
  Texture tex;
  int     x, y, xw, yh;
  float   rgb;
} ButtonInfo;

#define VERTICES(a, b, c) \
  vert[a] = vert[b] = vert[c]

static const float   ILLUMINATION_DECAY = 0.05f;
static const GLfloat TEXVERT[12] = {
  0.0f, 1.0f,
  0.0f, 0.0f,
  1.0f, 0.0f,
  1.0f, 0.0f,
  1.0f, 1.0f,
  0.0f, 1.0f
};

static ButtonInfo infos[BUTTONSLEN];
static GLuint     vbos[BUTTONSLEN];

static void generate_texquad(GLfloat vert[18 + 12],
                             float x, float y, float xw, float yh)
{
  VERTICES(0,  3, 15) = x;
  VERTICES(6,  9, 12) = xw;
  VERTICES(4,  7, 10) = y;
  VERTICES(1, 13, 16) = yh;

  memcpy(vert + 18, TEXVERT, sizeof(TEXVERT));
}

static void generate_button(Button b, Texture tex,
                            int x, int y, int xw, int yh)
{
#define COPY(v) infos[b].v = v;
  COPY(tex);
  COPY(  x);
  COPY(  y);
  COPY( xw);
  COPY( yh);
#undef COPY

  infos[b].rgb = 1.0f;
}

int buttons_new()
{
  glGenBuffers(BUTTONSLEN, vbos);

  generate_button(BUTTON_PLAY, TEX_PLAY, 16,          48,
                  16 + 32, 48 + 32);
  generate_button(BUTTON_STOP, TEX_STOP, 16 + 32 + 8, 48,
                  16 + 32 + 8 + 32, 48 + 32);

  return 0;
}

void buttons_delete()
{
  glDeleteBuffers(BUTTONSLEN, vbos);
}

void buttons_update()
{
  int b;

  for (b = 0; b < BUTTONSLEN; ++b)
    {
      GLfloat vert[18 + 12] = {0};

      generate_texquad(vert, render_itofx(infos[b].x), render_itofy(infos[b].y),
                       render_itofx(infos[b].xw), render_itofy(infos[b].yh));

      glBindBuffer(GL_ARRAY_BUFFER, vbos[b]);
      glBufferData(GL_ARRAY_BUFFER, (18 + 12) * sizeof(GLfloat),
                   vert, GL_STATIC_DRAW);
    }
}

void buttons_render()
{
  int b;

  shaders_use(PROG_DIRECTTEX);

  glEnableVertexAttribArray(POSITION_ATTRIB);
  glEnableVertexAttribArray(TEXCOORD_ATTRIB);

  for (b = 0; b < BUTTONSLEN; ++b)
    {
      if (infos[b].rgb > 1.0f)
        {
          glVertexAttrib4f(COLOR_ATTRIB,
                           infos[b].rgb, infos[b].rgb, infos[b].rgb, 1.0f);
          infos[b].rgb -= ILLUMINATION_DECAY;
        }
      else
        glVertexAttrib4f(COLOR_ATTRIB, 1.0f, 1.0f, 1.0f, 1.0f);

      glBindBuffer(GL_ARRAY_BUFFER, vbos[b]);
      glVertexAttribPointer(POSITION_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);
      glVertexAttribPointer(TEXCOORD_ATTRIB, 2, GL_FLOAT, GL_FALSE,
                            0, (GLvoid *)(18 * sizeof(GLfloat)));

      textures_bind(infos[b].tex);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

  glDisableVertexAttribArray(POSITION_ATTRIB);
  glDisableVertexAttribArray(TEXCOORD_ATTRIB);
}

void buttons_set_isplaying(int b)
{
  if (b)
    infos[BUTTON_PLAY].tex = TEX_PAUSE;
  else
    infos[BUTTON_PLAY].tex = TEX_PLAY;
}

void onclick(Button b)
{
  switch (b)
    {
      case BUTTON_PLAY:
        {
          player_toggle();
          break;
        }

      case BUTTON_STOP:
        {
          player_stop();
          break;
        }
    }
}

int buttons_click(int x, int y)
{
  int b;

  for (b = 0; b < BUTTONSLEN; ++b)
    if ((x > infos[b].x) && (x < infos[b].xw) // click on that button
        && (y > infos[b].y) && (y < infos[b].yh))
      {
        onclick(b);
        infos[b].rgb = 2.0f;

        return 1;
      }


  return 0;
}
