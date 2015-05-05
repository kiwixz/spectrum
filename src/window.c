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
#include <GL/glew.h>
#include <gdk/gdkkeysyms.h>
#include "window.h"
#include "buttons.h"
#include "config.h"
#include "player.h"
#include "render.h"
#include "shaders.h"
#include "shared.h"
#include "spectrum.h"
#include "textures.h"

static const int DOUBLEBUFFER = 0,
                 DEFAULTW = 1280,
                 DEFAULTH = 720;
static const float FPSSTABILITY = 0.9999f;

static GtkWidget *window, *area;
static int       fps, fullscreen, areaw, areah, clicking, motionblur;
static long      ftime;
static GMainLoop *loop;

static gboolean on_configure(GtkWidget *widget,
                             GdkEvent *event, gpointer nul)
{
  static int done;

  GdkGLDrawable *gldrawable;

  areaw = area->allocation.width;
  areah = area->allocation.height;
  motionblur = 0;

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

  if (render_setup())
    return FALSE;

  gdk_gl_drawable_gl_end(gldrawable);

  return TRUE;
}

static gboolean on_delete(GtkWidget *widget, GdkEvent *event, gpointer nul)
{
  Config *config;
  gint   w, h;

  gtk_window_get_size(GTK_WINDOW(window), &w, &h);
  config = config_get();
  config->winw = w;
  config->winh = h;

  return FALSE;
}

static void on_destroy(GtkWidget *widget, gpointer nul)
{
  player_delete();
  render_delete();
  shaders_delete();
  spectrum_delete();
  textures_delete();

  g_main_loop_quit(loop);
}

static int click_bars(int cx, int cy)
{
  float x, xw;

  if (cy < TIMEBARH * areah)
    {
      player_set_position((float)cx / areaw);
      return 1;
    }

  x = VOLBARXPX;
  xw = VOLBARXW * areaw;
  if ((cy < VOLBARYHPX) && (cy > VOLBARYPX) && (cx < xw) && (cx > x))
    {
      player_set_volume((int)(MAXVOL * (cx - x) / (xw - x)) + 1);
      return 1;
    }

  return 0;
}

static gboolean on_motion(GtkWidget *widget,
                          GdkEventButton *event, gpointer nul)
{
  if (clicking)
    {
      event->y = areah - event->y; // get same orientation as OpenGL

      if (click_bars(event->x, event->y))
        return TRUE;
    }

  return FALSE;
}

static gboolean on_press(GtkWidget *widget, GdkEventButton *event,
                         gpointer nul)
{
  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  clicking = 1;
  event->y = areah - event->y; // get same orientation as OpenGL

  if (buttons_click(event->x, event->y) || click_bars(event->x, event->y))
    return TRUE;

  return FALSE;
}

static gboolean on_release(GtkWidget *widget,
                           GdkEventButton *event, gpointer nul)
{
  clicking = 0;

  return FALSE;
}

static gboolean on_state_change(GtkWidget *widget,
                                GdkEventWindowState *event, gpointer nul)
{
  fullscreen = event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN;

  return FALSE;
}

static int check_fps()
{
  static gint64 lastus;

  long   diff;
  gint64 us;

  us = g_get_monotonic_time();
  diff = us - lastus;
  if ((diff < ftime * FPSSTABILITY) && ftime)
    {
      ftime *= FPSSTABILITY;
      return 0;
    }

  lastus = us;
  ftime = diff;
  fps = 1000000L / diff;
  if (!fps)
    fps = 1;

  return 1;
}

static gboolean redraw(gpointer nul)
{
  GdkGLDrawable *gldrawable;

  if (!check_fps())
    return TRUE;

  player_set_fps(fps);

  gldrawable = gtk_widget_get_gl_drawable(area);
  if (!gdk_gl_drawable_gl_begin(gldrawable, gtk_widget_get_gl_context(area)))
    {
      ERROR("Failed to initialize rendering");
      return FALSE;
    }

  if (render(motionblur))
    return FALSE;

  if (gdk_gl_drawable_is_double_buffered(gldrawable))
    gdk_gl_drawable_swap_buffers(gldrawable);
  else
    glFlush();

  gdk_gl_drawable_gl_end(gldrawable);

  motionblur = 1;
  return TRUE;
}

int window_new(GMainLoop *mainloop)
{
  Config        *config;
  GtkAccelGroup *accel;
  GdkGLConfig   *glconf;
  GdkGeometry   geom;

  geom.min_width = 640;
  geom.min_height = 480;

  loop = mainloop;
  config = config_get();

  if (spectrum_new())
    return -1;

  // definitions
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  accel = gtk_accel_group_new();
  area = gtk_drawing_area_new();

  // window
  gtk_window_set_title(GTK_WINDOW(window), "Spectrum");
  gtk_window_set_geometry_hints(GTK_WINDOW(window), window,
                                &geom, GDK_HINT_MIN_SIZE);
  if (config->winw && config->winh)
    gtk_window_set_default_size(GTK_WINDOW(window), config->winw, config->winh);
  else
    gtk_window_set_default_size(GTK_WINDOW(window), DEFAULTW, DEFAULTH);

  g_signal_connect(window, "window-state-event",
                   G_CALLBACK(on_state_change), NULL);
  g_signal_connect(window, "delete-event", G_CALLBACK(on_delete), NULL);
  g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), NULL);

  // accel
  gtk_window_add_accel_group(GTK_WINDOW(window), accel);
  gtk_accel_group_connect(accel, GDK_KEY_space, 0, 0,
                          g_cclosure_new(player_toggle, 0, 0));

  // area
  gtk_container_add(GTK_CONTAINER(window), area);

  if (DOUBLEBUFFER)
    {
      glconf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB | GDK_GL_MODE_DOUBLE);
      if (!glconf)
        ERROR("Could not find any double-buffered capable visual");
      // try single-buffered (next "if")
    }
  if (!DOUBLEBUFFER || !glconf)
    {
      glconf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB);
      if (!glconf)
        {
          ERROR("Could not find any capable visual");
          return -1;
        }
    }

  if (!gtk_widget_set_gl_capability(area, glconf, NULL, TRUE,
                                    GDK_GL_RGBA_TYPE))
    {
      ERROR("Failed to setup OpenGL capabilities");
      return -1;
    }
  gtk_widget_set_events(area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK
                        | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
  g_signal_connect(area, "button-press-event", G_CALLBACK(on_press), NULL);
  g_signal_connect(area, "button-release-event", G_CALLBACK(on_release), NULL);
  g_signal_connect(area, "motion-notify-event", G_CALLBACK(on_motion), NULL);
  g_signal_connect(area, "configure-event", G_CALLBACK(on_configure), NULL);

  gtk_widget_show_all(window);

  if (player_new(loop))
    return -1;

  g_idle_add(redraw, NULL);

  return 0;
}

int window_get_w()
{
  return areaw;
}

int window_get_h()
{
  return areah;
}

int window_get_fps()
{
  return fps;
}

long window_get_ftime()
{
  return ftime;
}

int window_is_fullscreen()
{
  return fullscreen;
}

void window_set_fullscreen(int b)
{
  if (b)
    gtk_window_fullscreen(GTK_WINDOW(window));
  else
    gtk_window_unfullscreen(GTK_WINDOW(window));
}

void window_set_resizable(int b)
{
  gtk_window_set_resizable(GTK_WINDOW(window), b ? TRUE : FALSE);
}
