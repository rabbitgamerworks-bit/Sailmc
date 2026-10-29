#!/bin/bash
# Draws the SailMC icon (an isometric grass block with a sail) with ImageMagick.
set -e
cd "$(dirname "$0")"
S=512
convert -size ${S}x${S} xc:none \
  -fill '#2d6a2f' -draw "polygon 256,40 470,150 256,260 42,150" \
  -fill '#7a5230' -draw "polygon 42,150 256,260 256,480 42,370" \
  -fill '#5c3b20' -draw "polygon 470,150 256,260 256,480 470,370" \
  -fill '#3f8f3f' -draw "polygon 42,150 256,260 256,300 42,190" \
  -fill '#357a35' -draw "polygon 470,150 256,260 256,300 470,190" \
  -fill '#4caf50' -draw "polygon 256,60 440,150 256,240 72,150" \
  -fill '#ffffff' -draw "polygon 250,70 250,200 160,200" \
  -fill '#e0f2f1' -draw "polygon 262,90 262,200 330,200" \
  -stroke '#ffffff' -strokewidth 6 -draw "line 256,60 256,215" \
  sailmc-512.png
for s in 86 108 128 172; do
  mkdir -p ${s}x${s}
  convert sailmc-512.png -resize ${s}x${s} ${s}x${s}/sailmc.png
done
rm sailmc-512.png
