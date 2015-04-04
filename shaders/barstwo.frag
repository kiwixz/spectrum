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
  0.0681818f, 0.0261009f, 0.0191761f, 0.0133168f,
  0.0085227f, 0.0047940f, 0.0021307f, 0.0005327f
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
  color = vec4(0.0f);

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
