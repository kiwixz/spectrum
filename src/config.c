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
#include <string.h>
#include "config.h"
#include "shared.h"

typedef enum
{
  TYPE_INT
} Type;
typedef struct
{
  const char *var;
  void       *val;
  Type       type;
} ConfigEntry;

#define ENTRIESLEN 3

#ifdef _WIN32
  static const char *FILEENVPREFIX = "APPDATA",
                    *FILESUFFIX = "\spectrum.conf";
#else
  static const char *FILEENVPREFIX = "HOME",
                    *FILESUFFIX = "/.config/spectrum.conf";
#endif
static const int BUFFERSIZE = 512;

static int         varmaxlen;
static ConfigEntry entries[ENTRIESLEN];
static char        configfile[1024];
static Config      config;

static void add_entry(const char *var, void *val, Type type)
{
  static int i;

  int len;

  len = strlen(var);
  if (varmaxlen < len)
    varmaxlen = len;

  entries[i].var = var;
  entries[i].val = val;
  entries[i].type = type;

  ++i;
}

void config_init()
{
  snprintf(configfile, sizeof(configfile), "%s%s",
           getenv(FILEENVPREFIX), FILESUFFIX);

  add_entry("volume", &config.vol, TYPE_INT);
  add_entry("window_width", &config.winw, TYPE_INT);
  add_entry("window_height", &config.winh, TYPE_INT);
}

static int process_line(const char var[], const char val[])
{
  int i;

  for (i = 0; i < ENTRIESLEN; ++i)
    if (!strcmp(var, entries[i].var))
      switch (entries[i].type)
        {
          case TYPE_INT:
            {
              *(int *)entries[i].val = atoi(val);
              break;
            }
        }


  return 0;
}

int config_read()
{
  int  c;
  FILE *fp;
  char var[BUFFERSIZE], val[BUFFERSIZE];

  fp = fopen(configfile, "r");
  if (!fp)
    {
      WARNING("Failed to open config file '%s'", configfile);
      return 1;
    }

  do
    {
      int i;

      for (i = 0; i < BUFFERSIZE; ++i)
        {
          c = fgetc(fp);
          if ((c == '=') || (c == '\n') || (c == EOF))
            break;
          else if ((c == ' ') || (c == '\t'))
            {
              --i;
              continue;
            }

          var[i] = c;
        }
      var[i] = '\0';

      if ((c == '\n') || (c == EOF))
        break;

      i = 0;
      for (i = 0; i < BUFFERSIZE; ++i)
        {
          c = fgetc(fp);
          if ((c == '\n') || (c == EOF))
            break;

          val[i] = c;
        }
      val[i] = '\0';

      if (process_line(var, val))
        return -1;
    }
  while (c != EOF);

  fclose(fp);

  return 0;
}

static void write_entry(FILE *fp, int i)
{
#define WRITE(s, val) \
  fprintf(fp, "%-*s = "s "\n", varmaxlen, entries[i].var, val)

  switch (entries[i].type)
    {
      case TYPE_INT:
        {
          WRITE("%d", *(int *)entries[i].val);
          break;
        }
    }

#undef WRITE
}

int config_write()
{
  int  i;
  FILE *fp;

  fp = fopen(configfile, "w");
  if (!fp)
    {
      ERROR("Failed to open config file '%s' for writing", configfile);
      return 1;
    }

  for (i = 0; i < ENTRIESLEN; ++i)
    write_entry(fp, i);

  fclose(fp);

  return 0;
}

Config *config_get()
{
  return &config;
}
