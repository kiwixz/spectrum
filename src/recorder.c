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
#include <string.h>
#include <GL/glew.h>
#include <gtk/gtk.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include "recorder.h"
#include "buttons.h"
#include "player.h"
#include "shared.h"
#include "window.h"

#define CODEC 'X', 'V', 'I', 'D'
#define RECVIDEOFILE "/tmp/spectrum_rec_video.mkv"
#define AUDIOADDER                   \
  "xterm -e ffmpeg -i '"RECVIDEOFILE \
  "' -i '"RECAUDIOFILE "' -vcodec copy -shortest -y '%s' &"

static const int CMDLEN = 8192;

static int           width, height, row, size;
static CvVideoWriter *writer;
static IplImage      *img;
static char          *data, *file;

static int ask_file()
{
  GtkWidget *fc;

  fc = gtk_file_chooser_dialog_new("Save a video", NULL,
                                   GTK_FILE_CHOOSER_ACTION_SAVE,
                                   GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                   GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);

  if (gtk_dialog_run(GTK_DIALOG(fc)) != GTK_RESPONSE_ACCEPT)
    {
      gtk_widget_destroy(fc);
      window_set_resizable(1);

      return -1;
    }

  file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
  gtk_widget_destroy(fc);

  return 0;
}

int recorder_start()
{
  width = window_get_w();
  height = window_get_h();
  window_set_resizable(0);
  window_resize(width, height);
  row = width * 3;
  size = height * row;

  if (ask_file())
    return 1;

  writer = cvCreateVideoWriter(RECVIDEOFILE, CV_FOURCC(CODEC), RECFPS,
                               cvSize(width, height), 1);
  if (!writer)
    {
      ERROR("Failed to create video writer");
      return -1;
    }

  img = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 3);
  if (!img)
    {
      ERROR("Failed to create container for video frame");
      return -1;
    }

  data = malloc(size);
  if (!data)
    {
      ERROR("Failed to malloc pixels array for video frame");
      return -1;
    }

  buttons_set_isrecording(1);
  player_record_start();
  return 0;
}

void recorder_stop()
{
  char cmd[CMDLEN];

  player_record_stop();
  cvReleaseImageHeader(&img);
  cvReleaseVideoWriter(&writer);

  snprintf(cmd, CMDLEN, AUDIOADDER, file);
  system(cmd);
  g_free(file);

  free(data);
  data = NULL;
  buttons_set_isrecording(0);
  window_set_resizable(1);
}

int recorder_toggle()
{
  if (data)
    {
      recorder_stop();
      return 0;
    }
  else
    return recorder_start();
}

int recorder_frame()
{
  int  i;
  char *flipped, *last;

  if (!data)
    return 1;

  flipped = malloc(size);
  if (!flipped)
    {
      ERROR("Failed to malloc flipped image");
      return -1;
    }

  glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, flipped);
  last = flipped + size - row;

  // flip image vertically
  for (i = 0; i < size; i += row)
    memcpy(data + i, last - i, row);

  cvSetData(img, data, row);
  cvWriteFrame(writer, img);

  free(flipped);
  return 0;
}

int recorder_isrec()
{
  return data ? 1 : 0;
}
