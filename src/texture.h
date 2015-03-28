#ifndef TEXTURE_H
#define TEXTURE_H

#include <GL/glew.h>

#define TEXTURES_LENGTH 2
typedef enum {
  TEX_NONE = 0,
  TEX_FONT = 1
} Texture;

int texture_init();
void texture_delete();

void texture_bind(Texture tex);
GLuint texture_get(Texture tex);

#endif
