#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <GL/glew.h>
#include <gdk/gdkkeysyms.h>
#include "window.h"
#include "player.h"
#include "render.h"
#include "shader.h"
#include "shared.h"
#include "spectrum.h"
#include "texture.h"

#define GETMS() 1000 * clock() / CLOCKS_PER_SEC

static const int W = 1280,
                 H = 720,
                 RESIZINGMS = 128;

static int       lastresize;
static GMainLoop *loop;

static void on_destroy()
{
  player_delete();
  render_delete();
  shader_delete();
  spectrum_delete();
  texture_delete();

  g_main_loop_quit(loop);
}

static gboolean on_expose(GtkWidget *area, GdkEventExpose *event,
                          gpointer data)
{
  GdkGLContext  *glcontext;
  GdkGLDrawable *gldrawable;

  glcontext = gtk_widget_get_gl_context(area);
  gldrawable = gtk_widget_get_gl_drawable(area);
  if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
    {
      ERROR("Failed to initialize rendering.");
      return FALSE;
    }

  if (render())
    return FALSE;

  if (gdk_gl_drawable_is_double_buffered(gldrawable))
    gdk_gl_drawable_swap_buffers(gldrawable);
  else
    glFlush();

  gdk_gl_drawable_gl_end(gldrawable);

  return TRUE;
}

static gboolean on_configure(GtkWidget *area, GdkEventConfigure *event,
                             gpointer data)
{
  static int done;

  GdkGLContext  *glcontext;
  GdkGLDrawable *gldrawable;

  glcontext = gtk_widget_get_gl_context(area);
  gldrawable = gtk_widget_get_gl_drawable(area);
  if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
    {
      ERROR("Failed to initialize rendering.");
      return FALSE;
    }

  // one-time init
  if (!done)
    {
      if (glewInit() != GLEW_OK)
        {
          ERROR("Failed to initialize graphics.");
          return FALSE;
        }

      if (texture_init() || shader_init())
        return FALSE;

      done = 1;
    }

  if (render_setup(area))
    return FALSE;

  gdk_gl_drawable_gl_end(gldrawable);

  lastresize = GETMS();

  return TRUE;
}

static gboolean on_tick(gpointer data)
{
  static int lasttick;

  int       tick;
  GtkWidget *area;

  tick = GETMS();
  if (tick - lastresize < RESIZINGMS)
    {
      if (tick - lasttick < RESIZINGMS)
        return TRUE;

      lasttick = tick;
    }

  area = GTK_WIDGET(data);
  gdk_window_invalidate_rect(area->window, &area->allocation, FALSE);
  gdk_window_process_updates(area->window, FALSE);

  return TRUE;
}

int window_new(GMainLoop *mainloop, const char *file)
{
  GtkWidget     *window, *area;
  GtkAccelGroup *accel;
  GClosure      *closure;
  GdkGLConfig   *conf;

  // global variables
  loop = mainloop;

  if (spectrum_new())
    return -1;

  // definitions
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  accel = gtk_accel_group_new();
  area = gtk_drawing_area_new();

  // window
  gtk_window_set_title(GTK_WINDOW(window), "spectrum");
  gtk_window_set_default_size(GTK_WINDOW(window), W, H);
  g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), NULL);

  // accel
  gtk_window_add_accel_group(GTK_WINDOW(window), accel);
  closure = g_cclosure_new(player_toggle, 0, 0);
  gtk_accel_group_connect(accel, GDK_KEY_space, 0, 0, closure);

  // area
  gtk_container_add(GTK_CONTAINER(window), area);
  gtk_widget_set_events(area, GDK_EXPOSURE_MASK);
  conf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB | GDK_GL_MODE_DOUBLE);
  if (!conf)
    {
      ERROR("Could not find any double-buffered capable visual.");

      // try single-buffered
      conf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGB);
      if (!conf)
        {
          ERROR("Could not find any capable visual.");
          return -1;
        }
    }
  if (!gtk_widget_set_gl_capability(area, conf, NULL, TRUE,
                                    GDK_GL_RGBA_TYPE))
    {
      ERROR("Failed to setup OpenGL capabilities.");
      return -1;
    }
  g_signal_connect(area, "configure-event", G_CALLBACK(on_configure), NULL);
  g_signal_connect(area, "expose-event",    G_CALLBACK(on_expose),    NULL);

  gtk_widget_show(window);
  gtk_widget_show_all(window);

  if (player_new(loop, file))
    return -1;

  // g_timeout_add(1000.0f / 10, on_tick, area);
  g_idle_add(on_tick, area);

  return 0;
}
