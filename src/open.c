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
#include <gtk/gtk.h>
#include <regex.h>
#include <string.h>
#include "open.h"
#include "player.h"
#include "shared.h"

typedef struct
{
  GtkWidget *window, *textbox, *progressbar;
} UrlWidgets;

#define DOWNLOADER_MAXLEN 1024

static const char DOWNLOADER[] = "youtube-dl -wi --no-warnings --no-playlist -f bestaudio -o \"/tmp/%%(title)s.m4a\" \"%s\"",
                  DOWNLOADER_REGEX[] = "([0-9]+)\\.[0-9]+%";

static GtkWidget *window;

static void on_destroy()
{
  window = NULL;
}

static void open_file()
{
  GtkWidget *fc;

  fc = gtk_file_chooser_dialog_new("Open a file", GTK_WINDOW(window),
                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                   "Play", GTK_RESPONSE_ACCEPT,
                                   GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);

  if (gtk_dialog_run(GTK_DIALOG(fc)) == GTK_RESPONSE_ACCEPT)
    {
      char *file;

      file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
      player_play_file(file);
      g_free(file);
    }

  gtk_widget_destroy(fc);
  gtk_widget_destroy(window);
}

static void open_url_download(GtkWidget *widget, gpointer *data)
{
  regex_t    nregex, pregex;
  UrlWidgets *urlw;
  FILE       *fp;
  char       cmd[DOWNLOADER_MAXLEN],
             file[DOWNLOADER_MAXLEN] = {0};

  urlw = (UrlWidgets *)data;
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(urlw->progressbar),
                            "Initializing...");
  g_main_context_iteration(NULL, TRUE);

  if (regcomp(&nregex, "/tmp/.+\\.m4a", REG_EXTENDED)
      || regcomp(&pregex, DOWNLOADER_REGEX, REG_EXTENDED))
    {
      ERROR("Failed to compile a regex: '%s'", DOWNLOADER_REGEX);
      return;
    }

  snprintf(cmd, DOWNLOADER_MAXLEN, DOWNLOADER,
           gtk_entry_get_text(GTK_ENTRY(urlw->textbox)));

  fp = popen(cmd, "r");
  if (!fp)
    {
      ERROR("Failed to run '%s'", cmd);
      return;
    }

  while (1)
    {
      int        lasti;
      int        c;
      char       buffer[1024] = {0};
      regmatch_t matches[2];

      lasti = -1;
      do
        {
          ++lasti;
          buffer[lasti] = c = fgetc(fp);
        }
      while (c >= 0 && buffer[lasti] != '\n' && buffer[lasti] != '\r');

      if (c < 0)
        break;

      buffer[lasti] = '\0';

      if (!file[0] && !regexec(&nregex, buffer, 1, matches, 0))
        {
          int len;

          len = matches[0].rm_eo - matches[0].rm_so;
          strncpy(file, buffer + matches[0].rm_so, len);
          file[len] = '\0';
        }

      if (regexec(&pregex, buffer, 2, matches, 0))
        continue;

      buffer[matches[0].rm_eo] = '\0';
      gtk_progress_bar_set_text(GTK_PROGRESS_BAR(urlw->progressbar),
                                buffer + matches[0].rm_eo);
      buffer[matches[1].rm_eo] = '\0';
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(urlw->progressbar),
                                    atoi(buffer + matches[1].rm_so) / 100.0f);

      g_main_context_iteration(NULL, TRUE);
    }

  pclose(fp);

  gtk_widget_destroy(urlw->window);
  gtk_widget_destroy(window);
  free(urlw);

  player_play_file(file);
}

static void open_url()
{
  UrlWidgets *urlw;
  GtkWidget  *box, *label, *button;

  urlw = malloc(sizeof(UrlWidgets));
  if (!urlw)
    {
      ERROR("Failed to malloc the URL window");
      return;
    }

  // definitions
  urlw->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  box = gtk_vbox_new(TRUE, 4);
  label = gtk_label_new("Enter the URL of the video:");
  urlw->textbox = gtk_entry_new();
  button = gtk_button_new_with_label("Download");
  urlw->progressbar = gtk_progress_bar_new();

  // window
  gtk_window_set_title(GTK_WINDOW(urlw->window), "Open an URL");
  gtk_window_set_default_size(GTK_WINDOW(urlw->window), 512, 128);
  gtk_container_set_border_width(GTK_CONTAINER(urlw->window), 8);

  // button
  g_signal_connect(button, "clicked",
                   G_CALLBACK(open_url_download), urlw);

  gtk_container_add(GTK_CONTAINER(box),          label);
  gtk_container_add(GTK_CONTAINER(box),          urlw->textbox);
  gtk_container_add(GTK_CONTAINER(box),          button);
  gtk_container_add(GTK_CONTAINER(box),          urlw->progressbar);
  gtk_container_add(GTK_CONTAINER(urlw->window), box);

  gtk_widget_show_all(urlw->window);
}

void open_audio()
{
  GtkWidget *box, *buttonfile, *buttonurl;

  if (window)
    {
      gtk_window_present(GTK_WINDOW(window));
      return;
    }

  // definitions
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  box = gtk_vbox_new(TRUE, 4);
  buttonfile = gtk_button_new_with_label("Open a file");
  buttonurl = gtk_button_new_with_label("Open an URL");

  // window
  gtk_window_set_title(GTK_WINDOW(window), "Open");
  gtk_window_set_default_size(GTK_WINDOW(window), 256, 128);
  gtk_container_set_border_width(GTK_CONTAINER(window), 8);
  g_signal_connect(window,     "destroy", G_CALLBACK(on_destroy), NULL);

  // buttons
  g_signal_connect(buttonfile, "clicked",
                   G_CALLBACK(open_file), NULL);
  g_signal_connect(buttonurl, "clicked",
                   G_CALLBACK(open_url), NULL);

  gtk_container_add(GTK_CONTAINER(box),    buttonfile);
  gtk_container_add(GTK_CONTAINER(box),    buttonurl);
  gtk_container_add(GTK_CONTAINER(window), box);

  gtk_widget_show_all(window);
}
