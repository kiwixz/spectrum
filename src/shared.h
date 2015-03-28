#ifndef SHARED_H
#define SHARED_H

#define AUDIOFREQ 44100
#define BARSY 0.3f
#define MAXDB -10
#define MINDB -80
#define SMOOTHING 1
#define SPECBANDS 64

#define ERROR(s, ...) fprintf(stderr, "\x1b[31;1m"s"\x1b[0m\n", ##__VA_ARGS__)

#endif
