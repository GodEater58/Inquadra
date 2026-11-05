import sys, glob, os
from PIL import Image, ImageDraw, ImageFont
def pal(i):
    if i == 255: return None
    base = {0:None,1:(255,255,255),2:(0,0,0),3:(0,0,0),5:(0,220,230),6:(60,210,90),7:(120,200,120),8:(235,50,45),
            9:(110,170,255),11:(40,80,210),12:(150,25,25),14:(220,0,220),15:(250,215,0),19:(255,140,0)}
    if i in base: return base[i]
    if 38 <= i <= 79:
        g = int((i-38)/41*255); return (g,g,g)
    return (255,0,255)
F = {0:("DejaVuSansMono.ttf",11,12),1:("DejaVuSansMono.ttf",17,20),2:("DejaVuSans.ttf",19,23),3:("DejaVuSans.ttf",23,28),4:("DejaVuSans.ttf",27,32)}
fonts = {k:ImageFont.truetype("/usr/share/fonts/truetype/dejavu/"+v[0], v[1]) for k,v in F.items()}
# caratteri veri di Magic Lantern (fonts/*.rbf): il testo delle schermate e' identico a quello della 6D
import struct
def _rbf(path):
    f = open(path, "rb"); f.read(72)
    cs, pts, h, mw, first, last, _u, wmap, cmap, de, il = struct.unpack("=11l", f.read(44))
    f.seek(wmap); w = list(f.read(last - first + 1)); f.seek(cmap); c = f.read((last - first + 1) * cs)
    return dict(h=h, first=first, last=last, cs=cs, bpr=(8 * cs // h) // 8, w=w, c=c)
RBF = {i: _rbf(os.path.join(os.path.dirname(os.path.abspath(__file__)), "fonts", n + ".rbf"))
       for i, n in enumerate(["term12", "term20", "argnor23", "argnor28", "argnor32"])}
def rbf_text(im, fid, x, y, s, fg, bg):
    f = RBF.get(fid, RBF[2]); px = im.load()
    for ch in s:
        c = ord(ch)
        if c < f["first"] or c > f["last"]: continue
        w = f["w"][c - f["first"]]; g = f["c"][(c - f["first"]) * f["cs"]:]
        if bg is not None:
            for yy in range(f["h"]):
                for xx in range(w):
                    if 0 <= x + xx < 720 and 0 <= y + yy < 480: px[x + xx, y + yy] = bg
        for yy in range(f["h"]):
            for xx in range(w):
                if g[yy * f["bpr"] + xx // 8] & (1 << (xx % 8)) and 0 <= x + xx < 720 and 0 <= y + yy < 480:
                    px[x + xx, y + yy] = fg
        x += w
PALA = [((pal(i) + (255,)) if (i and pal(i)) else (0, 0, 0, 0)) for i in range(256)]
def render(opsfile, bg=None):
    im = Image.new("RGBA",(720,480),(0,0,0,0)); d = ImageDraw.Draw(im)
    for line in open(opsfile, encoding="utf-8", errors="replace"):
        p = line.rstrip("\n").split("\t")
        t = p[0]
        if t == "X": d.rectangle([0,0,719,479], fill=(0,0,0,0))
        elif t == "F":
            c,x,y,w,h = map(int,p[1:6]); col = pal(c)
            if w>0 and h>0: d.rectangle([x,y,x+w-1,y+h-1], fill=(col+(255,)) if col else (0,0,0,0))
        elif t == "R":
            c,x,y,w,h = map(int,p[1:6]); col = pal(c); d.rectangle([x,y,x+w-1,y+h-1], outline=(col+(255,)) if col else (0,0,0,0))
        elif t == "C":
            c,x,y,r,fill = map(int,p[1:6]); col = pal(c); col = (col+(255,)) if col else (0,0,0,0)
            if fill: d.ellipse([x-r,y-r,x+r,y+r], fill=col)
            else: d.ellipse([x-r,y-r,x+r,y+r], outline=col)
        elif t == "P":
            x,y = int(p[1]), int(p[2]); bs = bytes.fromhex(p[3])
            if not bs: continue
            px = [PALA[c] for c in bs]
            row = Image.new("RGBA", (len(bs), 1)); row.putdata(px)
            im.alpha_composite(row, (x, y))
        elif t == "L":
            c,x1,y1,x2,y2 = map(int,p[1:6]); col = pal(c); d.line([x1,y1,x2,y2], fill=(col+(255,)) if col else (0,0,0,0))
        elif t == "T":
            fid,fg,bgc,x,y = map(int,p[1:6]); s = p[6] if len(p) > 6 else ""
            b = pal(bgc) if bgc != 255 else None
            rbf_text(im, fid, x, y, s, (pal(fg) + (255,)) if pal(fg) else (0, 0, 0, 0), (b + (255,)) if b else None)
    base = Image.new("RGBA",(720,480),(0,0,0,255))
    if bg: base = Image.open(bg).convert("RGBA").resize((720,480))
    base.alpha_composite(im)
    return base.convert("RGB")
os.makedirs("png", exist_ok=True)
names = sorted(glob.glob("out/*.ops"))
imgs = []
for n in names:
    k = os.path.basename(n)[:-4]
    bg = (f"out/{k}_lv.pgm" if os.path.exists(f"out/{k}_lv.pgm") else "out/05_lv.pgm") if k.startswith("05") else None
    im = render(n, bg); im.save(f"png/{k}.png"); imgs.append((k, im))
# foglio riassuntivo 2 colonne
pad, lab = 16, 30
cols = 3; rows = (len(imgs)+2)//3
sheet = Image.new("RGB", (cols*(720+pad)+pad, rows*(480+pad+lab)+pad), (24,24,28))
d = ImageDraw.Draw(sheet); ft = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 20)
for i,(k,im) in enumerate(imgs):
    x = pad + (i%cols)*(720+pad); y = pad + (i//cols)*(480+pad+lab)
    d.text((x, y), k[3:].replace("_"," ").capitalize(), font=ft, fill=(230,230,230))
    sheet.paste(im, (x, y+lab))
sheet.save("png/iride_schermate.png")
print("ok", len(imgs))
