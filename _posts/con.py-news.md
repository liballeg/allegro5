---
#!/usr/bin/env python3
---
import sys
import re

for f in sys.argv:
    rows = open(f).read().splitlines()
    f2 = re.sub(r"^news\.", "", f)
    f2 += "-news"
    f2 += ".md"
    o = open(f2, "w")
    c = "---\n"
    c += re.sub(".*-.*-.*-", "title:", rows[0])
    c += "\n---\n"
    c += "\n".join(rows[1:])
    o.write(c)
    print(f)