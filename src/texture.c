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

static int create_texture(int index, const char *file)
{
  GLuint tex;

  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  if (read_send_tex(file))
    {
      printf("send tex err\n");
      return -1;
    }

  glBindTexture(GL_TEXTURE_2D, 0);
  texs[index] = tex;
  return 0;
}

int texture_init()
{
  if (create_texture(TEX_FONT, "textures/font.dds"))
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
