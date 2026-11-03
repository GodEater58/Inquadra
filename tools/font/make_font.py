#!/usr/bin/env python3
"""Genera module/iride/font_small.inc: il carattere piccolo di Iride (Inter Medium 13 px,
SIL Open Font License) in bitmap a 4 livelli (2 bit per pixel), caratteri ASCII 32-126.

Uso: python3 tools/font/make_font.py [percorso di Inter-Medium.otf]
Magic Lantern ha come carattere piccolo solo Terminus 12, a spaziatura fissa: accanto ai
caratteri grandi (simili ad Arial) sembrava "da terminale"."""
import os
import sys
from PIL import Image, ImageDraw, ImageFont

FONT = sys.argv[1] if len(sys.argv) > 1 else "/usr/share/fonts/opentype/inter/Inter-Medium.otf"
SIZE = 13
OUT = os.path.join(os.path.dirname(__file__), "..", "..", "module", "iride", "font_small.inc")

f = ImageFont.truetype(FONT, SIZE)
ascent, descent = f.getmetrics()
CAN_H = ascent + descent + 4
glyphs = []
top_all, bot_all = CAN_H, 0
for code in range(32, 127):
    ch = chr(code)
    adv = int(round(f.getlength(ch)))
    W = adv + 8
    m = Image.new("L", (W, CAN_H), 0)
    ImageDraw.Draw(m).text((2, 2), ch, font=f, fill=255)
    px = m.load()
    cols = [x for x in range(W) if any(px[x, y] > 30 for y in range(CAN_H))]
    rows = [y for y in range(CAN_H) if any(px[x, y] > 30 for x in range(W))]
    if cols:
        x0, x1 = cols[0], cols[-1] + 1
        top_all, bot_all = min(top_all, rows[0]), max(bot_all, rows[-1] + 1)
    else:
        x0 = x1 = 2
    glyphs.append((code, adv, x0 - 2, x1 - x0, m))

H = bot_all - top_all
BASE = 2 + ascent - top_all              # riga della linea di base dentro la cella
bits, offs = [], []
for code, adv, lb, w, m in glyphs:
    offs.append(len(bits))
    px = m.load()
    for y in range(top_all, bot_all):
        for x in range(w):
            v = px[2 + lb + x, y]
            bits.append(min(3, (v + 42) // 85))
packed = []
for i in range(0, len(bits), 4):
    b = bits[i:i + 4] + [0] * (4 - len(bits[i:i + 4]))
    packed.append(b[0] | b[1] << 2 | b[2] << 4 | b[3] << 6)

lines = [
    "/* font_small.inc - GENERATO da tools/font/make_font.py, non modificare a mano.",
    " * Inter Medium %d px (c) The Inter Project Authors, SIL Open Font License 1.1." % SIZE,
    " * 4 livelli di copertura (2 bit), caratteri ASCII 32-126. */",
    "#define FS_H %d          /* righe di ogni carattere */" % H,
    "#define FS_BASE %d       /* riga della linea di base */" % BASE,
    "static const unsigned char fs_adv[95] = { %s };" % ", ".join(str(g[1]) for g in glyphs),
    "static const signed char fs_lb[95] = { %s };" % ", ".join(str(g[2]) for g in glyphs),
    "static const unsigned char fs_w[95] = { %s };" % ", ".join(str(g[3]) for g in glyphs),
    "static const unsigned short fs_off[95] = { %s };" % ", ".join(str(o) for o in offs),
    "static const unsigned char fs_bits[%d] = {" % len(packed),
]
for i in range(0, len(packed), 24):
    lines.append("    " + ", ".join(str(v) for v in packed[i:i + 24]) + ",")
lines.append("};")
open(OUT, "w").write("\n".join(lines) + "\n")
print(f"{OUT}: altezza {H}, base {BASE}, {len(packed)} byte")
