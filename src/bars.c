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
#include "bars.h"
#include "shader.h"
#include "shared.h"
#include "spectrum.h"

#define VERTICES(a, b, c) \
  vert[index + a] = vert[index + b] = vert[index + c]

#define SPACESIDE 0.1f
#define SPACEBETWEEN 0.005f

static const float BARSW = (1.0f - 2 * SPACESIDE) / SPECBANDS - SPACEBETWEEN,
                   BARSMINH = 0.005f,
                   BARSD = 0.0f;

static GLfloat *vert;
static GLuint  vbo;

static void make_rectangle(int index,
                           float x, float y, float z,
                           float w, float h, float d)
{
  VERTICES(0,  3, 15) = x;
  VERTICES(6,  9, 12) = x + w;
  VERTICES(4,  7, 10) = y;
  VERTICES(1, 13, 16) = y + h;
  VERTICES(2,  5, 17) = z;
  VERTICES(8, 11, 14) = z - d;
}

static float make_bar(int bar, float x) // return next bar's x
{
  int index;

  index = bar * 6 * 18;

  // front
  make_rectangle(index, x, BARSY, 0.0f,
                 BARSW, BARSMINH, 0.0f);
  index += 18;

  // back
  make_rectangle(index, x, BARSY, BARSD,
                 BARSW, BARSMINH, 0.0f);
  index += 18;

  // down
  make_rectangle(index, x, BARSY, 0.0f,
                 BARSW, 0.0f, -BARSD);
  index += 18;

  // up
  make_rectangle(index, x, BARSY + BARSMINH, 0.0f,
                 BARSW, 0.0f, -BARSD);
  index += 18;

  // left
  make_rectangle(index, x, BARSY, 0.0f,
                 0.0f, BARSMINH, -BARSD);
  index += 18;

  // right
  make_rectangle(index, x + BARSW, BARSY, 0.0f,
                 0.0f, BARSMINH, -BARSD);

  return vert[index + 6] + SPACEBETWEEN; // because right face is the last
}

int bars_new()
{
  int   bar;
  float x;

  if (!vert)
    {
      vert = malloc(SPECBANDS * 18 * 6 * sizeof(GLfloat));
      if (!vert)
        {
          fprintf(stderr, "Failed to malloc vert.");
          return -1;
        }
    }

  x = make_bar(0, SPACESIDE);
  for (bar = 1; bar < SPECBANDS; ++bar)
    x = make_bar(bar, x);

  glGenBuffers(1, &vbo);
  return 0;
}

void bars_delete()
{
  glDeleteBuffers(1, &vbo);
  free(vert);
}

static void set_barh(int bar, float h)
{
  int index;

  index = bar * 6 * 18;
  if (h < BARSMINH)
    h = BARSMINH;

  VERTICES(1, 13, 16) // front
    = VERTICES(19, 31, 34) // back
        = VERTICES(73, 85, 88) // left
            = VERTICES(91, 103, 106) // right
                = VERTICES(58, 61, 64) // up
                    = VERTICES(55, 67, 70)
                        = BARSY + h;
}

static void send_color(const Spectrum *spectrum)
{
  int   bar;
  float aver;

  aver = 0.0f;
  for (bar = 0; bar < SPECBANDS; ++bar)
    aver += spectrum[bar].mag;

  aver /= SPECBANDS;
  glVertexAttrib4f(COLOR_ATTRIB, aver, 1.0f, 2 * aver, 1.0f);
}

void bars_render()
{
  int            bar;
  const Spectrum *spectrum;

  spectrum = spectrum_get_and_lock();

  for (bar = 0; bar < SPECBANDS; ++bar)
    set_barh(bar, spectrum[bar].mag);

  send_color(spectrum);
  spectrum_unlock();

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, SPECBANDS * 18 * 6 * sizeof(GLfloat),
               vert, GL_STREAM_DRAW);

  glEnableVertexAttribArray(POSITION_ATTRIB);
  glVertexAttribPointer(POSITION_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glDrawArrays(GL_TRIANGLES, 0, SPECBANDS * 6 * 6);

  glDisableVertexAttribArray(POSITION_ATTRIB);
}
