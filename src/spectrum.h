#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <gst/gst.h>

typedef struct {
  float mag;
} Spectrum;

int spectrum_new();
void spectrum_delete();

void spectrum_parse(const GstStructure *s);

const Spectrum *spectrum_get_and_lock();
void spectrum_unlock();

#endif
