#!/usr/bin/env python3
"""Generate the icon.c file used for the default icon.

Usage:

    make_icon.py in_file.png out_file.inc
"""

import numpy as np
import PIL.Image
import sys

image = np.array(PIL.Image.open(sys.argv[1]).convert("RGBA"))

with open(sys.argv[2], "w") as f:
    f.write("/* Generated using make_icon.py */\n")
    f.write(f"#define ICON_WIDTH {image.shape[1]}\n")
    f.write(f"#define ICON_HEIGHT {image.shape[0]}\n")
    f.write("static uint32_t icon_data[] = {\n")
    for i, row in enumerate(image):
        for j, col in enumerate(row):
            # Premultiply alpha.
            col = col.astype(np.float32) / 255.
            col = (col * col[-1] * 255.).astype(np.uint32)
            col = (col[0] << 24) + (col[1] << 16) + (col[2] << 8) + col[3]
            f.write(f"{col:#010x}")
            if i + 1 < image.shape[0] or j + 1 < image.shape[1]:
                f.write(",")
        f.write("\n")
    f.write("};\n")
