#!/usr/bin/env python3
import collections, optparse, re, sys

formats_by_name = {}
formats_list = []

def read_color_h(filename):
    """
    Read in the list of formats.
    """
    formats = []
    inside_enum = False
    for line in open(filename):
        line = line.strip()
        if line == "{": continue
        if line == "typedef enum A5O_PIXEL_FORMAT": inside_enum = True
        elif inside_enum:
            match = re.match(r"\s*A5O_PIXEL_FORMAT_(\w+)", line)
            if match:
                formats.append(match.group(1))
            else:
                break
    return formats

def parse_format(format):
    """
    Parse the format name into an info structure.
    """
    if format.startswith("ANY"): return None
    if "DXT" in format: return None

    separator = format.find("_")
    class Info: pass
    class Component: pass

    Info = collections.namedtuple("Info", "components, name, little_endian,"
        "float, single_channel, size")
    Component = collections.namedtuple("Component", "color, size,"
        "position_from_left, position")

    pos = 0
    info = Info(
        components={},
        name=format,
        little_endian="_LE" in format,
        float=False,
        single_channel=False,
        size=None,
    )

    if "F32" in format:
        return info._replace(
            float=True,
            size=128,
        )

    if "SINGLE_CHANNEL" in format:
        return info._replace(
            single_channel=True,
            size=8,
        )

    for i in range(separator):
        c = Component(
            color=format[i],
            size=int(format[separator + 1 + i]),
            position_from_left=pos,
            position=None,
        )
        info.components[c.color] = c
        pos += c.size
    size = pos
    return info._replace(
        components={k: c._replace(position=size - c.position_from_left - c.size)
                    for k, c in info.components.items()},
        size=size,
        float=False,
    )

def macro_lines(info_a, info_b):
    """
    Write out the lines of a conversion macro.
    """
    r = ""

    names = list(info_b.components.keys())
    names.sort()

    if info_a.float:
        if info_b.single_channel:
            return "   (uint32_t)((x).r * 255)\n"
        lines = []
        for name in names:
            if name == "X": continue
            c = info_b.components[name]
            mask = (1 << c.size) - 1
            lines.append(
                "((uint32_t)((x)." + name.lower() + " * " + str(mask) +
                ") << " + str(c.position) + ")")
        r += "   ("
        r += " | \\\n    ".join(lines)
        r += ")\n"
        return r

    if info_b.float:
        if info_a.single_channel:
            return "   al_map_rgb(x, 0, 0)\n"
        lines = []
        for name in "RGBA":
            if name not in info_a.components: break
            c = info_a.components[name]
            mask = (1 << c.size) - 1
            line = "((x) >> " + str(c.position) + ") & " + str(mask)
            if c.size < 8:
                line = "_al_rgb_scale_" + str(c.size) + "[" + line + "]"
            lines.append(line)

        r += "   al_map_rgba(" if len(lines) == 4 else "   al_map_rgb("
        r += ",\\\n   ".join(lines)
        r += ")\n"
        return r

    if info_a.single_channel:
        lines = []
        for name in names:
           # Only map to R, and let A=1
           if name in ["X", "G", "B"]:
               continue
           c = info_b.components[name]
           shift = 8 - c.size - c.position
           m = hex(((1 << c.size) - 1) << c.position)
           if name == "A":
               lines.append(m)
               continue
           if shift > 0:
               lines.append("(((x) >> " + str(shift) + ") & " + m + ")")
           elif shift < 0:
               lines.append("(((x) << " + str(-shift) + ") & " + m + ")")
           else:
               lines.append("((x) & " + m + ")")
        r += "   ("
        r += " | \\\n   ".join(lines)
        r += ")\n"
        return r

    if info_b.single_channel:
        c = info_a.components["R"]
        m = (1 << c.size) - 1
        extract = "(((x) >> " + str(c.position) + ") & " + hex(m) + ")"
        if (c.size != 8):
            scale = "(_al_rgb_scale_" + str(c.size) + "[" + extract + "])"
        else:
            scale = extract
        r += "   " + scale + "\n"
        return r

    # Generate a list of (mask, shift, add) tuples for all components.
    ops = {}
    for name in names:
        if name == "X": continue # We simply ignore X components.
        c_b = info_b.components[name]
        if name not in info_a.components:
            # Set A component to all 1 bits if the source doesn't have it.
            if name == "A":
                add = (1 << c_b.size) - 1
                add <<= c_b.position
                ops[name] = (0, 0, add, 0, 0, 0)
            continue
        c_a = info_a.components[name]
        mask = (1 << c_b.size) - 1
        shift_right = c_a.position
        mask_pos = c_a.position
        shift_left = c_b.position
        bitdiff = c_a.size - c_b.size
        if bitdiff > 0:
            shift_right += bitdiff
            mask_pos += bitdiff
        else:
            shift_left -= bitdiff
            mask = (1 << c_a.size) - 1

        mask <<= mask_pos
        shift = shift_left - shift_right
        ops[name] = (mask, shift, 0, c_a.size, c_b.size, mask_pos)

    # Collapse multiple components if possible.
    common_shifts = {}
    for name, (mask, shift, add, size_a, size_b, mask_pos) in ops.items():
        if not add and not (size_a != 8 and size_b == 8):
            if shift in common_shifts: common_shifts[shift].append(name)
            else: common_shifts[shift] = [name]
    for newshift, colors in common_shifts.items():
        if len(colors) == 1: continue
        newname = ""
        newmask = 0
        colors.sort()
        masks_pos = []
        for name in colors:
            mask, shift, add, size_a, size_b, mask_pos = ops[name]

            names.remove(name)
            newname += name
            newmask |= mask
            masks_pos.append(mask_pos)
        names.append(newname)
        ops[newname] = (newmask, newshift, 0, size_a, size_b, min(masks_pos))

    # Write out a line for each remaining operation.
    lines = []
    add_format = "0x%0" + str(info_b.size >> 2) + "x"
    mask_format = "0x%0" + str(info_a.size >> 2) + "x"
    for name in names:
        if not name in ops: continue
        mask, shift, add, size_a, size_b, mask_pos = ops[name]
        if add:
            line = "(" + (add_format % add) + ")"
            lines.append((line, name, 0, size_a, size_b, mask_pos))
            continue
        mask_string = "((x) & " + (mask_format % mask) + ")"
        if (size_a != 8 and size_b == 8):
            line = "(_al_rgb_scale_" + str(size_a) + "[" + mask_string + " >> %2d" % mask_pos + "]"
        else:
            if (shift > 0):
                line = "(" + mask_string + " << %2d" % shift + ")"
            elif (shift < 0):
                line = "(" + mask_string + " >> %2d" % -shift + ")"
            else:
                line = mask_string + "      "
        lines.append((line, name, shift, size_a, size_b, mask_pos))

    # Concoct the macro.
    for i in range(len(lines)):
        line, name, shift, size_a, size_b, mask_pos = lines[i]
        if i == 0: start = "   ("
        else: start = "    "
        if i == len(lines) - 1: cont = ")   "
        else: cont = " | \\"
        if size_a != 8 and size_b == 8:
            shift = shift+(mask_pos-(8-size_a))
            if shift > 0:
                backshift = " << %2d)" % shift
            elif shift < 0:
                backshift = " >> %2d)" % -shift
            else:
                backshift = "       )"
        else:
            backshift = "       "
        r += start + line + backshift + " /* " + name + " */" + cont + "\n"
    return r


