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

#ifndef SHADERS_H
#define SHADERS_H

#include <GL/glew.h>

#define POSITION_ATTRIB 0
#define COLOR_ATTRIB 1
#define TEXCOORD_ATTRIB 2
#define PTSIZE_ATTRIB 3

#define PROGSLEN 7
typedef enum
{
  PROG_NONE = 0,
  PROG_BARS = 1,
  PROG_DIRECT = 2,
  PROG_DIRECTPT = 3,
  PROG_DIRECTTEX = 4,
  PROG_PARTICLES = 5,
  PROG_PASS = 6
} Program;

#define UNIFSLEN 3
typedef enum
{
  UNIF_MATRIX = 0,
  UNIF_OFFSET = 1,
  UNIF_TEX = 2
} Uniform;

int  shaders_init();
void shaders_delete();

void shaders_use(Program prog);
void shaders_set_const(GLfloat *matrix);
void shaders_set_texture(Program prog, GLuint tex);
GLint shaders_get_uniformid(Program prog, Uniform unif);

#endif
