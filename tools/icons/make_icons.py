#!/usr/bin/env python3
"""Genera module/iride/icons_small.inc: le icone delle liste dei menu Astro e Foto.

Ogni icona e' disegnata con forme vettoriali a 8 volte la grandezza, poi ridotta a 26x26 con
l'antialias e portata sui colori della 6D (scala dei grigi di Magic Lantern + arancione e pochi
altri). Sullo sfondo nero del riquadro i bordi restano morbidi. Uso: python3 tools/icons/make_icons.py
Con --anteprima scrive anche icons_small.png per guardarle."""
import math
import os
import sys
from PIL import Image, ImageDraw

N = 26                 # lato dell'icona sullo schermo
K = 8                  # sovracampionamento
S = N * K
OUT = os.path.join(os.path.dirname(__file__), "..", "..", "module", "iride", "icons_small.inc")

# colori della tavolozza (indice ML -> RGB come nel simulatore)
ORANGE, YELLOW, LBLUE, BLUE, GREEN = 0x13, 0x0F, 0x09, 0x0B, 0x06
PAL = {ORANGE: (255, 140, 0), YELLOW: (250, 215, 0), LBLUE: (110, 170, 255), BLUE: (40, 80, 210), GREEN: (60, 210, 90)}
for i in range(38, 80):
    g = int((i - 38) / 41 * 255)
    PAL[i] = (g, g, g)

OR = PAL[ORANGE]
LINE = (215, 215, 215)
MID = (140, 140, 140)
DARK = (80, 80, 80)
W = int(1.7 * K)       # spessore delle linee


def P(x, y):
    return (x * K, y * K)


def circle(d, cx, cy, r, outline=None, fill=None, width=W):
    d.ellipse([(cx - r) * K, (cy - r) * K, (cx + r) * K, (cy + r) * K], outline=outline, fill=fill, width=width)


def line(d, pts, col=LINE, width=W):
    d.line([P(x, y) for x, y in pts], fill=col, width=width, joint="curve")


def star4(d, cx, cy, r, col):
    pts = []
    for k in range(8):
        a = k * math.pi / 4 - math.pi / 2
        rr = r if k % 2 == 0 else r * 0.32
        pts.append(P(cx + rr * math.cos(a), cy + rr * math.sin(a)))
    d.polygon(pts, fill=col)


def icon_milkyway(d):
    import random
    from PIL import ImageFilter
    im = d._image
    glow = Image.new("L", im.size, 0)
    gd = ImageDraw.Draw(glow)
    gd.polygon([P(0, 17), P(19, 0), P(25, 4), P(5, 22)], fill=110)     # la fascia in diagonale
    glow = glow.filter(ImageFilter.GaussianBlur(1.6 * K))
    im.paste(Image.new("RGB", im.size, (150, 150, 165)), (0, 0), glow)
    rnd = random.Random(3)
    for i in range(26):
        x, y = rnd.uniform(1, 25), rnd.uniform(1, 18)
        circle(d, x, y, rnd.choice([0.4, 0.55, 0.75]), fill=(235, 235, 235), width=0)
    star4(d, 19, 6, 3.4, OR)
    d.polygon([P(0, 26), P(0, 21), P(5, 17), P(9, 20), P(14, 15), P(20, 20), P(26, 17), P(26, 26)], fill=(55, 55, 55))
    d.line([P(0, 21), P(5, 17), P(9, 20), P(14, 15), P(20, 20), P(26, 17)], fill=(120, 120, 120), width=K)


def icon_interval(d):
    circle(d, 13, 15, 9.2, outline=LINE)
    d.rectangle([P(11, 2.2), P(15, 4.4)], fill=LINE)
    line(d, [(13, 4.4), (13, 6)])
    line(d, [(20.5, 6.5), (22, 5)])
    line(d, [(13, 15), (17.5, 10.5)], col=OR, width=int(2 * K))
    circle(d, 13, 15, 1.6, fill=OR, width=0)
    for k in range(12):
        a = k * math.pi / 6
        line(d, [(13 + 7.3 * math.cos(a), 15 + 7.3 * math.sin(a)), (13 + 8.2 * math.cos(a), 15 + 8.2 * math.sin(a))], col=MID, width=K)


