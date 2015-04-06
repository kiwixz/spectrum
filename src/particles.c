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
  float movx, movy;
} Particle;

#define OUTOFRANGE(f) f < -0.1f || f > 1.1f
#define OUTOFSCREEN(i) \
  OUTOFRANGE(vert[i]) || OUTOFRANGE(vert[i + 1]) || OUTOFRANGE(vert[i + 2])

#define NUMBER 1024

static const int SIZE = 5;

static GLuint   vbo;
static Particle parts[NUMBER];
static GLfloat  vert[NUMBER * 4];

static float randf(float min, float max)
{
  return rand() * (max - min) / RAND_MAX + min;
}

static float randrf(float min, float max)
{
  return (rand() - RAND_MAX / 2) * (max - min) * 2 / RAND_MAX + min;
}

static void respawn_particle(int index, int vindex)
{
  parts[index].movx = randrf(0.001, 0.01);
  parts[index].movy = randf(-0.001, -0.01);

  vert[vindex] = randf(-0.1f, 1.1f);
  vert[vindex + 1] = randf(1.0f, 1.1f);

  vert[NUMBER * 3 + index] = randf(0.1f, 1.0f);
}

int particles_new()
{
  int i;

  glGenBuffers(1, &vbo);
  glPointSize(SIZE);

  for (i = 0; i < NUMBER; ++i)
    {
      int vi;

      vi = i * 3;
      respawn_particle(i, vi);
      vert[vi + 1] = randf(0.0f, 1.0f);
    }

  return 0;
}

void particles_delete()
{
  glDeleteBuffers(1, &vbo);
}

void particles_render()
{
  int i;

  shaders_use(PROG_PARTICLES);

  for (i = 0; i < NUMBER; ++i)
    {
      int   iv;
      float k;

      iv = i * 3;
      k = spectrum_get_average();

      vert[iv] += parts[i].movx * k;
      vert[iv + 1] += parts[i].movy * k;

      if (OUTOFSCREEN(iv))
        respawn_particle(i, iv);
    }

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, NUMBER * 4 * sizeof(GLfloat),
               vert, GL_STREAM_DRAW);

  glEnableVertexAttribArray(POSITION_ATTRIB);
  glEnableVertexAttribArray(COLOR_ATTRIB);
  glVertexAttribPointer(POSITION_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glVertexAttribPointer(COLOR_ATTRIB,    1, GL_FLOAT, GL_FALSE,
                        0, (GLvoid *)(NUMBER * 3 * sizeof(GLfloat)));

  glDrawArrays(GL_POINTS, 0, NUMBER);

  glDisableVertexAttribArray(POSITION_ATTRIB);
  glDisableVertexAttribArray(COLOR_ATTRIB);
}
