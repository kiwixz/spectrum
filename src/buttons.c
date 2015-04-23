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
#include "open.h"
#include "player.h"
#include "render.h"
#include "shaders.h"
#include "shared.h"
#include "textures.h"
#include "window.h"

#define BUTTONSLEN 5
typedef enum
{
  BUTTON_FULLSCREEN = 0,
  BUTTON_MUTE = 1,
  BUTTON_OPEN = 2,
  BUTTON_PLAY = 3,
  BUTTON_STOP = 4
} Button;
typedef struct
{
  Texture tex;
  int     x, y, xw, yh;
  float   rgb;
} ButtonInfo;

static const int     ILLUMINATION_DECAY = 3;
static const GLubyte VBOID[6] = {
  0, 1, 2, 2, 3, 0
};
static const GLfloat TEXVERT[8] = {
  0.0f, 1.0f,
  0.0f, 0.0f,
  1.0f, 0.0f,
  1.0f, 1.0f
};

static ButtonInfo infos[BUTTONSLEN];
static GLuint     vbos[BUTTONSLEN], vboi, vbotex;

static void generate_quad(GLfloat *vert,
                          float x, float y, float xw, float yh)
{
#define VERTICES(a, b) vert[a] = vert[b]

  VERTICES(0, 3) = x;
  VERTICES(4, 7) = y;
  VERTICES(6, 9) = xw;
  VERTICES(1, 10) = yh;

#undef VERTICES
}

static void generate_button(Button b, Texture tex,
                            int x, int y, int xw, int yh)
{
#define COPY(v) infos[b].v = v;

  COPY(tex);
  COPY(x);
  COPY(y);
  COPY(xw);
  COPY(yh);

  infos[b].rgb = 1.0f;

#undef COPY
}

int buttons_new()
{
  glGenBuffers(BUTTONSLEN, vbos);
  glGenBuffers(BUTTONSLEN, &vboi);
  glGenBuffers(BUTTONSLEN, &vbotex);

  generate_button(BUTTON_FULLSCREEN, TEX_FULLSCREEN, 16, 3 * 40,
                  16 + 32, 3 * 40 + 32);
  generate_button(BUTTON_MUTE, TEX_MUTE, 16 + 2 * (32 + 8) + 16, 40,
                  16 + 2 * (32 + 8) + 16 + 32, 40 + 32);
  generate_button(BUTTON_OPEN, TEX_OPEN, 16, 2 * 40,
                  16 + 32, 2 * 40 + 32);
  generate_button(BUTTON_PLAY, TEX_PLAY, 16, 40,
                  16 + 32, 40 + 32);
  generate_button(BUTTON_STOP, TEX_STOP, 16 + 32 + 8, 40,
                  16 + 32 + 8 + 32, 40 + 32);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboi);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(VBOID), VBOID, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vbotex);
  glBufferData(GL_ARRAY_BUFFER, sizeof(TEXVERT), TEXVERT, GL_STATIC_DRAW);

  return 0;
}

void buttons_delete()
{
  glDeleteBuffers(BUTTONSLEN, vbos);
  glDeleteBuffers(BUTTONSLEN, &vboi);
  glDeleteBuffers(BUTTONSLEN, &vbotex);
}

void buttons_update()
{
  int b;

  for (b = 0; b < BUTTONSLEN; ++b)
    {
      GLfloat vert[4 * 3] = {0};

      generate_quad(vert, render_itofx(infos[b].x), render_itofy(infos[b].y),
                    render_itofx(infos[b].xw), render_itofy(infos[b].yh));

      glBindBuffer(GL_ARRAY_BUFFER, vbos[b]);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vert), vert, GL_STATIC_DRAW);
    }
}

void buttons_render()
{
  int b;

  shaders_use(PROG_DIRECTTEX);

  glEnableVertexAttribArray(POSITION_ATTRIB);
  glEnableVertexAttribArray(TEXCOORD_ATTRIB);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboi);
  glBindBuffer(GL_ARRAY_BUFFER, vbotex);
  glVertexAttribPointer(TEXCOORD_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, 0);

  for (b = 0; b < BUTTONSLEN; ++b)
    {
      textures_bind(infos[b].tex);
      if (infos[b].rgb > 1.0f)
        {
          glVertexAttrib4f(COLOR_ATTRIB,
                           infos[b].rgb, infos[b].rgb, infos[b].rgb, 1.0f);
          infos[b].rgb -= (float)ILLUMINATION_DECAY / render_get_fps();
        }
      else
        glVertexAttrib4f(COLOR_ATTRIB, 1.0f, 1.0f, 1.0f, 1.0f);

      glBindBuffer(GL_ARRAY_BUFFER, vbos[b]);
      glVertexAttribPointer(POSITION_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);

      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
    }

  glDisableVertexAttribArray(POSITION_ATTRIB);
  glDisableVertexAttribArray(TEXCOORD_ATTRIB);
}

void buttons_set_isplaying(int b)
{
  infos[BUTTON_PLAY].tex = b ? TEX_PAUSE : TEX_PLAY;
}

void buttons_set_ismuted(int b)
{
  infos[BUTTON_MUTE].tex = b ? TEX_UNMUTE : TEX_MUTE;
}

static void onclick(Button b)
{
  switch (b)
    {
      case BUTTON_FULLSCREEN:
        {
          window_set_fullscreen(!window_is_fullscreen());
          break;
        }

      case BUTTON_MUTE:
        {
          player_toggle_mute();
          break;
        }

      case BUTTON_OPEN:
        {
          open_audio();
          break;
        }

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
