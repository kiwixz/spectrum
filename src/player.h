#ifndef PLAYER_H
#define PLAYER_H

#include <gst/gst.h>

int player_new(GMainLoop *loop, const char *file);
void player_delete();

void player_toggle();

const char *player_get_name();
void player_get_time(char *time, int maxlen);

#endif
