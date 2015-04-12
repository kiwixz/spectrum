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
#include "bars.h"
#include "shaders.h"
#include "shared.h"
#include "spectrum.h"

#define SPACESIDE 0.1f
#define SPACEBETWEEN 0.005f

static const float BARSW = (1.0f - 2 * SPACESIDE) / SPECBANDS - SPACEBETWEEN,
                   BARSMINH = 0.005f,
                   BARSD = 0.0f;
static const GLushort VBOID[6] = {
  0, 1, 2, 2, 3, 0
};

static GLfloat *vert;
static GLuint  vbo, vboi;

static void generate_quad(int index,
                          float x, float y, float z,
                          float w, float h, float d)
{
#define VERTICES(a, b) vert[index + a] = vert[index + b]

  VERTICES(0, 3) = x;
  VERTICES(4, 7) = y;
  VERTICES(2, 5) = z;
  VERTICES(6, 9) = x + w;
  VERTICES(1, 10) = y + h;
  VERTICES(8, 11) = z - d;

#undef VERTICES
}

static float generate_bar(int bar, float x) // return next bar's x
{
  int index;

  index = bar * 5 * 4 * 3;

  // front
  generate_quad(index, x, BARSY, 0.0f,
                BARSW, BARSMINH, 0.0f);
  index += 4 * 3;

  // down
  generate_quad(index, x, BARSY, 0.0f,
                BARSW, 0.0f, -BARSD);
  index += 4 * 3;

  // up
  generate_quad(index, x, BARSY + BARSMINH, 0.0f,
                BARSW, 0.0f, -BARSD);
  index += 4 * 3;

  // left
  generate_quad(index, x, BARSY, 0.0f,
                0.0f, BARSMINH, -BARSD);
  index += 4 * 3;

  // right
  generate_quad(index, x + BARSW, BARSY, 0.0f,
                0.0f, BARSMINH, -BARSD);

  return vert[index + 6] + SPACEBETWEEN; // because right face is the "last"
}

int bars_new()
{
  int      i, inc;
  float    x;
  GLushort id[5 * SPECBANDS * 6] = {0};

  glGenBuffers(1, &vbo);
  glGenBuffers(1, &vboi);
  vert = malloc(5 * SPECBANDS * 4 * 3 * sizeof(GLfloat));
  if (!vert)
    {
      fprintf(stderr, "Failed to malloc bars' vert (%lu bits)",
              5 * SPECBANDS * 4 * 3 * sizeof(GLfloat));
      return -1;
    }

  x = SPACESIDE;
  inc = 0;
  for (i = 0; i < SPECBANDS; ++i)
    {
      int j, k, offset;

      x = generate_bar(i, x);

      offset = 5 * 6 * i;
      for (j = 0; j < 5; ++j)
        {
          for (k = 0; k < 6; ++k)
            id[offset + k] = VBOID[k] + inc;

          inc += 4;
          offset += 6;
        }
    }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboi);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(id), id,
               GL_STATIC_DRAW);

  return 0;
}

void bars_delete()
{
  glDeleteBuffers(1, &vbo);
  free(vert);
}

static void set_barh(int bar, float h)
{
  int i;

  i = 5 * bar * 4 * 3;
  if (h < BARSMINH)
    h = BARSMINH;

  vert[i + 1] = vert[i + 10] // front
      = vert[i + 13] = vert[i + 16] = vert[i + 19] = vert[i + 22] // up
                = vert[i + 25] = vert[i + 34] // left
                      = vert[i + 49] = vert[i + 58] // right
                            = BARSY + h;
}

void bars_render()
{
  int            bar;
  float          average;
  const Spectrum *spectrum;

  spectrum = spectrum_get_and_lock();

  for (bar = 0; bar < SPECBANDS; ++bar)
    set_barh(bar, spectrum[bar].mag);

  spectrum_unlock();

  average = spectrum_get_averagemag();
  glVertexAttrib4f(COLOR_ATTRIB, average, 1.0f, 2 * average, 1.0f);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboi);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, 6 * SPECBANDS * 4 * 3 * sizeof(GLfloat),
               vert, GL_STREAM_DRAW);

  glEnableVertexAttribArray(POSITION_ATTRIB);
  glVertexAttribPointer(POSITION_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glDrawElements(GL_TRIANGLES, 5 * SPECBANDS * 6, GL_UNSIGNED_SHORT, 0);

  glDisableVertexAttribArray(POSITION_ATTRIB);
}
