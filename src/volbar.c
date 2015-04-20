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
#include "volbar.h"
#include "player.h"
#include "render.h"
#include "shaders.h"
#include "shared.h"

static GLuint  vbo, vboi;
static GLubyte id[6] = {
  0, 1, 2, 2, 3, 0
};

int volbar_new()
{
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &vboi);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboi);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(id), id, GL_STATIC_DRAW);

  return 0;
}

void volbar_delete()
{
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &vboi);
}

void volbar_render()
{
  float   x, y, yh;
  GLfloat vert[4 * 2] = {
    x = render_itofx(VOLBARXPX), yh = render_itofy(VOLBARYHPX),
    x, y = render_itofy(VOLBARYPX),
    VOLBARXW, y,
    VOLBARXW, yh
  };

  shaders_use(PROG_DIRECT);
  glVertexAttrib4f(COLOR_ATTRIB, 0.5f, 0.1f, 0.1f, 0.2f);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboi);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vert), vert, GL_STREAM_DRAW);

  glEnableVertexAttribArray(POSITION_ATTRIB);
  glVertexAttribPointer(POSITION_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

  vert[4] = vert[6] = x + (VOLBARXW - x)
    * player_get_volume() / (float)MAXVOL;
  glBufferData(GL_ARRAY_BUFFER, sizeof(vert), vert, GL_STREAM_DRAW);
  glVertexAttrib4f(COLOR_ATTRIB, 0.5f, 0.1f, 0.1f, 0.9f);

  glVertexAttribPointer(POSITION_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(POSITION_ATTRIB);
}
