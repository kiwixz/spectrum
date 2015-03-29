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

#version 450

const uniform float blur[8] = {
  0.1517f, 0.0379f, 0.0303f, 0.0228f, 0.0152f, 0.0076f, 0.0038f, 0.0008f
};

const int blurmid = blur.length() / 2;

in vec2 vf_texcoord;
layout(location = 0) out vec4 color;

uniform sampler2D tex;

void main()
{
  ivec2 texcoord;
  int   x, y, dist, disty;

  texcoord = ivec2(textureSize(tex, 0) * vf_texcoord);

  color = vec4(0.0f);
  for (y = blur.length() - 1; y >= 0; --y)
    {
      disty = abs(y - blurmid);
      for (x = 0; x < blur.length(); ++x)
        {
          dist = max(abs(x - blurmid), disty);

          color +=
            texelFetch(tex, texcoord + ivec2(x + blurmid, y + blurmid), 0)
            * blur[dist];
        }
    }
}
