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
#include "textures.h"
#include "shared.h"

static GLuint texs[TEXTURES_LENGTH];

static int read_send_tex(const char *file) // read only ppm
{
#define GOAFTER(c)                              \
  for (++offset; buffer[offset] != c; ++offset) \
    {                                           \
    }                                           \
  ++offset;

  int    w, h, len, offset;
  GLchar *buffer;
  FILE   *fp;

  fp = fopen(file, "rb");
  if (!fp)
    {
      ERROR("Failed to open a texture: %s.", file);
      return -1;
    }

  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  rewind(fp);

  buffer = malloc(len * sizeof(GLchar));
  if (!buffer)
    {
      ERROR("Failed to malloc buffer for a texture: %s.", file);
      return -1;
    }

  fread(buffer, sizeof(GLchar), len, fp);
  fclose(fp);

  // read header
  if ((buffer[0] != 'P') || (buffer[1] != '6'))
    {
      ERROR("Failed to load a non-PPM file: %s.", file);
      return -1;
    }

  offset = 3;
  if (buffer[3] == '#') // skip comment
    GOAFTER('\n');

  w = atoi(buffer + offset);
  GOAFTER(' ');
  h = atoi(buffer + offset);
  GOAFTER('\n');

  if (atoi(buffer + offset) != 255)
    {
      ERROR("Failed to load a PPM without a max color of 255: %s.", file);
      return -1;
    }

  GOAFTER('\n');
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB,
               GL_UNSIGNED_BYTE, buffer + offset);

  free(buffer);
  return 0;
#undef GOAFTER
}

static int create_texture(int index, const char *file)
{
  GLuint tex;

  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);

  if (read_send_tex(file))
    return -1;

  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
  texs[index] = tex;

  return 0;
}

int texture_init()
{
  if (create_texture(TEX_FONT, "textures/font.ppm"))
    return -1;

  return 0;
}

void texture_delete()
{
  ERROR("texture_delete isn't implemented yet.");
}

void texture_bind(Texture tex)
{
  glBindTexture(GL_TEXTURE_2D, texs[tex]);
}

GLuint texture_get(Texture tex)
{
  return texs[tex];
}