def converter_macro(info_a, info_b):
    """
    Create a conversion macro.
    """
    if not info_a or not info_b: return None

    name = "A5O_CONVERT_" + info_a.name + "_TO_" + info_b.name

    r = ""

    if info_a.little_endian or info_b.little_endian:
        r += "#ifdef A5O_BIG_ENDIAN\n"
        r += "#define " + name + "(x) \\\n"
        if info_a.name == "ABGR_8888_LE":
            r += macro_lines(formats_by_name["RGBA_8888"], info_b)
        elif info_b.name == "ABGR_8888_LE":
            r += macro_lines(info_a, formats_by_name["RGBA_8888"])
        else:
            r += "#error Conversion %s -> %s not understood by make_converters.py\n" % (
                info_a.name, info_b.name)
        r += "#else\n"
        r += "#define " + name + "(x) \\\n"
        r += macro_lines(info_a, info_b)
        r += "#endif\n"
    else:
        r += "#define " + name + "(x) \\\n"
        r += macro_lines(info_a, info_b)

    return r

def write_convert_h(filename):
    """
    Create the file with all the conversion macros.
    """
    f = open(filename, "w")
    f.write("""\
// Warning: This file was created by make_converters.py - do not edit.
#ifndef __al_included_allegro5_aintern_convert_h
#define __al_included_allegro5_aintern_convert_h

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_pixels.h"
""")

    for a in formats_list:
        for b in formats_list:
            if b == a: continue
            macro = converter_macro(a, b)
            if macro:
                f.write(macro)

    f.write("""\
#endif
// Warning: This file was created by make_converters.py - do not edit.
""")

