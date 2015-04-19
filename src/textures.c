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
#include <tiffio.h>
#include "textures.h"
#include "shared.h"

#define TEXPATH(s) "textures/"s ".tif"

static GLuint texs[TEXTURESLEN];

static int read_send_tex(const char *file)
{
  TIFF   *tif;
  uint32 w, h, *raster;

  tif = TIFFOpen(file, "r");
  if (!tif)
    return -1;

  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
  raster = (uint32 *)_TIFFmalloc(w * h * sizeof(uint32));
  if (!raster)
    {
      ERROR("Failed to malloc the raster of %lu bytes for the texture %s",
            w * h * sizeof(uint32), file);
      return -1;
    }

  if (!TIFFReadRGBAImage(tif, w, h, raster, 0))
    {
      ERROR("Failed to read the texture %s", file);
      return -1;
    }

  TIFFClose(tif);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, raster);

  _TIFFfree(raster);
  return 0;
}

static int create_texture(int index, const char *file)
{
  glBindTexture(GL_TEXTURE_2D, texs[index]);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);

  if (read_send_tex(file))
    return -1;

  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);

  return 0;
}

int textures_init()
{
  glGenTextures(TEXTURESLEN - 1, texs + 1);

  if (create_texture(TEX_FONT, TEXPATH("font"))
      || create_texture(TEX_OPEN, TEXPATH("open"))
      || create_texture(TEX_PLAY, TEXPATH("play"))
      || create_texture(TEX_PAUSE, TEXPATH("pause"))
      || create_texture(TEX_STOP, TEXPATH("stop")))
    return -1;

  return 0;
}

void textures_delete()
{
  textures_bind(TEX_NONE);
  glDeleteTextures(TEXTURESLEN - 1, texs + 1);
}

void textures_bind(Texture tex)
{
  glBindTexture(GL_TEXTURE_2D, texs[tex]);
}

GLuint textures_get(Texture tex)
{
  return texs[tex];
}
