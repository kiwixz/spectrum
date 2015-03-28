#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <gst/gst.h>
#include "shader.h"
#include "shared.h"
#include "window.h"

#define OUT_ITALIC "\x1b[3m"

int main(int argc, char *argv[])
{
  GMainLoop *loop;

  if (argc < 2)
    {
      ERROR("Usage: %s " OUT_ITALIC "file", argv[0]);

      return EXIT_FAILURE;
    }

  gtk_init(&argc, &argv);
  gtk_gl_init(&argc, &argv);
  gst_init(&argc, &argv);

  loop = g_main_loop_new(NULL, FALSE);

  if (window_new(loop, argv[1]))
    return EXIT_FAILURE;

  g_main_loop_run(loop);

  g_main_loop_unref(loop);
  return EXIT_SUCCESS;
}
