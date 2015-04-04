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

const uniform float blur[16] = {
  0.0952744, 0.0214367, 0.0186738, 0.0161014, 0.0137195, 0.0115282, 0.0095274,
  0.0077172, 0.0060976, 0.0046684, 0.0034299, 0.0023819, 0.0015244, 0.0008575,
  0.0003811, 0.0000953
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
