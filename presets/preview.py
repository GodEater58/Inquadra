#!/usr/bin/env python3
"""Anteprima INDICATIVA dei preset: approssima i cursori crs: su un'immagine gia' sviluppata.
Non e' il motore di LightCraft/Lightroom: serve solo a giudicare il carattere di ogni pellicola.

Uso: preview.py OUT.png IMG_VERTICALE IMG_QUADRATA
"""
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy.interpolate import PchipInterpolator
from scipy.ndimage import gaussian_filter
from skimage.color import hsv2rgb, rgb2hsv

from films import FILMS
from make_presets import BANDS, full_name, settings

CENTERS = {"Red": 0, "Orange": 30, "Yellow": 60, "Green": 120, "Aqua": 180, "Blue": 240,
           "Purple": 275, "Magenta": 315}


def luma(x):
    return x[..., 0] * 0.2126 + x[..., 1] * 0.7152 + x[..., 2] * 0.0722


def band_weights(h):
    """Peso di ogni banda per tonalita' h (gradi), interpolazione triangolare tra centri vicini."""
    order = sorted(CENTERS.items(), key=lambda kv: kv[1])
    w = {b: np.zeros_like(h) for b in BANDS}
    for i, (b, c) in enumerate(order):
        cp = order[i - 1][1]
        cn = order[(i + 1) % len(order)][1]
        d = (h - c + 540) % 360 - 180
        left = (c - cp) % 360
        right = (cn - c) % 360
        w[b] = np.where(d < 0, np.clip(1 + d / left, 0, 1), np.clip(1 - d / right, 0, 1))
    return w


def curve_apply(x, pts):
    xs = np.array([p[0] for p in pts]) / 255.0
    ys = np.array([p[1] for p in pts]) / 255.0
    return np.clip(PchipInterpolator(xs, ys)(np.clip(x, 0, 1)), 0, 1)


def develop(img, f, seed=1):
    s = settings(f)
    x = img.astype(np.float64) / 255.0
    # luci
    L = luma(x)[..., None]
    hm = np.clip((L - 0.45) / 0.55, 0, 1) ** 1.5
    sm = np.clip(1 - L / 0.55, 0, 1) ** 1.5
    x = x + (s["Highlights2012"] / 100) * 0.28 * hm * x + (s["Shadows2012"] / 100) * 0.22 * sm * (1 - x) * 0.6
    x = x + (s["Whites2012"] / 100) * 0.12 * x ** 3 + (s["Blacks2012"] / 100) * 0.12 * (1 - x) ** 4
    k = s["Contrast2012"] / 100 * 0.55
    x = np.clip(x, 0, 1)
    x = x - k * np.sin(2 * np.pi * x) / (2 * np.pi)
    x = np.clip(x, 0, 1)
    # chiarezza (contrasto locale)
    if s["Clarity2012"]:
        blur = gaussian_filter(x, sigma=(12, 12, 0))
        x = np.clip(x + s["Clarity2012"] / 100 * 0.6 * (x - blur), 0, 1)
    hsv = rgb2hsv(x)
    h, sat, v = hsv[..., 0] * 360, hsv[..., 1], hsv[..., 2]
    w = band_weights(h)
    if s["ConvertToGrayscale"] == "True":
        g = luma(x)
        mix = sum(w[b] * s[f"GrayMixer{b}"] for b in BANDS) / 100
        g = np.clip(g * (1 + 0.9 * mix * sat), 0, 1)
        x = np.repeat(g[..., None], 3, axis=2)
    else:
        dh = sum(w[b] * s[f"HueAdjustment{b}"] for b in BANDS) / 100 * 30
        ds = sum(w[b] * s[f"SaturationAdjustment{b}"] for b in BANDS) / 100
        dl = sum(w[b] * s[f"LuminanceAdjustment{b}"] for b in BANDS) / 100
        h = (h + dh * np.clip(sat * 3, 0, 1)) % 360
        sat = sat * (1 + ds)
        v = v * (1 + 0.35 * dl * np.clip(sat * 2, 0, 1))
        sat = sat * (1 + s["Saturation"] / 100)
        sat = sat * (1 + s["Vibrance"] / 100 * (1 - np.clip(sat, 0, 1)))
        x = hsv2rgb(np.stack([h / 360, np.clip(sat, 0, 1), np.clip(v, 0, 1)], axis=-1))
        # color grading ombre/luci
        L = luma(x)[..., None]
        for key, wgt in (("Shadow", (1 - L) ** 2), ("Highlight", L ** 2)):
            hue = s[f"SplitToning{key}Hue"] / 360
            amt = s[f"SplitToning{key}Saturation"] / 100
            if amt:
                tint = hsv2rgb(np.array([[[hue, 1.0, 1.0]]]))[0, 0]
                tint = tint - tint.mean()
                x = x + wgt * amt * 0.35 * tint
    x = curve_apply(np.clip(x, 0, 1), f["curve"])
    # vignettatura
    if s["PostCropVignetteAmount"]:
        hh, ww = x.shape[:2]
        yy, xx = np.mgrid[0:hh, 0:ww]
        r = np.hypot((xx - ww / 2) / (ww / 2), (yy - hh / 2) / (hh / 2)) / np.sqrt(2)
        x = x * (1 + s["PostCropVignetteAmount"] / 100 * 0.8 * np.clip(r - 0.3, 0, 1) ** 2)[..., None]
    # grana (sulla luminanza, piu' forte nei mezzitoni)
    if s["GrainAmount"]:
        rng = np.random.default_rng(seed)
        n = rng.standard_normal(x.shape[:2])
        n = gaussian_filter(n, 0.4 + s["GrainSize"] / 100 * 1.6)
        n /= n.std() + 1e-9
        L = luma(x)
        amp = s["GrainAmount"] / 100 * 0.11 * (0.35 + 2.6 * L * (1 - L))
        x = x + (n * amp)[..., None]
    return (np.clip(x, 0, 1) * 255 + 0.5).astype(np.uint8)


