#ifndef WINDOW_H
#define WINDOW_H

#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkgl.h>
#include <gst/gst.h>

int  window_new(GMainLoop *loop, const char *file);

#endif
