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
#include <time.h>
#include <GL/glew.h>
#include <gdk/gdkkeysyms.h>
#include "window.h"
#include "buttons.h"
#include "open.h"
#include "player.h"
#include "render.h"
#include "shaders.h"
#include "shared.h"
#include "spectrum.h"
#include "textures.h"

static const int W = 1280,
                 H = 720;
static const unsigned int RESIZINGMS = 128;

static GtkWidget     *glarea;
static int           areaw, areah, clicking, motionblur;
static unsigned long lastresize;
static GMainLoop     *loop;

static unsigned long get_ms()
{
  struct timespec t;

  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000 + t.tv_nsec / 1000000L;
}

static gboolean on_press(GtkWidget *area, GdkEventButton *event, gpointer data)
{
  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  clicking = 1;

  if (buttons_click(event->x, areah - event->y))
    return TRUE;

  if (event->y > areah * (1.0f - TIMEBARH))
    {
      player_set_position(event->x / areaw);
      return TRUE;
    }

  return FALSE;
}

static gboolean on_release(GtkWidget *area, GdkEventButton *event,
                           gpointer data)
{
  clicking = 0;

  return FALSE;
}

static gboolean on_motion(GtkWidget *area, GdkEventButton *event, gpointer data)
{
  if (clicking && (event->y > areah * (1.0f - TIMEBARH)))
    {
      player_set_position(event->x / areaw);
      return TRUE;
    }

  return FALSE;
}

static void on_destroy(GtkWidget *win, gpointer data)
{
  player_delete();
  render_delete();
  shaders_delete();
  spectrum_delete();
  textures_delete();

  g_main_loop_quit(loop);
}

static gboolean on_configure(GtkWidget *area,
                             GdkEventConfigure *event, gpointer data)
{
  static int done;

  GdkGLDrawable *gldrawable;

  gldrawable = gtk_widget_get_gl_drawable(area);
  if (!gdk_gl_drawable_gl_begin(gldrawable, gtk_widget_get_gl_context(area)))
    {
      ERROR("Failed to initialize rendering");
      return FALSE;
    }

  // one-time init
  if (!done)
    {
      if (glewInit() != GLEW_OK)
        {
          ERROR("Failed to initialize graphics");
          return FALSE;
        }

      if (textures_init() || shaders_init())
        return FALSE;

      done = 1;
    }

  if (render_setup(area))
    return FALSE;

  gdk_gl_drawable_gl_end(gldrawable);

  areaw = glarea->allocation.width;
  areah = glarea->allocation.height;
  motionblur = 0;
  lastresize = get_ms();

  return TRUE;
}

int window_new(GMainLoop *mainloop)
{
  GtkWidget     *window, *area;
  GtkAccelGroup *accel;
  GdkGLConfig   *conf;

  // global variables
  loop = mainloop;

  if (spectrum_new())
    return -1;

  // definitions
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  accel = gtk_accel_group_new();
  glarea = area = gtk_drawing_area_new();

  // window
  gtk_window_set_title(GTK_WINDOW(window), "Spectrum");
  gtk_window_set_default_size(GTK_WINDOW(window), W, H);
  g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), NULL);

  // accel
  gtk_window_add_accel_group(GTK_WINDOW(window), accel);
  gtk_accel_group_connect(accel, GDK_KEY_space, 0, 0,
                          g_cclosure_new(player_toggle, 0, 0));
  gtk_accel_group_connect(accel, GDK_KEY_O, 0, 0,
                          g_cclosure_new(open_audio, 0, 0));

  // area
  gtk_container_add(GTK_CONTAINER(window), area);
  gtk_widget_set_events(area, GDK_EXPOSURE_MASK);
  conf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB | GDK_GL_MODE_DOUBLE);
  if (!conf)
    {
      ERROR("Could not find any double-buffered capable visual");

      // try single-buffered
      conf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB);
      if (!conf)
        {
          ERROR("Could not find any capable visual");
          return -1;
        }
    }
  if (!gtk_widget_set_gl_capability(area, conf, NULL, TRUE,
                                    GDK_GL_RGBA_TYPE))
    {
      ERROR("Failed to setup OpenGL capabilities");
      return -1;
    }
  gtk_widget_add_events(area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                        | GDK_POINTER_MOTION_MASK);
  g_signal_connect(area, "button-press-event", G_CALLBACK(on_press), NULL);
  g_signal_connect(area, "button-release-event", G_CALLBACK(on_release), NULL);
  g_signal_connect(area, "motion-notify-event", G_CALLBACK(on_motion), NULL);
  g_signal_connect(area, "configure-event", G_CALLBACK(on_configure), NULL);

  gtk_widget_show_all(window);

  if (player_new(loop))
    return -1;

  return 0;
}

int window_redraw()
{
  static unsigned long lasttick;

  unsigned long tick;
  GdkGLDrawable *gldrawable;

  tick = get_ms();
  if (tick - lastresize < RESIZINGMS)
    {
      if (tick - lasttick < 1000 / FPS)
        return 0;

      lasttick = tick;
    }

  gldrawable = gtk_widget_get_gl_drawable(glarea);
  if (!gdk_gl_drawable_gl_begin(gldrawable, gtk_widget_get_gl_context(glarea)))
    {
      ERROR("Failed to initialize rendering");
      return -1;
    }

  if (render(motionblur))
    return -1;

  if (gdk_gl_drawable_is_double_buffered(gldrawable))
    gdk_gl_drawable_swap_buffers(gldrawable);
  else
    glFlush();

  gdk_gl_drawable_gl_end(gldrawable);

  if (tick - lastresize >= RESIZINGMS)
    motionblur = 1;

#if 1 // should be done automatically in "idle" time ?
    while (g_main_context_pending(NULL))
      g_main_context_iteration(NULL, FALSE);
#endif

  return 0;
}
