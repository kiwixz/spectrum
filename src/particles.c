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
#include "particles.h"
#include "render.h"
#include "shaders.h"
#include "shared.h"
#include "spectrum.h"

typedef struct
{
  float movx, movy, opacity;
} Particle;

#define OUTOFRANGE(f) f < -0.1f || f > 1.1f
#define OUTOFSCREEN(i) \
  OUTOFRANGE(vert[i]) || OUTOFRANGE(vert[i + 1]) || OUTOFRANGE(vert[i + 2])

#define NUMBER 2048

static const int   ANIMLEN = 8;
static const float GOBACK = 0.01f,
                   GRAVITY = 1.001f,
                   MOVXMIN = -0.3f,
                   MOVXMAX = 0.3f,
                   MOVYMIN = -0.15f,
                   MOVYMAX = -0.3f,
                   OPACITYMIN = 0.0f,
                   OPACITYMAX = 1.0f,
                   SIZEMIN = 1.0f,
                   SIZEMAX = 8.0f;

static float    opacityk;
static int      opacityinc;
static GLuint   vbo;
static Particle parts[NUMBER];
static GLfloat  vert[NUMBER * (3 + 4 + 1)];

static float randf(float min, float max)
{
  return (float)rand() / RAND_MAX * (max - min) + min;
}

#if 0 // could be useful later
  static float randrf(float min, float max)
  {
    float f;

    f = ((float)rand() / RAND_MAX * 2 - 1) * (max - min);
    if (f > 0)
      return f + min;
    else
      return f - min;
  }

#endif

static void respawn_particle(int index, int vindex)
{
  parts[index].movx = randf(MOVXMIN, MOVXMAX);
  parts[index].movy = randf(MOVYMIN, MOVYMAX);

  vert[vindex] = randf(-0.1f, 1.1f);
  vert[vindex + 1] = randf(1.0f, 1.1f);

  parts[index].opacity = randf(OPACITYMIN, OPACITYMAX);
  vert[3 * NUMBER + index * 4 + 3] = parts[index].opacity * opacityk;

  vert[(3 + 4) * NUMBER + index] = 1 / randf(1.0f / SIZEMAX, 1.0f / SIZEMIN);
}

int particles_new()
{
  int i;

  glGenBuffers(1, &vbo);

  opacityk = 1.0f;
  for (i = 0; i < NUMBER; ++i)
    {
      int iv;

      iv = i * 3;
      respawn_particle(i, iv);

      vert[iv + 1] = randf(0.0f, 1.1f);
      vert[3 * NUMBER + i * 4] = vert[3 * NUMBER + i * 4 + 1]
          = vert[3 * NUMBER + i * 4 + 2] = 1.0f;
      vert[3 * NUMBER + i * 4 + 3] = 0.0f;
    }
  opacityk = 0;

  return 0;
}

void particles_delete()
{
  glDeleteBuffers(1, &vbo);
}

void particles_render()
{
  int   i;
  float k;

  if (opacityk < 0.0f)
    return;
  else if (opacityinc)
    {
      opacityk += (float)opacityinc / ANIMLEN / render_get_fps();
      if (opacityk > 1.0f)
        {
          opacityk = 1.0f;
          opacityinc = 0;
        }
      else
        for (i = 0; i < NUMBER; ++i)
          vert[3 * NUMBER + i * 4 + 3] = parts[i].opacity * opacityk;

    }

  k = (spectrum_get_averagevel() + spectrum_get_averagemag() - GOBACK)
    / render_get_fps();
  for (i = 0; i < NUMBER; ++i)
    {
      int   iv;
      float old;

      iv = i * 3;
      old = parts[i].movx;
      parts[i].movx /= GRAVITY;
      if (old > 0)
        parts[i].movy -= old - parts[i].movx;
      else
        parts[i].movy -= parts[i].movx - old;

      vert[iv] += parts[i].movx * k;
      vert[iv + 1] += parts[i].movy * k;

      if (OUTOFSCREEN(iv))
        respawn_particle(i, iv);
    }

  shaders_use(PROG_DIRECTPT);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vert), vert, GL_STREAM_DRAW);

  glEnableVertexAttribArray(POSITION_ATTRIB);
  glEnableVertexAttribArray(COLOR_ATTRIB);
  glEnableVertexAttribArray(PTSIZE_ATTRIB);
  glVertexAttribPointer(POSITION_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribPointer(COLOR_ATTRIB, 4, GL_FLOAT, GL_FALSE, 0,
                        (GLvoid *)(3 * NUMBER * sizeof(GLfloat)));
  glVertexAttribPointer(PTSIZE_ATTRIB, 1, GL_FLOAT, GL_FALSE, 0,
                        (GLvoid *)((3 + 4) * NUMBER * sizeof(GLfloat)));

  glDrawArrays(GL_POINTS, 0, NUMBER);

  glDisableVertexAttribArray(POSITION_ATTRIB);
  glDisableVertexAttribArray(COLOR_ATTRIB);
  glDisableVertexAttribArray(PTSIZE_ATTRIB);
  glDisable(GL_PROGRAM_POINT_SIZE);
}

void particles_start()
{
  opacityinc = 1;
  if (opacityk < 0.0f)
    opacityk = 0.0f;
}

void particles_end()
{
  opacityinc = -1;
  if (opacityk < 0.0f)
    opacityk = 0.0f;
}
