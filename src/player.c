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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "shared.h"
#include "spectrum.h"
#include "window.h"

#ifdef _WIN32
  static const char PATHSEPARATOR = '\\';
#else
  static const char PATHSEPARATOR = '/';
#endif

static const int MSGPERSEC = 60;

static char       *name;
static GstElement *pipeline;
static guint      buswatch;

static gboolean on_message(GstBus *bus, GstMessage *msg, gpointer data)
{
  GMainLoop *loop;

  loop = (GMainLoop *)data;
  switch (GST_MESSAGE_TYPE(msg))
    {
      case GST_MESSAGE_ELEMENT:
        {
          const GstStructure *s;

          s = gst_message_get_structure(msg);

          if (strcmp(gst_structure_get_name(s), "spectrum") == 0)
            spectrum_parse(s);

          break;
        }

      case GST_MESSAGE_EOS:
        // End of stream
        break;

      case GST_MESSAGE_ERROR:
        {
          GError *err;

          gst_message_parse_error(msg, &err, NULL);
          ERROR("The player failed:\n%s", err->message);
          g_error_free(err);

          g_main_loop_quit(loop);
          break;
        }
    }

  return TRUE;
}

// add pad when file is demuxed
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
  GstPad *sinkpad;

  sinkpad = gst_element_get_static_pad((GstElement *)data, "sink");
  gst_pad_link(pad, sinkpad);
  gst_object_unref(sinkpad);
}

int player_new(GMainLoop *loop, const char *file)
{
  int        i, len;
  const char *from;
  GstElement *source, *demuxer, *decoder, *conv, *spec, *sink;
  GstBus     *bus;
  GstCaps    *caps;

  from = strrchr(file, PATHSEPARATOR);
  if (!from)
    from = file;

  len = strrchr(file, '.') - from;
  name = malloc((len + 1) * sizeof(char));
  if (!name)
    {
      ERROR("Failed to malloc the name.");
      return -1;
    }

  // memcpy(name, from, len);
  for (i = 0; i < len; ++i)
    {
      if ((*from >= 'a') && (*from <= 'z'))
        name[i] = *from + 'A' - 'a';
      else
        name[i] = *from;

      ++from;
    }
  name[len] = '\0';

  pipeline = gst_pipeline_new("audio-player");
  source = gst_element_factory_make("filesrc", "file-source");
  demuxer = gst_element_factory_make("qtdemux", "demuxer");
  decoder = gst_element_factory_make("faad", "decoder");
  conv = gst_element_factory_make("audioconvert", "converter");
  spec = gst_element_factory_make("spectrum", "spectrum");
  caps = gst_caps_new_simple("audio/x-raw", "rate",
                             G_TYPE_INT, AUDIOFREQ, NULL);
  sink = gst_element_factory_make("autoaudiosink", "audio-output");

  if (!pipeline || !source || !demuxer || !decoder
      || !conv || !spec || !caps || !sink)
    {
      ERROR("Failed to create audio pipeline.");
      return -1;
    }

  g_object_set(G_OBJECT(source), "location", file, NULL);
  g_object_set(G_OBJECT(spec),   "bands",    SPECBANDS,
               "interval", 1000000000L / MSGPERSEC, "threshold", MINDB, NULL);

  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  buswatch = gst_bus_add_watch(bus, on_message, loop);
  gst_object_unref(bus);

  gst_bin_add_many(GST_BIN(pipeline),
                   source, demuxer, decoder, conv, spec, sink, NULL);

  if (!gst_element_link(source, demuxer)
      || !gst_element_link(decoder, conv)
      || !gst_element_link_filtered(conv, spec, caps)
      || !gst_element_link(spec, sink))
    {
      ERROR("Failed to link audio pipeline.");
      return -1;
    }
  g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), decoder);

  return 0;
}

void player_delete()
{
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(GST_OBJECT(pipeline));
  g_source_remove(buswatch);

  free(name);
}

void player_toggle()
{
  GstState state;

  gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
  if (state == GST_STATE_PLAYING)
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
  else
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

const char *player_get_name()
{
  return name;
}

void player_get_time(char *time, int maxlen)
{
  int    posmin, possec, maxmin, maxsec;
  gint64 pos, max;

  if (!gst_element_query_position(pipeline, GST_FORMAT_TIME, &pos)
      || !gst_element_query_duration(pipeline, GST_FORMAT_TIME, &max))
    return;

  posmin = pos / 60000000000L;
  pos -= posmin * 60000000000L;
  possec = pos / 1000000000 - posmin;
  pos -= possec * 1000000000L;

  maxmin = max / 60000000000L;
  max -= maxmin * 60000000000L;
  maxsec = max / 1000000000 - maxmin;
  max -= maxsec * 1000000000L;

  snprintf(time, maxlen, "%d:%d.%d / %d:%d.%d",
           posmin, possec, (int)(pos / 10000000),
           maxmin, maxsec, (int)(max / 10000000));
}
