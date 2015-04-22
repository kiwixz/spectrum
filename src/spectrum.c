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

#define MAXDB -30

static const int KUP = 0.0333333f * FPS,
                 KDOWN = 0.1333333f * FPS;

static const float DBRATIO = (1.0f - BARSY) / (MAXDB - MINDB);

static float    averagemag, averagevel;
static Spectrum *spectrum;

int spectrum_new()
{
  spectrum = calloc(SPECBANDS, sizeof(Spectrum));
  if (!spectrum)
    {
      ERROR("Failed to calloc spectrum");
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
  float oldmag;

  oldmag = spectrum[band].mag;
  if (mag > oldmag)
    {
      spectrum[band].mag = ((KUP - 1) * oldmag + mag) / KUP;
    }
  else
    {
      spectrum[band].mag = ((KDOWN - 1) * oldmag + mag) / KDOWN;
    }

  spectrum[band].vel = (spectrum[band].mag - mag) / 2;
  if (spectrum[band].vel < 0)
    spectrum[band].vel = -spectrum[band].vel;
}

void spectrum_parse(const GstStructure *s)
{
  const GValue *magnitudes;
  int          band;

  magnitudes = gst_structure_get_value(s, "magnitude");
  averagemag = averagevel = 0.0f;

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

  averagemag /= SPECBANDS;
  averagevel /= SPECBANDS;
}

void spectrum_reset()
{
  int band;

  for (band = 0; band < SPECBANDS; ++band)
    spectrum[band].mag = spectrum[band].vel = 0.0f;

  averagemag = 0;
  averagevel = 0;
}

float spectrum_get_averagemag()
{
  return averagemag;
}

float spectrum_get_averagevel()
{
  return averagevel;
}

const Spectrum *spectrum_get()
{
  return spectrum;
}