def tile(imgs, label, sub, font, small):
    gap = 6
    w = sum(i.width for i in imgs) + gap * (len(imgs) - 1)
    h = imgs[0].height
    t = Image.new("RGB", (w, h + 58), (24, 24, 24))
    xo = 0
    for i in imgs:
        t.paste(i, (xo, 0))
        xo += i.width + gap
    d = ImageDraw.Draw(t)
    d.text((4, h + 6), label, fill=(240, 240, 240), font=font)
    d.text((4, h + 32), sub, fill=(150, 150, 150), font=small)
    return t


def main():
    out, tall, square = sys.argv[1], sys.argv[2], sys.argv[3]
    H = 330
    a = Image.open(tall).convert("RGB")
    a = a.resize((int(a.width * H / a.height), H), Image.LANCZOS)
    b = Image.open(square).convert("RGB")
    b = b.resize((int(b.width * H / b.height), H), Image.LANCZOS)
    A, B = np.asarray(a), np.asarray(b)
    try:
        font = ImageFont.truetype("DejaVuSans-Bold.ttf", 19)
        small = ImageFont.truetype("DejaVuSans.ttf", 14)
    except OSError:
        font = small = ImageFont.load_default()
    tiles = [tile([a, b], "Originale (base Color)", "nessun preset", font, small)]
    for i, f in enumerate(FILMS):
        da = Image.fromarray(develop(A, f, 10 + i))
        db = Image.fromarray(develop(B, f, 50 + i))
        tiles.append(tile([da, db], f"{i + 1:02d}  {full_name(f)}", f"{f['desc']}. {f['use']}", font, small))
    cols = 4
    tw, th = tiles[0].size
    rows = (len(tiles) + cols - 1) // cols
    pad = 14
    sheet = Image.new("RGB", (cols * tw + (cols + 1) * pad, rows * th + (rows + 1) * pad + 50), (14, 14, 14))
    d = ImageDraw.Draw(sheet)
    d.text((pad, 14), "Iride 6D - preset pellicola (anteprima indicativa)", fill=(255, 255, 255), font=font)
    for n, t in enumerate(tiles):
        r, c = divmod(n, cols)
        sheet.paste(t, (pad + c * (tw + pad), 50 + pad + r * (th + pad)))
    sheet.save(out, optimize=True)
    print(out, sheet.size)


if __name__ == "__main__":
    main()
