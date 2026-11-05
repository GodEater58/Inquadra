#!/usr/bin/env python3
"""Converte i caratteri RBF di Magic Lantern (data/fonts) in un header C per la demo nel browser:
cosi' il testo della demo e' identico a quello della 6D. Uso:
    python3 tools/web/rbf2c.py <cartella data/fonts di ML> simulator/web/fonts_ml.h
I caratteri sono quelli di Magic Lantern (GPL), come il resto del progetto."""
import struct, sys, os

MAGIC = b'\xE0\x0E\xF0\x0D\x03\x00\x00\x00'
NAMES = ["term12", "term20", "argnor23", "argnor28", "argnor32"]   # ordine di rbf_font.c (FONT_DYN 0..4)

def load(path):
    f = open(path, "rb")
    assert f.read(8) == MAGIC, path
    f.read(64)
    charsize, points, height, maxwidth, first, last, _u, wmap, cmap, descent, intline = struct.unpack("=11l", f.read(44))
    n = last - first + 1
    f.seek(wmap); w = list(f.read(n))
    f.seek(cmap); c = f.read(n * charsize)
    width = 8 * charsize // height                     # bit per riga (multiplo di 8)
    return dict(height=height, first=first, last=last, charsize=charsize, width=width, w=w, c=c)

def main():
    src, out = sys.argv[1], sys.argv[2]
    lines = ["/* fonts_ml.h - GENERATO da tools/web/rbf2c.py dai caratteri RBF di Magic Lantern (GPL). */",
             "struct ml_font { int height, first, last, charsize, width; const unsigned char *w; const unsigned char *c; };"]
    for i, name in enumerate(NAMES):
        f = load(os.path.join(src, name + ".rbf"))
        lines.append("static const unsigned char mlf%d_w[] = {%s};" % (i, ",".join(map(str, f["w"]))))
        cs = f["c"]
        body = ",".join(str(b) for b in cs)
        lines.append("static const unsigned char mlf%d_c[] = {%s};" % (i, body))
    lines.append("static const struct ml_font ml_fonts[5] = {")
    for i, name in enumerate(NAMES):
        f = load(os.path.join(src, name + ".rbf"))
        lines.append("    { %d, %d, %d, %d, %d, mlf%d_w, mlf%d_c },   /* %s */" % (f["height"], f["first"], f["last"], f["charsize"], f["width"], i, i, name))
    lines.append("};")
    open(out, "w").write("\n".join(lines) + "\n")
    print(out)

main()
