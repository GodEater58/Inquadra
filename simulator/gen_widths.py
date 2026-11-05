#!/usr/bin/env python3
"""Larghezze dei caratteri per il simulatore: quelle VERE di Magic Lantern (file RBF in data/fonts),
cosi' i controlli "testo fuori dallo schermo" valgono anche sulla 6D.
Uso: python3 gen_widths.py <cartella data/fonts di ML>"""
import struct, sys, os
MAGIC = b'\xE0\x0E\xF0\x0D\x03\x00\x00\x00'
NAMES = ["term12", "term20", "argnor23", "argnor28", "argnor32"]   # FONT_DYN 0..4 (rbf_font.c)
src = sys.argv[1] if len(sys.argv) > 1 else "fonts"
heights, widths = [], []
for n in NAMES:
    f = open(os.path.join(src, n + ".rbf"), "rb")
    assert f.read(8) == MAGIC
    f.read(64)
    charsize, points, height, maxw, first, last, _u, wmap, cmap, desc, il = struct.unpack("=11l", f.read(44))
    f.seek(wmap); w = list(f.read(last - first + 1))
    row = [0] * 128
    for c in range(32, 128):
        if first <= c <= last: row[c] = w[c - first]
    heights.append(height); widths.append(row)
out = ["/* generato da gen_widths.py dai caratteri RBF di Magic Lantern: larghezze vere della 6D */",
       "static const int sim_font_h[8] = {%s,0,0,0};" % ",".join(map(str, heights)),
       "static const unsigned char sim_w[8][128] = {"]
for r in widths + [[0] * 128] * 3:
    out.append("  {" + ",".join(map(str, r)) + "},")
out.append("};")
open("shim/widths.h", "w").write("\n".join(out) + "\n")
print("shim/widths.h", heights)