def icon_starfocus(d):
    for sx, sy in ((1, 1), (-1, 1), (1, -1), (-1, -1)):
        cx, cy = 13 - sx * 10, 13 - sy * 10
        line(d, [(cx, cy + sy * 5), (cx, cy), (cx + sx * 5, cy)])
    star4(d, 13, 13, 6.5, OR)
    circle(d, 13, 13, 1.4, fill=(255, 230, 190), width=0)


def icon_polar(d):
    circle(d, 13, 13, 10.5, outline=LINE)
    circle(d, 13, 13, 6, outline=MID, width=K)
    line(d, [(13, 1.5), (13, 24.5)], col=MID, width=K)
    line(d, [(1.5, 13), (24.5, 13)], col=MID, width=K)
    circle(d, 17.5, 8.5, 2.2, fill=OR, width=0)


def icon_calendar(d):
    d.rounded_rectangle([P(2.5, 4.5), P(23.5, 23.5)], radius=2.5 * K, outline=LINE, width=W)
    d.rectangle([P(3.3, 5.3), P(22.7, 9.5)], fill=OR)
    for x in (8, 18):
        line(d, [(x, 2), (x, 6.5)])
    for r in range(2):
        for c in range(3):
            circle(d, 7 + c * 4.2, 13.5 + r * 4.5, 0.9, fill=MID, width=0)
    circle(d, 19.5, 17.5, 3.6, fill=(235, 235, 235), width=0)
    circle(d, 21.0, 16.4, 3.1, fill=(0, 0, 0), width=0)


def icon_mwplan(d):
    line(d, [(1.5, 20.5), (24.5, 20.5)], col=MID)
    d.arc([P(3, 6), P(23, 34)], 200, 340, fill=LINE, width=W)
    for t in (0.2, 0.4, 0.6, 0.8):
        a = math.radians(200 + 140 * t)
        x, y = 13 + 10 * math.cos(a), 20 + 14 * math.sin(a)
        circle(d, x, y, 0.8, fill=MID, width=0)
    a = math.radians(285)
    circle(d, 13 + 10 * math.cos(a), 20 + 14 * math.sin(a), 2.4, fill=OR, width=0)
    d.polygon([P(13, 21.5), P(11, 25), P(15, 25)], fill=LINE)


def icon_moonshot(d):
    circle(d, 13, 13, 10, fill=(225, 225, 225), width=0)
    for x, y, r in ((9, 9, 2.2), (15, 16, 1.6), (10, 17, 1.2), (16, 8, 1.0)):
        circle(d, x, y, r, fill=(165, 165, 165), width=0)
    d.pieslice([P(3, 3), P(23, 23)], 90, 270, fill=(0, 0, 0))
    d.ellipse([P(8.5, 3), P(17.5, 23)], fill=(225, 225, 225))
    for x, y, r in ((12, 9, 1.4), (14, 17, 1.2)):
        circle(d, x, y, r, fill=(165, 165, 165), width=0)
    circle(d, 13, 13, 10, outline=(70, 70, 70), width=K)


def icon_tag(d):
    d.polygon([P(3, 5), P(14, 5), P(23, 14), P(14, 23), P(3, 12)], outline=LINE, width=W)
    circle(d, 8, 9.5, 2, fill=OR, width=0)
    line(d, [(12, 15), (16, 11)], col=MID, width=K)
    line(d, [(14, 17), (18, 13)], col=MID, width=K)


def icon_hdr(d):
    for i, g in enumerate((70, 140, 225)):
        x, y = 2.5 + i * 4.5, 13.5 - i * 4.5
        d.rectangle([P(x, y), P(x + 13, y + 10)], fill=(g, g, g), outline=(0, 0, 0), width=K)
    d.polygon([P(15, 16), P(18, 12), P(21, 16)], fill=OR)


def icon_fstack(d):
    for i in range(3):
        y = 17 - i * 5
        col = OR if i == 1 else (LINE if i == 2 else MID)
        d.polygon([P(4, y + 4), P(10, y), P(23, y), P(17, y + 4)], outline=col, width=W if i == 1 else K)


def icon_dual(d):
    d.rounded_rectangle([P(3, 3), P(23, 23)], radius=2 * K, outline=LINE, width=W)
    for r in range(4):
        y = 5.5 + r * 4.4
        d.rectangle([P(5, y), P(21, y + 2)], fill=(220, 220, 220) if r % 2 == 0 else (90, 90, 90))
    line(d, [(23.5, 7), (23.5, 19)], col=OR)


