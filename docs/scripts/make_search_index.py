#!/usr/bin/env python
"""
Script to read html_refs and convert it to a javascript index.
"""

input = "html_refs"
output = "search_index.js"

# Read input and fill arrays.
search_index = []
search_urls = []
for line in open(input):
    entry, link = line.split(":")
    entry = entry.strip("[]")
    link = link.strip()
    search_index.append('"%s"' % entry)
    search_urls.append('"%s"' % link)

# Write javascript output.
out = open(output, "w")
out.write("var search_index=[")
out.write(",".join(search_index))
out.write("]\n")
out.write("var search_urls=[")
out.write(",".join(search_urls))
out.write("]\n")
