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

const uniform int   ssaa = #SSAA;
const uniform float blur[8] = {
  1.0000000f,
  0.0500000f,
  0.0125000f,
  0.0031250f,
  0.0007813f,
  0.0001953f,
  0.0000488f,
  0.0000122f
};

const int blurmid = blur.length() / 2;

in vec2 vf_texcoord;
layout(location = 0) out vec4 color;

uniform sampler2D tex;

void main()
{
  ivec2 texcoord;
  int   i, x, y, dist, disty;

  texcoord = ivec2(textureSize(tex, 0) * vf_texcoord);
  color = vec4(vec3(0.0f), 1.0f);

  for (i = 0; i < ssaa; ++i)
    for (y = 0; y < blur.length(); ++y)
      {
        disty = abs(y - blurmid);
        for (x = 0; x < blur.length(); ++x)
          {
            dist = max(abs(x - blurmid), disty);

            color +=
              texelFetch(tex, texcoord + ivec2(ssaa * x + blurmid - i,
                                               ssaa * y + blurmid - i), 0)
              * blur[dist];
          }
      }

}
