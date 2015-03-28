#include <stdlib.h>
#include <stdio.h>
#include "spectrum.h"
#include "shared.h"

static const float stepup = 0.1f,
                   stepdown = 0.05f;

static const float dbratio = (1.0f - BARSY) / (MAXDB - MINDB);

static Spectrum *spectrum;
static GMutex   specmutex;

int spectrum_new()
{
  spectrum = calloc(SPECBANDS, sizeof(Spectrum));

  if (!spectrum)
    {
      fprintf(stderr, "Failed to malloc spectrum.");
      return -1;
    }

  return 0;
}

void spectrum_delete()
{
  free(spectrum);
}

static void band_set(int band, float new)
{
  if (new < spectrum[band].mag)
    {
      float kstep;

      kstep = stepdown * (new + spectrum[band].mag) / 2;

      if (spectrum[band].mag - new <= kstep)
        spectrum[band].mag = new;
      else
        spectrum[band].mag -= kstep;
    }
  else // the new value is very rarely equal to the old value
    {
      float kstep;

      kstep = stepup * (new + spectrum[band].mag) / 2;

      if (new - spectrum[band].mag <= kstep)
        spectrum[band].mag = new;
      else
        spectrum[band].mag += kstep;
    }
}

void spectrum_parse(const GstStructure *s)
{
  const GValue *magnitudes;
  int          band;

  magnitudes = gst_structure_get_value(s, "magnitude");

  g_mutex_lock(&specmutex);
  for (band = 0; band < SPECBANDS; ++band)
    {
      const GValue *mag;

      mag = gst_value_list_get_value(magnitudes, band);

      if (mag)
        band_set(band, (g_value_get_float(mag) - MINDB) * dbratio);
    }

  for (band = SPECBANDS - 2; band > 0; --band)
    spectrum[band].mag =
      (spectrum[band - 1].mag * SMOOTHING + spectrum[band].mag * 2
       + spectrum[band + 1].mag * SMOOTHING) / (2 + 2 * SMOOTHING);

  g_mutex_unlock(&specmutex);
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
