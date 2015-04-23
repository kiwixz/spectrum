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

#ifndef PLAYER_H
#define PLAYER_H

#include <gst/gst.h>

int player_new(GMainLoop *loop);
void player_delete();

void player_bus_pop();
void player_set_fps(int fps);

void player_toggle();
int player_play_file(const char *file);
int player_set_position(float frac);
void player_stop();

const char *player_get_name();
void player_get_time(char *time, int maxlen);
float player_get_time_frac();

void player_toggle_mute();
void player_set_volume();
int player_get_volume();

#endif
