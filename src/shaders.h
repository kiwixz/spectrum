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

#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>

#define POSITION_ATTRIB 0
#define COLOR_ATTRIB 1
#define TEXCOORD_ATTRIB 2

#define PROGRAMS_LENGTH 7
typedef enum
{
  PROG_NONE = 0,
  PROG_DIRECT = 1,
  PROG_PARTICLES = 2,
  PROG_DIRECTTEX = 3,
  PROG_TEXT = 4,
  PROG_PASS = 5,
  PROG_BARSTWO = 6
} Program;

int  shader_init();
void shader_delete();

void shader_use(Program prog);
void shader_set_uniforms(GLfloat *matrix);
void shader_send_matrix(Program prog, GLfloat *matrix);
void shader_set_texture(Program prog, GLuint tex);

#endif
