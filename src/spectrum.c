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
#include "spectrum.h"
#include "shared.h"

static const int KUP = 2,
                 KDOWN = 8;

static const float DBRATIO = (1.0f - BARSY) / (MAXDB - MINDB);

static float    averagemag, averagevel;
static Spectrum *spectrum;
static GMutex   specmutex;

int spectrum_new()
{
  spectrum = calloc(SPECBANDS, sizeof(Spectrum));

  if (!spectrum)
    {
      ERROR("Failed to malloc spectrum");
      return -1;
    }

  return 0;
}

void spectrum_delete()
{
  free(spectrum);
}

static void band_set(int band, float mag)
{
  if (mag > spectrum[band].mag)
    spectrum[band].mag = (mag + (KUP - 1) * spectrum[band].mag) / KUP;
  else
    spectrum[band].mag = (mag + (KDOWN - 1) * spectrum[band].mag) / KDOWN;

  spectrum[band].vel = (mag + spectrum[band].vel) / 2;
}

void spectrum_parse(const GstStructure *s)
{
  const GValue *magnitudes;
  int          band;

  magnitudes = gst_structure_get_value(s, "magnitude");
  averagemag = averagevel = 0.0f;

  g_mutex_lock(&specmutex);
  for (band = 0; band < SPECBANDS; ++band)
    {
      const GValue *mag;

      mag = gst_value_list_get_value(magnitudes, band);
      band_set(band, (g_value_get_float(mag) - MINDB) * DBRATIO);

      averagemag += spectrum[band].mag;
      averagevel += spectrum[band].vel;
    }

  for (band = SPECBANDS - 2; band > 0; --band)
    spectrum[band].mag =
      (spectrum[band - 1].mag * SMOOTHING + spectrum[band].mag * 2
       + spectrum[band + 1].mag * SMOOTHING) / (2 + 2 * SMOOTHING);

  g_mutex_unlock(&specmutex);

  averagemag /= SPECBANDS;
  averagevel /= SPECBANDS;
}

float spectrum_get_averagemag()
{
  return averagemag;
}

float spectrum_get_averagevel()
{
  return averagevel;
}

const Spectrum *spectrum_get_and_lock()
{
  g_mutex_lock(&specmutex);
  return spectrum;
}

void spectrum_unlock()
{
  g_mutex_unlock(&specmutex);
}
