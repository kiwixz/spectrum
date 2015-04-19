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

static const int SIZE = 5,
                 ANIMLEN = 512;
static const float GRAVITY = 1.002f,
                   GOBACK = 0.01f;

static int      start, end;
static GLuint   vbo;
static Particle parts[NUMBER];
static GLfloat  vert[NUMBER * 4];

static float randf(float min, float max)
{
  return (float)rand() / RAND_MAX * (max - min) + min;
}

static float randrf(float min, float max)
{
  float f;

  f = ((float)rand() / RAND_MAX * 2 - 1) * (max - min);
  if (f > 0)
    return f + min;
  else
    return f - min;
}

static void respawn_particle(int index, int vindex)
{
  parts[index].movx = randrf(0.001f, 0.005f);
  parts[index].movy = randf(-0.002f, -0.005f);

  vert[vindex] = randf(-0.1f, 1.1f);
  vert[vindex + 1] = randf(1.0f, 1.1f);

  parts[index].opacity = randf(0.0f, 1.0f);
  vert[3 * NUMBER + index] = parts[index].opacity * end / ANIMLEN;
}

int particles_new()
{
  int i;

  glGenBuffers(1, &vbo);
  glPointSize(SIZE);

  end = ANIMLEN;
  for (i = 0; i < NUMBER; ++i)
    {
      int iv;

      iv = i * 3;
      respawn_particle(i, iv);

      vert[iv + 1] = randf(0.0f, 1.1f);
      vert[3 * NUMBER + i] = 0.0f;
    }
  end = 0;

  return 0;
}

void particles_delete()
{
  glDeleteBuffers(1, &vbo);
}

void particles_render()
{
  int i;

  if (end == -1)
    return;
  else if (start)
    {
      for (i = 0; i < NUMBER; ++i)
        vert[3 * NUMBER + i] += parts[i].opacity / ANIMLEN;

      --start;
      ++end;
    }
  else if (end < ANIMLEN)
    {
      for (i = 0; i < NUMBER; ++i)
        vert[3 * NUMBER + i] -= parts[i].opacity / ANIMLEN;

      --end;
    }

  shaders_use(PROG_PARTICLES);

  for (i = 0; i < NUMBER; ++i)
    {
      int   iv;
      float old, k;

      iv = i * 3;
      k = spectrum_get_averagevel() + spectrum_get_averagemag() - GOBACK;

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

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, (3 + 1) * NUMBER * sizeof(GLfloat),
               vert, GL_STREAM_DRAW);

  glEnableVertexAttribArray(POSITION_ATTRIB);
  glEnableVertexAttribArray(COLOR_ATTRIB);
  glVertexAttribPointer(POSITION_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribPointer(COLOR_ATTRIB, 1, GL_FLOAT, GL_FALSE,
                        0, (GLvoid *)(3 * NUMBER * sizeof(GLfloat)));

  glDrawArrays(GL_POINTS, 0, NUMBER);

  glDisableVertexAttribArray(POSITION_ATTRIB);
  glDisableVertexAttribArray(COLOR_ATTRIB);
}

void particles_start()
{
  start = ANIMLEN - end;

  if (end == -1)
    end = 0;
}

void particles_end()
{
  start = 0;

  if (end > 0)
    --end;
}