def icon_ettr(d):
    hs = [2, 3, 5, 4, 7, 9, 12, 15, 17, 14]
    for i, h in enumerate(hs):
        x = 2.5 + i * 1.9
        d.rectangle([P(x, 22 - h), P(x + 1.5, 22)], fill=(200, 200, 200) if i < 9 else (240, 240, 240))
    line(d, [(23, 3), (23, 23)], col=OR)
    line(d, [(1.5, 22.8), (24.5, 22.8)], col=MID, width=K)


def icon_motion(d):
    d.polygon([P(15, 2), P(6, 15), P(12, 15), P(9, 24), P(20, 10), P(14, 10), P(17, 2)], fill=OR)


def icon_lvtools(d):
    d.rounded_rectangle([P(2, 4), P(24, 22)], radius=2 * K, outline=MID, width=K)
    d.line([P(4, 19), P(10, 11), P(14, 15), P(18, 9), P(22, 16)], fill=OR, width=W, joint="curve")
    for k in range(4):
        x = 14 + k * 2.5
        line(d, [(x, 5.5), (x + 2, 3.5 + 2)], col=(220, 220, 220), width=K)


def icon_nd(d):
    circle(d, 13, 13, 10.5, outline=LINE)
    circle(d, 13, 13, 8.2, fill=(45, 45, 45), width=0)
    d.pieslice([P(4.8, 4.8), P(21.2, 21.2)], 200, 340, fill=(110, 110, 110))
    line(d, [(8, 19), (18, 7)], col=OR, width=int(1.4 * K))


ICONS = [("milkyway", icon_milkyway), ("interval", icon_interval), ("starfocus", icon_starfocus),
         ("polar", icon_polar), ("calendar", icon_calendar), ("mwplan", icon_mwplan),
         ("moonshot", icon_moonshot), ("tag", icon_tag), ("hdr", icon_hdr), ("fstack", icon_fstack),
         ("dual", icon_dual), ("ettr", icon_ettr), ("motion", icon_motion), ("lvtools", icon_lvtools),
         ("nd", icon_nd)]


def quantize(rgb):
    """colore piu' vicino della tavolozza; 0 = trasparente (nero dello sfondo)"""
    r, g, b = rgb
    if r + g + b < 30:
        return 0
    best, bd = 0, 1e9
    for idx, (pr, pg, pb) in PAL.items():
        dd = (r - pr) ** 2 * 3 + (g - pg) ** 2 * 4 + (b - pb) ** 2 * 2
        if dd < bd:
            best, bd = idx, dd
    return best if best != 38 else 0


def main():
    data, prev = [], Image.new("RGB", (len(ICONS) * (N + 4) * 4, (N + 4) * 4), (0, 0, 0))
    for k, (name, fn) in enumerate(ICONS):
        im = Image.new("RGB", (S, S), (0, 0, 0))
        fn(ImageDraw.Draw(im))
        sm = im.resize((N, N), Image.LANCZOS)
        px = [quantize(sm.getpixel((x, y))) for y in range(N) for x in range(N)]
        data.append((name, px))
        q = Image.new("RGB", (N, N))
        q.putdata([PAL.get(v, (0, 0, 0)) if v else (0, 0, 0) for v in px])
        prev.paste(q.resize((N * 4, N * 4), Image.NEAREST), (k * (N + 4) * 4, 8))
    lines = ["/* icons_small.inc - GENERATO da tools/icons/make_icons.py, non modificare a mano.",
             " * Icone %dx%d delle liste Astro e Foto: indici della tavolozza di ML, 0 = trasparente. */" % (N, N),
             "#define ICS_N %d" % N]
    for i, (name, _) in enumerate(data):
        lines.append("#define ICS_%s %d" % (name.upper(), i))
    lines.append("static const unsigned char ics_px[%d][%d] = {" % (len(data), N * N))
    for name, px in data:
        lines.append("    { /* %s */" % name)
        for y in range(N):
            lines.append("      " + ",".join(str(v) for v in px[y * N:(y + 1) * N]) + ",")
        lines.append("    },")
    lines.append("};")
    open(OUT, "w").write("\n".join(lines) + "\n")
    if "--anteprima" in sys.argv:
        prev.save(os.path.join(os.path.dirname(OUT), "..", "..", "simulator", "icons_small.png"))
    print(OUT, len(data), "icone")


main()
