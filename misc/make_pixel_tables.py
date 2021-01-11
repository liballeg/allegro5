#!/usr/bin/env python3
import optparse, sys

def main(argv):
    p = optparse.OptionParser()
    p.description = ("When run from the toplevel A5 folder, this will "
        "re-create the src/pixel_tables.inc.""")
    p.parse_args()

    with open("src/pixel_tables.inc", "w") as f:
        f.write("""// Warning: This file was created by make_pixel_tables.py - do not edit.

""");

        f.write("float _al_u8_to_float[] = {\n")
        for i in range(256):
            f.write(f"   {i / 255.0},\n")
        f.write("};\n\n")

        f.write("int _al_rgb_scale_1[] = {\n")
        for i in range(2):
            f.write(f"   {i * 255 // 1},\n")
        f.write("};\n\n")

        f.write("int _al_rgb_scale_4[] = {\n")
        for i in range(16):
            f.write(f"   {i * 255 // 15},\n")
        f.write("};\n\n")

        f.write("int _al_rgb_scale_5[] = {\n")
        for i in range(32):
            f.write(f"   {i * 255 // 31},\n")
        f.write("};\n\n")

        f.write("int _al_rgb_scale_6[] = {\n")
        for i in range(64):
            f.write(f"   {i * 255 // 63},\n")
        f.write("};\n\n")

        f.write("""// Warning: This file was created by make_pixel_tables.py - do not edit.
""");

if __name__ == "__main__":
    main(sys.argv)
