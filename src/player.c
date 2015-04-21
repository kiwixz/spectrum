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
#include "buttons.h"
#include "config.h"
#include "particles.h"
#include "shared.h"
#include "spectrum.h"
#include "window.h"

#ifdef _WIN32
  static const char PATHSEPARATOR = '\\';
#else
  static const char PATHSEPARATOR = '/';
#endif
static const int audiofreq = 44100;

static int        vol, muted;
static float      bpm;
static gint64     position, duration;
static char       *name;
static GstElement *pipeline, *source, *volume;
static GstBus     *bus;

static void process_tag(const GstTagList *list, const gchar *tag, gpointer nul)
{
  const GValue *in;

  if (strcmp(tag, GST_TAG_BEATS_PER_MINUTE))
    return;

  in = gst_tag_list_get_value_index(list, tag, 0);
  if (!in)
    {
      ERROR("Failed to read BPM tag");
      return;
    }

  bpm = g_value_get_double(in);

  printf("bpm: %f\n", bpm);
}

static void process_message(GstMessage *msg)
{
  switch (GST_MESSAGE_TYPE(msg))
    {
      case GST_MESSAGE_DURATION_CHANGED:
      case GST_MESSAGE_STREAM_START:
        {
          if (!gst_element_query_duration(pipeline, GST_FORMAT_TIME, &duration))
            duration = 0;

          break;
        }

      case GST_MESSAGE_ELEMENT:
        {
          const GstStructure *s;

          s = gst_message_get_structure(msg);
          if (strcmp(gst_structure_get_name(s), "spectrum"))
            break;

          spectrum_parse(s);
          break;
        }

      case GST_MESSAGE_EOS:
        {
          player_stop();
          break;
        }

      case GST_MESSAGE_ERROR:
        {
          GError *err;

          gst_message_parse_error(msg, &err, NULL);
          ERROR("Failed to play: %s", err->message);
          g_error_free(err);

          break;
        }

      case GST_MESSAGE_TAG:
        {
          GstTagList *tags;

          gst_message_parse_tag(msg, &tags);
          if (GST_IS_TAG_LIST(tags))
            {
              gst_tag_list_foreach(tags, process_tag, NULL);
              gst_tag_list_free(tags);
            }

          break;
        }
    }
}

// add pad when file is demuxed
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
  GstPad *sinkpad;

  sinkpad = gst_element_get_static_pad((GstElement *)data, "sink");
  gst_pad_link(pad, sinkpad);
  gst_object_unref(sinkpad);
}

static int set_name(const char *file)
{
  int        i, len;
  const char *from;

  from = strrchr(file, PATHSEPARATOR);
  if (from)
    ++from;
  else
    from = file;

  len = strrchr(file, '.') - from;

  free(name);
  name = malloc(len + 1);
  if (!name)
    {
      ERROR("Failed to malloc the %d-long name", len + 1);
      return -1;
    }

  for (i = 0; i < len; ++i)
    {
      if ((*from >= 'a') && (*from <= 'z'))
        name[i] = *from + 'A' - 'a';
      else
        name[i] = *from;

      ++from;
    }
  name[len] = '\0';

  return 0;
}

