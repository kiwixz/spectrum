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
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include "recorder.h"
#include "shared.h"
#include "window.h"

static CvVideoWriter *writer;
static IplImage      *img;
static char          *data;

int recorder_start()
{
  CvSize area;

  window_set_resizable(0);
  area = cvSize(window_get_w(), window_get_h());

  writer = cvCreateVideoWriter("s.avi", 0, 10.0, area, 1);
  if (!writer)
    {
      ERROR("Failed to create video writer");
      return -1;
    }

  img = cvCreateImageHeader(area, IPL_DEPTH_8U, 3);
  if (!img)
    {
      ERROR("Failed to create container for video frame");
      return -1;
    }

  data = malloc(window_get_w() * window_get_h() * 3);
  if (!data)
    {
      ERROR("Failed to create pixels array for video frame");
      return -1;
    }

  return 0;
}

void recorder_stop()
{
  cvReleaseImageHeader(&img);
  cvReleaseVideoWriter(&writer);
  window_set_resizable(1);
}

void recorder_frame()
{
  glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
  img->imageData = data;
  cvWriteFrame(writer, img);
}
