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
