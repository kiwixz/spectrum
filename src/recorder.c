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
#include "shared.h"
#include "window.h"

#define VIDEOFPS 60
#define CODEC 'X', 'V', 'I', 'D'

static const int ftime = 1000000L / VIDEOFPS;

static int           width, height, row, size;
static CvVideoWriter *writer;
static IplImage      *img;
static char          *data;

static int create_writer()
{
  char      *file;
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
  writer = cvCreateVideoWriter(file, CV_FOURCC(CODEC), VIDEOFPS,
                               cvSize(width, height), 1);

  g_free(file);
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

  if (create_writer())
    return 1;

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
  return 0;
}

void recorder_stop()
{
  cvReleaseImageHeader(&img);
  cvReleaseVideoWriter(&writer);
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
  char *last, *flipped;

  if (!data)
    return 1;

  // won't work with SSAA
  // glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
  
  glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, data);
  last = data + size - row;

  // flip image vertically
  flipped = malloc(size);
  if (!flipped)
    {
      ERROR("Failed to malloc flipped image");
      return -1;
    }

  for (i = 0; i < size; i += row)
    memcpy(flipped + i, last - i, row);

  cvSetData(img, flipped, row);
  cvWriteFrame(writer, img);

  return 0;
}

int recorder_ftime()
{
  return data ? ftime : 0;
}