int player_new(GMainLoop *loop)
{
  float      configvol;
  GstElement *demuxer, *decoder, *conv, *spec, *bpmdetector, *sink;

  name = malloc(sizeof(char));
  if (!name)
    {
      ERROR("Failed to malloc the empty name");
      return -1;
    }
  name[0] = '\0';

  // definitions
  pipeline = gst_pipeline_new("audio-player");
  source = gst_element_factory_make("filesrc", NULL);
  demuxer = gst_element_factory_make("qtdemux", NULL);
  decoder = gst_element_factory_make("faad", NULL);
  conv = gst_element_factory_make("audioconvert", NULL);
  spec = gst_element_factory_make("spectrum", NULL);
  bpmdetector = gst_element_factory_make("bpmdetect", NULL);
  volume = gst_element_factory_make("volume", NULL);
  sink = gst_element_factory_make("autoaudiosink", NULL);

  if (!pipeline || !source || !demuxer || !decoder
      || !conv || !bpmdetector || !spec || !sink)
    {
      ERROR("Failed to create the audio pipeline");
      return -1;
    }

  // properties
  g_object_set(G_OBJECT(spec), "bands", SPECBANDS,
               "interval", 1000000000L / FPS, "threshold", MINDB, NULL);

  configvol = config_get()->vol;
  if (configvol)
    vol = configvol;
  else
    vol = 100;

  g_object_set(G_OBJECT(volume), "volume", vol / 100.0f, NULL);

  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  gst_bin_add_many(GST_BIN(pipeline), source, demuxer, decoder,
                   conv, spec, bpmdetector, volume, sink, NULL);

  if (!gst_element_link(source, demuxer)
      || !gst_element_link_many(decoder, conv, spec, bpmdetector,
                                volume, sink, NULL))
    {
      ERROR("Failed to link the audio pipeline");
      return -1;
    }
  g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), decoder);

  return 0;
}

void player_delete()
{
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  config_get()->vol = vol;

  if (name)
    free(name);
}

void player_bus_pop()
{
  while (gst_bus_have_pending(bus))
    {
      GstMessage *msg;

      msg = gst_bus_pop(bus);
      process_message(msg);
      gst_message_unref(msg);
    }
}

void player_toggle()
{
  GstState state;

  gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
  if (state == GST_STATE_PLAYING)
    {
      gst_element_set_state(pipeline, GST_STATE_PAUSED);
      particles_end();
      buttons_set_isplaying(0);
    }
  else if (name[0])
    {
      gst_element_set_state(pipeline, GST_STATE_PLAYING);
      particles_start();
      buttons_set_isplaying(1);
    }
}

int player_play_file(const char *file)
{
  if (set_name(file))
    return -1;

  gst_element_set_state(pipeline, GST_STATE_READY);
  g_object_set(G_OBJECT(source), "location", file, NULL);
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  particles_start();
  buttons_set_isplaying(1);
  return 0;
}

int player_set_position(float frac)
{
  if (!duration || !gst_element_seek_simple(pipeline, GST_FORMAT_TIME,
                                            GST_SEEK_FLAG_FLUSH |
                                            GST_SEEK_FLAG_KEY_UNIT,
                                            frac * duration))
    return -1;

  return 0;
}

static void end_of_play()
{
  particles_end();
  buttons_set_isplaying(0);
  spectrum_reset();

  position = 0;
}

void player_stop()
{
  gst_element_set_state(pipeline, GST_STATE_READY);
  end_of_play();
}

const char *player_get_name()
{
  return name;
}

void player_get_time(char *time, int maxlen)
{
  int    pos, posmin, possec, max, maxmin, maxsec;
  gint64 gpos;

  if (!duration)
    {
      time[0] = '\0';
      return;
    }

  if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &gpos))
    position = gpos;
  else
    gpos = position;

  pos = gpos / 100000000L;
  posmin = pos / 600;
  pos -= posmin * 600;
  possec = pos / 10;

  max = duration / 100000000L;
  maxmin = max / 600;
  max -= maxmin * 600;
  maxsec = max / 10;

  snprintf(time, maxlen, "%02d:%02d.%d / %02d:%02d.%d",
           posmin, possec, pos - possec * 10,
           maxmin, maxsec, max - maxsec * 10);
}

float player_get_time_frac()
{
  gint64 pos;

  if (!duration)
    return 0.0f;

  if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &pos))
    position = pos;

  return (double)position / duration;
}

void player_toggle_mute()
{
  if (muted)
    {
      muted = 0;
      g_object_set(G_OBJECT(volume), "mute", FALSE, NULL);
    }
  else
    {
      muted = 1;
      g_object_set(G_OBJECT(volume), "mute", TRUE, NULL);
    }
  buttons_set_ismuted(muted);
}

void player_set_volume(int v)
{
  vol = v;
  g_object_set(G_OBJECT(volume), "volume", v / 100.0f, NULL);
}

int player_get_volume()
{
  return vol;
}
