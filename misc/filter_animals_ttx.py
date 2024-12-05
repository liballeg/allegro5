"""
This script is used to create NotoColorEmoji_Animals.ttf from
NotoColorEmoji.ttf, simply removing any character exept the ones in the
keep list below.
"""

keep = """
0x1f403
0x1f405
0x1f406
0x1f408
0x1f40a
0x1f40f
0x1f410
0x1f411
0x1f412
0x1f416
0x1f415
0x1f418
0x1f42b
0x1f420
0x1f422
0x1f427
0x1f41f
0x1f419
0x1f996
0x1f54a
0xfe0f
0x1f992
0x1f98c
""".splitlines()[1:]
keep = set(keep)

import xml.etree.ElementTree as ET
tree = ET.parse("NotoColorEmoji.ttx")
root = tree.getroot()

names = set()
for child in root:
    if child.tag == "cmap":
        for child2 in child.findall("cmap_format_12"):
                for g in child2.findall("map"):
                    if g.attrib["code"] not in keep:
                        child2.remove(g)
                    else:
                        names.add(g.attrib["name"])
for child in root:
    if child.tag == "cmap":
        for child2 in child.findall("cmap_format_14"):
                for g in child2.findall("map"):
                    if g.attrib["uv"] not in keep:
                        child2.remove(g)

for child in root:
    if child.tag == "CBDT":
        for child2 in child.findall("strikedata"):
                for cbdt in child2.findall("cbdt_bitmap_format_17"):
                    if cbdt.attrib["name"] not in names:
                        child2.remove(cbdt)

x = 0
for child in root:
    if child.tag == "GlyphOrder":
        for child2 in child.findall("GlyphID"):
            if child2.attrib["name"] not in names:
                child.remove(child2)
                x += 1
print(f"removed {x} GlyphOrder")

x = 0
for child in root:
    if child.tag == "vmtx":
        for child2 in child:
            if child2.attrib["name"] not in names:
                child.remove(child2)
                x += 1
print(f"removed {x} vmtx")

x = 0
for child in root:
    if child.tag == "hmtx":
        for child2 in child:
            if child2.attrib["name"] not in names:
                child.remove(child2)
                x += 1
print(f"removed {x} hmtx")

x = 0
for child in root.findall("CBLC"):
    for child2 in child.findall("strike"):
        for child3 in child2.findall("eblc_index_sub_table_1"):
            for g in child3.findall("glyphLoc"):
                if g.attrib["name"] not in names:
                    child3.remove(g)
                    x += 1
            if not child3.findall("glyphLoc"):
                child2.remove(child3)
        
print(f"removed {x} CBLC")

for child in root.findall("GSUB"):
    for child2 in child.findall("LookupList"):
        for child3 in child2.findall("Lookup"):
            for child4 in child3.findall("LigatureSubst"):
                child3.remove(child4)

for child in root.findall("GSUB"):
    for child2 in child.findall("LookupList"):
        for child3 in child2.findall("Lookup"):
            for child4 in child3.findall("MultipleSubst"):
                child3.remove(child4)

for child in root.findall("GSUB"):
    for child2 in child.findall("LookupList"):
        for child3 in child2.findall("Lookup"):
            for child4 in child3.findall("ChainContextSubst"):
                child3.remove(child4)

for child in root.findall("GSUB"):
    for child2 in child.findall("LookupList"):
        for child3 in child2.findall("Lookup"):
            for child4 in child3.findall("ContextSubst"):
                child3.remove(child4)

for child in root.findall("GSUB"):
    for child2 in child.findall("LookupList"):
        for child3 in child2.findall("Lookup"):
            for child4 in child3.findall("SingleSubst"):
                child3.remove(child4)



tree.write("out.ttx", encoding="utf-8", xml_declaration=True)
