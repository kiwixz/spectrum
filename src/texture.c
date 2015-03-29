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
#include "texture.h"
#include "shared.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
  #define UCTOUI(c) \
  ((c)[3] << 24)    \
  | ((c)[2] << 16)  \
  | ((c)[1] << 8)   \
  | ((c)[0])
#elif __BYTE_ORDER == __BIG_ENDIAN
  #define CTOUI(c) \
  ((c)[0] << 24)   \
  | ((c)[1] << 16) \
  | ((c)[2] << 8)  \
  | ((c)[3])
#else
  #error Could not determine endianness !
#endif

static GLuint texs[TEXTURES_LENGTH];

#if 0
  static int read_send_tex(const char *file)
  {
    unsigned int  w, h, size, mipmaps, len, offset, level;
    unsigned char header[128];
    GLchar        *buffer;
    FILE          *f;

    f = fopen(file, "rb");
    if (!f)
      {
        ERROR("Failed to open a texture.");
        return -1;
      }

    fread(header, sizeof(unsigned char), 128, f);

    h = UCTOUI(header + 12);
    w = UCTOUI(header + 16);
    size = UCTOUI(header + 20);
    mipmaps = UCTOUI(header + 28);

    len = size * 2;
    buffer = malloc(len * sizeof(GLchar));
    if (!buffer)
      {
        ERROR("Failed to malloc buffer.");
        return -1;
      }

    fread(buffer, sizeof(GLchar), len, f);
    fclose(f);

    offset = 0;
    for (level = 0; level < mipmaps && (w || h);
         ++level)
      {
        glCompressedTexImage2D(GL_TEXTURE_2D, level,
                               GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                               w, h, 0, size, buffer + offset);

        offset += size;
        w /= 2;
        h /= 2;
        size = ((w + 3) / 4) * ((h + 3) / 4) * 16;
      }

    free(buffer);
    return 0;
  }

#endif

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
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
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

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  if (read_send_tex(file))
    return -1;

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
