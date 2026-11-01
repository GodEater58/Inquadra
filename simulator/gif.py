"""GIF del README dai fotogrammi del simulatore (prima: ./preview && python3 render.py).

Le transizioni tra schermate (zoom dal riquadro della Home, foglio del dialogo) in macchina
spostano la memoria video: qui sono ricomposte con la stessa geometria e la stessa curva
partendo dalle schermate ferme.

Uso: python3 gif.py CARTELLA_USCITA   ->  caricamento.gif, animazioni.gif
"""
import os
import sys

from PIL import Image, ImageDraw

Y0, Y1, W = 47, 444, 720
ORANGE = (255, 140, 0)
PANEL = (62, 62, 62)          # COLOR_GRAY(26), fondo del riquadro scelto


def png(name):
    return Image.open(f"png/{name}.png").convert("RGB")


def seq(prefix, n):
    return [png(f"{prefix}_{i:02d}") for i in range(n)]


def ease(t, d):
    if t >= d:
        return 1.0
    q = 1 - t / d
    return 1 - q * q * q


def lerp(a, b, e):
    return int(a + (b - a) * e)


def zoom(a, b, tile, dur=260, step=40, back=False):
    """Come tr_zoom: il rettangolo cresce dal riquadro (o ci rientra) col bordo arancione;
    dentro, la schermata intera rimpicciolita con le sue proporzioni su fondo grigio."""
    frames = []
    tx, ty, tw, th = tile
    H = Y1 - Y0
    inner = (a if back else b).crop((0, Y0, W, Y1))
    for t in range(step, dur + step, step):
        e = ease(t, dur)
        p = 1 - e if back else e
        x, y = lerp(tx, 0, p), lerp(ty, Y0, p)
        w, h = lerp(tw, W, p), lerp(th, H, p)
        f = (b if back else a).copy()
        k = min(w / W, h / H)
        sw, sh = max(1, int(W * k)), max(1, int(H * k))
        ImageDraw.Draw(f).rectangle([x, y, x + w - 1, y + h - 1], fill=PANEL)
        f.paste(inner.resize((sw, sh), Image.NEAREST), (x + (w - sw) // 2, y + (h - sh) // 2))
        f.paste(b.crop((0, 0, W, Y0)), (0, 0))
        f.paste(b.crop((0, Y1, W, 480)), (0, Y1))
        d = ImageDraw.Draw(f)
        for k in range(3):
            d.rectangle([x + k, y + k, x + w - 1 - k, y + h - 1 - k], outline=ORANGE)
        frames.append(f)
    return frames


def sheet(a, b, dur=240, step=40, down=False):
    """Come tr_sheet: il dialogo sale dal basso / riscende scoprendo la schermata sotto."""
    frames = []
    H = Y1 - Y0
    for t in range(step, dur + step, step):
        e = ease(t, dur)
        f = a.copy()
        if not down:
            ys = Y0 + int(H * (1 - e))
            f.paste(b.crop((0, Y0, W, Y0 + Y1 - ys)), (0, ys))
            if ys - 2 >= Y0:
                ImageDraw.Draw(f).rectangle([0, ys - 2, W - 1, ys - 1], fill=ORANGE)
        else:
            off = int(H * e)
            f.paste(b.crop((0, Y0, W, Y0 + off)), (0, Y0))
            f.paste(a.crop((0, Y0, W, Y1 - off)), (0, Y0 + off))
        f.paste(b.crop((0, 0, W, Y0)), (0, 0))
        f.paste(b.crop((0, Y1, W, 480)), (0, Y1))
        frames.append(f)
    return frames


def save(path, frames, durs):
    pal = [f.quantize(colors=128, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE) for f in frames]
    pal[0].save(path, save_all=True, append_images=pal[1:], duration=durs, loop=0, optimize=True)
    print(path, len(frames), "fotogrammi", os.path.getsize(path) // 1024, "KB")


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(out, exist_ok=True)

    # caricamento del rullino
    end = png("22_caricamento")
    shelf = png("g_scaffale_00")                 # lo scaffale col rullino appena caricato
    out_fr = sheet(end, shelf, down=True)
    fr = seq("anim_carica", 24) + [end] + out_fr
    save(os.path.join(out, "caricamento.gif"), fr, [118] * 24 + [1400] + [40] * (len(out_fr) - 1) + [1500])

    # avvio, Home, scaffale, dialogo e ritorno
    frames, durs = [], []

    def add(fs, d=40, hold=0):
        for i, f in enumerate(fs):
            frames.append(f)
            durs.append(d + (hold if i == len(fs) - 1 else 0))

    tile = (24 + 2 * 230 - 3, 62 - 3, 212 + 6, 196 + 6)        # riquadro RULLINI con la cornice
    splash = seq("g_splash", 26)
    home = seq("g_home", 34)
    shelf = png("g_scaffale_00")
    dx = seq("g_scaffale_dx", 10)
    down = seq("g_scaffale_giu", 13)
    dlg = png("g_dialogo_00")
    add(splash, hold=250)
    add(home, hold=500)
    add(zoom(home[-1], shelf, tile), hold=500)
    add(dx, hold=500)
    add(down, hold=600)
    add(sheet(down[-1], dlg), hold=1100)
    add(sheet(dlg, down[-1], down=True), hold=400)
    add(zoom(down[-1], home[-1], tile, back=True), hold=1200)
    save(os.path.join(out, "animazioni.gif"), frames, durs)


if __name__ == "__main__":
    main()
