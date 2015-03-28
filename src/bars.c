#include <stdlib.h>
#include <stdio.h>
#include "bars.h"
#include "shader.h"
#include "shared.h"
#include "spectrum.h"

#define VERTICES(a, b, c) \
  barsvert[index + a] = barsvert[index + b] = barsvert[index + c]

static const float barsminh = 0.005f,
                   barsd = 0.01f,
                   spacebetween = 0.009f,
                   spaceside = 0.1f;

static float   barsw;
static GLfloat *barsvert;

static void make_rectangle(int index,
                           float x, float y, float z,
                           float w, float h, float d)
{
  VERTICES(0,  3, 15) = x;
  VERTICES(6,  9, 12) = x + w;
  VERTICES(4,  7, 10) = y;
  VERTICES(1, 13, 16) = y + h;
  VERTICES(2,  5, 17) = z;
  VERTICES(8, 11, 14) = z - d;
}

static float make_bar(int bar, float x) // return next bar's x
{
  int index;

  index = bar * 6 * 18;

  // front
  make_rectangle(index, x, BARSY, 0.0f,
                 barsw, barsminh, 0.0f);
  index += 18;

  // back
  make_rectangle(index, x, BARSY, barsd,
                 barsw, barsminh, 0.0f);
  index += 18;

  // down
  make_rectangle(index, x, BARSY, 0.0f,
                 barsw, 0.0f, -barsd);
  index += 18;

  // up
  make_rectangle(index, x, BARSY + barsminh, 0.0f,
                 barsw, 0.0f, -barsd);
  index += 18;

  // left
  make_rectangle(index, x, BARSY, 0.0f,
                 0.0f, barsminh, -barsd);
  index += 18;

  // right
  make_rectangle(index, x + barsw, BARSY, 0.0f,
                 0.0f, barsminh, -barsd);

  return barsvert[index + 6] + spacebetween; // because right face is the last
}

int bars_new()
{
  int   bar;
  float x;

  if (!barsvert)
    {
      barsvert = malloc(SPECBANDS * 18 * 6 * sizeof(GLfloat));
      if (!barsvert)
        {
          fprintf(stderr, "Failed to malloc barsvert.");
          return -1;
        }
    }

  barsw = (1.0f - 2 * spaceside) / SPECBANDS - spacebetween;

  x = make_bar(0, spaceside);
  for (bar = 1; bar < SPECBANDS; ++bar)
    x = make_bar(bar, x);

  return 0;
}

void bars_delete()
{
  free(barsvert);
}

static void set_barh(int bar, float h)
{
  int index;

  index = bar * 6 * 18;
  if (h < barsminh)
    h = barsminh;

  VERTICES(1, 13, 16) // front
    = VERTICES(19, 31, 34) // back
        = VERTICES(73, 85, 88) // left
            = VERTICES(91, 103, 106) // right
                = VERTICES(58, 61, 64) // up
                    = VERTICES(55, 67, 70)
                        = BARSY + h;
}

static void send_color(const Spectrum *spectrum)
{
  int   bar;
  float aver;

  aver = 0.0f;
  for (bar = 0; bar < SPECBANDS; ++bar)
    aver += spectrum[bar].mag;

  aver /= SPECBANDS;
  glVertexAttrib4f(COLOR_ATTRIB, aver, 1.0f, 2 * aver, 1.0f);
}

void bars_render()
{
  int            bar;
  const Spectrum *spectrum;

  spectrum = spectrum_get_and_lock();

  for (bar = 0; bar < SPECBANDS; ++bar)
    set_barh(bar, spectrum[bar].mag);

  send_color(spectrum);
  spectrum_unlock();

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, barsvert);

  glDrawArrays(GL_TRIANGLES, 0, SPECBANDS * 6 * 6);

  glDisableClientState(GL_VERTEX_ARRAY);
}
