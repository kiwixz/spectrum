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
#include <stdint.h>
#include <math.h>
#include <gtk/gtk.h>
#include "equalizer.h"
#include "config.h"
#include "player.h"
#include "shared.h"

static GtkWidget *window;

static void on_destroy(GtkWidget *widget, gpointer nul)
{
  window = NULL;
}

static gchar *format_db(GtkScale *slider, gdouble val, gpointer nul)
{
  return g_strdup_printf("%+d dB", (int)val);
}

static void on_value_change(GtkRange *slider, gpointer i)
{
  int *val;

  val = &config_get()->eqbands[(uintptr_t)i];
  *val = gtk_range_get_value(slider);
  gtk_range_set_fill_level(slider, *val);

  player_refresh_equalizer();
}

static void add_bandlabel(GtkWidget *table, int i, const char *text)
{
  GtkWidget *label;

  label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), text);
  gtk_table_attach(GTK_TABLE(table), label, i, i + 1, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);
}

void equalizer_show()
{
#define BANDLABEL(i, v) add_bandlabel(table, i, "<b>"v " Hz</b>")

  int       i;
  Config    *config;
  GtkWidget *table, *sliders[10];

  if (window)
    {
      gtk_window_present(GTK_WINDOW(window));
      return;
    }

  // definitions
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  table = gtk_table_new(8, 10, TRUE);
  BANDLABEL(0, "29");
  BANDLABEL(1, "59");
  BANDLABEL(2, "119");
  BANDLABEL(3, "237");
  BANDLABEL(4, "474");
  BANDLABEL(5, "947");
  BANDLABEL(6, "1889");
  BANDLABEL(7, "3770");
  BANDLABEL(8, "7523");
  BANDLABEL(9, "15011");

  config = config_get();
  for (i = 0; i < 10; ++i)
    {
      sliders[i] = gtk_vscale_new_with_range(-24, 12, 1);

      gtk_range_set_inverted(GTK_RANGE(sliders[i]), TRUE);
      gtk_range_set_show_fill_level(GTK_RANGE(sliders[i]), TRUE);
      gtk_range_set_restrict_to_fill_level(GTK_RANGE(sliders[i]), FALSE);
      gtk_range_set_value(GTK_RANGE(sliders[i]), config->eqbands[i]);

      g_signal_connect(sliders[i], "format-value", G_CALLBACK(format_db), NULL);
      g_signal_connect(sliders[i], "value-changed",
                       G_CALLBACK(on_value_change), NULL + i);
      gtk_table_attach(GTK_TABLE(table), sliders[i], i, i + 1, 1, 8,
                       GTK_FILL, GTK_FILL, 0, 0);
    }

  // window
  gtk_window_set_title(GTK_WINDOW(window), "Equalizer");
  gtk_container_set_border_width(GTK_CONTAINER(window), 16);
  g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), NULL);

  // table
  gtk_table_set_col_spacings(GTK_TABLE(table), 8);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);

  gtk_container_add(GTK_CONTAINER(window), table);

  gtk_widget_show_all(window);

#undef BANDLABEL
}