def converter_function(info_a, info_b):
    """
    Create a string with one conversion function.
    """
    name = info_a.name.lower() + "_to_" + info_b.name.lower()
    params = "const void *src, int src_pitch,\n"
    params += "   void *dst, int dst_pitch,\n"
    params += "   int sx, int sy, int dx, int dy, int width, int height"
    declaration = "static void " + name + "(" + params + ")"

    macro_name = "A5O_CONVERT_" + info_a.name + "_TO_" + info_b.name

    types_and_sizes = {
        8 : ("uint8_t", "", 1),
        15 : ("uint16_t", "", 2),
        16 : ("uint16_t", "", 2),
        24: ("uint8_t", " * 3", 1),
        32 : ("uint32_t", "", 4),
        128 : ("A5O_COLOR", "", 16)}
    a_type, a_count, a_size = types_and_sizes[info_a.size]
    b_type, b_count, b_size = types_and_sizes[info_b.size]

    if a_count == "" and b_count == "":
        conversion = """\
         *dst_ptr = %(macro_name)s(*src_ptr);
         dst_ptr++;
         src_ptr++;""" % locals()
    else:

        if a_count != "":
            s_conversion = """\
         #ifdef A5O_BIG_ENDIAN
         int src_pixel = src_ptr[2] | (src_ptr[1] << 8) | (src_ptr[0] << 16);
         #else
         int src_pixel = src_ptr[0] | (src_ptr[1] << 8) | (src_ptr[2] << 16);
         #endif
""" % locals()

        if b_count != "":
            d_conversion = """\
         #ifdef A5O_BIG_ENDIAN
         dst_ptr[0] = dst_pixel >> 16;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel;
         #else
         dst_ptr[0] = dst_pixel;
         dst_ptr[1] = dst_pixel >> 8;
         dst_ptr[2] = dst_pixel >> 16;
         #endif
""" % locals()

        if a_count != "" and b_count != "":
            conversion = s_conversion + ("""\
         int dst_pixel = %(macro_name)s(src_pixel);
""" % locals()) + d_conversion

        elif a_count != "":
            conversion = s_conversion + ("""\
         *dst_ptr = %(macro_name)s(src_pixel);
""" % locals())

        else:
            conversion = ("""\
         int dst_pixel = %(macro_name)s(*src_ptr);
""" % locals()) + d_conversion

        conversion += """\
         src_ptr += 1%(a_count)s;
         dst_ptr += 1%(b_count)s;""" % locals()

    r = declaration + "\n"
    r += "{\n"
    r += """\
   int y;
   const %(a_type)s *src_ptr = (const %(a_type)s *)((const char *)src + sy * src_pitch);
   %(b_type)s *dst_ptr = (void *)((char *)dst + dy * dst_pitch);
   int src_gap = src_pitch / %(a_size)d - width%(a_count)s;
   int dst_gap = dst_pitch / %(b_size)d - width%(b_count)s;
   src_ptr += sx%(a_count)s;
   dst_ptr += dx%(b_count)s;
   for (y = 0; y < height; y++) {
      %(b_type)s *dst_end = dst_ptr + width%(b_count)s;
      while (dst_ptr < dst_end) {
%(conversion)s
      }
      src_ptr += src_gap;
      dst_ptr += dst_gap;
   }
""" % locals()

    r += "}\n"

    return r

def write_convert_c(filename):
    """
    Write out the file with the conversion functions.
    """
    f = open(filename, "w")
    f.write("""\
// Warning: This file was created by make_converters.py - do not edit.
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_convert.h"
""")

    for a in formats_list:
        for b in formats_list:
            if b == a: continue
            if not a or not b: continue
            function = converter_function(a, b)
            f.write(function)

    f.write("""\
void (*_al_convert_funcs[A5O_NUM_PIXEL_FORMATS]
   [A5O_NUM_PIXEL_FORMATS])(const void *, int, void *, int,
   int, int, int, int, int, int) = {
""")
    for a in formats_list:
        if not a:
            f.write("   {NULL},\n")
        else:
            f.write("   {")
            was_null = False
            for b in formats_list:
                if b and a != b:
                    name = a.name.lower() + "_to_" + b.name.lower()
                    f.write("\n      " + name + ",")
                    was_null = False
                else:
                    if not was_null: f.write("\n     ")
                    f.write(" NULL,")
                    was_null = True
            f.write("\n   },\n")

    f.write("""\
};

// Warning: This file was created by make_converters.py - do not edit.
""")

def main(argv):
    global options
    p = optparse.OptionParser()
    p.description = """\
When run from the toplevel A5 folder, this will re-create the convert.h and
convert.c files containing all the low-level color conversion macros and
functions."""
    options, args = p.parse_args()

    # Read in color.h to get the available formats.
    formats = read_color_h("include/allegro5/color.h")

    print(formats)

    # Parse the component info for each format.
    for f in formats:
        info = parse_format(f)
        formats_by_name[f] = info
        formats_list.append(info)

    # Output a macro for each possible conversion.
    write_convert_h("include/allegro5/internal/aintern_convert.h")

    # Output a function for each possible conversion.
    write_convert_c("src/convert.c")

if __name__ == "__main__":
    main(sys.argv)

# vim: set sts=4 sw=4 et:
