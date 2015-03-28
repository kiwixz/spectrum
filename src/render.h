#ifndef RENDER_H
#define RENDER_H

#include <gtk/gtk.h>

int render_setup(GtkWidget *area);
void render_delete();

int render();
void render_vbo(int posdim, int texcoordoffset, int vertcount);

#endif
