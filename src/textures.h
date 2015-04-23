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

#ifndef TEXTURES_H
#define TEXTURES_H

#include <GL/glew.h>

#define TEXTURESLEN 10
typedef enum
{
  TEX_NONE = 0,
  TEX_EQUALIZER = 1,
  TEX_FONT = 2,
  TEX_FULLSCREEN = 3,
  TEX_MUTE = 4,
  TEX_OPEN = 5,
  TEX_PLAY = 6,
  TEX_PAUSE = 7,
  TEX_STOP = 8,
  TEX_UNMUTE = 9
} Texture;

int  textures_init();
void textures_delete();

void   textures_bind(Texture tex);
GLuint textures_get(Texture tex);

#endif
