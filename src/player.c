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

static int        vol, muted;
static gint64     position, duration;
static char       *name;
static GstElement *pipeline, *source, *equalizer, *tee, *spec, *volume, *valve;
static GstBus     *bus;
static GstPad     *rqueuepad;

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
    }
}

static void on_pad_added(GstElement *decodebin, GstPad *pad, gpointer conv)
{
  GstPad *sinkpad;

  sinkpad = gst_element_get_static_pad((GstElement *)conv, "sink");
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
  GstElement *decodebin, *conv, *queue, *sink,*rqueue, *encoder, *fsink;
  GstPad         *queuepad;
  GstPadTemplate *padtemplate;

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
  decodebin = gst_element_factory_make("decodebin", NULL);
  conv = gst_element_factory_make("audioconvert", NULL);
  tee = gst_element_factory_make("tee", NULL);
  queue = gst_element_factory_make("queue2", NULL);
  equalizer = gst_element_factory_make("equalizer-10bands", NULL);
  spec = gst_element_factory_make("spectrum", NULL);
  volume = gst_element_factory_make("volume", NULL);
  sink = gst_element_factory_make("autoaudiosink", NULL);
  rqueue = gst_element_factory_make("queue2", NULL);
  valve = gst_element_factory_make("valve", NULL);
  encoder = gst_element_factory_make("wavenc", NULL);
  fsink = gst_element_factory_make("filesink", NULL);

  if (!pipeline || !source || !decodebin || !conv || !tee || !queue
      || !equalizer || !spec || !volume || !sink || !rqueue || !valve
      || !encoder || !fsink)
    {
      ERROR("Failed to create the audio pipeline");
      return -1;
    }

  // properties
  g_object_set(G_OBJECT(spec), "bands", (guint)SPECBANDS,
               "threshold", (gint)MINDB, NULL);
  g_object_set(G_OBJECT(valve), "drop", TRUE, NULL);
  g_object_set(G_OBJECT(fsink), "async", FALSE, "location", RECAUDIOFILE, NULL);

  configvol = config_get()->vol;
  if (configvol)
    vol = configvol;
  else
    vol = 100;

  g_object_set(G_OBJECT(volume), "volume", (gdouble)(vol / 100.0f), NULL);
  player_refresh_equalizer();

  // links
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  gst_bin_add_many(GST_BIN(pipeline), source, decodebin, conv, tee, queue,
                   equalizer, spec, volume, sink, rqueue, valve, encoder,
                   fsink, NULL);

  if (!gst_element_link(source, decodebin)
      || !gst_element_link(conv, tee)
      || !gst_element_link_many(queue, equalizer, spec, volume, sink, NULL)
      || !gst_element_link_many(rqueue, valve, encoder, fsink, NULL))
    {
      ERROR("Failed to link the audio pipeline");
      return -1;
    }

  // other links
  g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), conv);

  padtemplate =
    gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee), "src_%u");
  queuepad = gst_element_get_static_pad(queue, "sink");
  rqueuepad = gst_element_get_static_pad(rqueue, "sink");
  if ((gst_pad_link(gst_element_request_pad(tee, padtemplate, NULL, NULL),
                    queuepad) != GST_PAD_LINK_OK) ||
      (gst_pad_link(gst_element_request_pad(tee, padtemplate, NULL, NULL),
                    rqueuepad) != GST_PAD_LINK_OK))
    {
      ERROR("Failed to link tee");
      return -1;
    }
  gst_object_unref(queuepad);
  gst_object_unref(rqueuepad);

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

void player_set_fps(int fps)
{
  g_object_set(G_OBJECT(spec), "interval", (guint64)(1000000000LL / fps), NULL);
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
  if (!duration
      || !gst_element_seek_simple(pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH
                                  | GST_SEEK_FLAG_KEY_UNIT, frac * duration))
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
  g_object_set(G_OBJECT(volume), "volume", (gdouble)(v / 100.0f), NULL);
}

int player_get_volume()
{
  return vol;
}

void player_refresh_equalizer()
{
  int *bands;

  bands = config_get()->eqbands;
  g_object_set(G_OBJECT(equalizer), "band0", (gdouble)bands[0],
               "band1", (gdouble)bands[1], "band2", (gdouble)bands[2],
               "band3", (gdouble)bands[3], "band4", (gdouble)bands[4],
               "band5", (gdouble)bands[5], "band6", (gdouble)bands[6],
               "band7", (gdouble)bands[7], "band8", (gdouble)bands[8],
               "band9", (gdouble)bands[9], NULL);
}

void player_record_start()
{
  g_object_set(G_OBJECT(valve), "drop", FALSE, NULL);
}

void player_record_stop()
{
  gst_element_send_event(valve, gst_event_new_eos());
  g_object_set(G_OBJECT(valve), "drop", TRUE, NULL);
}
