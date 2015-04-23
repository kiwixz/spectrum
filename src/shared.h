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

#ifndef SHARED_H
#define SHARED_H

// even in shaders: use #VAR
#define SSAA 2

#define BARSY 0.3f
#define MAXVOL 150
#define MINDB -80
#define SMOOTHING 1
#define SPECBANDS 64

#define TIMEBARH 0.03f
#define VOLBARXPX 16 + 3 * (32 + 8) + 16
#define VOLBARYPX 40
#define VOLBARXW 0.5f
#define VOLBARYHPX 40 + 16

#define WARNING(s, ...)                                    \
  fprintf(stderr, __FILE__ ":%d: \x1b[33;1m"s "\x1b[0m\n", \
          __LINE__, ## __VA_ARGS__)
#define ERROR(s, ...)                                      \
  fprintf(stderr, __FILE__ ":%d: \x1b[31;1m"s "\x1b[0m\n", \
          __LINE__, ## __VA_ARGS__)
#define GLERROR()                  \
  if (glGetError() != GL_NO_ERROR) \
    ERROR("OpenGL error: %s", gluErrorString(glGetError()))

#endif
