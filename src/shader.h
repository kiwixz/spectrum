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
  PROG_DIRECTTEX = 2,
  PROG_TEXT = 3,
  PROG_BARSONE = 4,
  PROG_PASSTWO = 5,
  PROG_BARSTWO = 6
} Program;

int  shader_init();
void shader_delete();

void shader_use(Program prog);
void shader_set_uniforms(GLfloat *matrix);
void shader_send_matrix(Program prog, GLfloat *matrix);
void shader_set_texture(Program prog, GLuint tex);

#endif
