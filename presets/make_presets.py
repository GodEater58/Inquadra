#!/usr/bin/env python3
"""Genera i preset XMP (crs:) delle pellicole di Iride 6D e un'anteprima indicativa.

Uso: make_presets.py OUT_DIR [IMMAGINE ...]
     make_presets.py --ricetta MIA1.TXT [MIA2.TXT ...] OUT_DIR   (ricette fatte sulla 6D)
"""
import os
import sys
import uuid
import zipfile
from xml.sax.saxutils import escape

from films import FILMS, GROUP

BANDS = ["Red", "Orange", "Yellow", "Green", "Aqua", "Blue", "Purple", "Magenta"]
IDENT = [(0, 0), (255, 255)]


UNSIGNED = {"GrainAmount", "GrainSize", "GrainFrequency", "SplitToningShadowHue",
            "SplitToningShadowSaturation", "SplitToningHighlightHue", "SplitToningHighlightSaturation",
            "ColorGradeBlending", "PostCropVignetteStyle", "PostCropVignetteMidpoint",
            "PostCropVignetteFeather"}


def fmt(k, v):
    v = int(round(v))
    return f"+{v}" if v > 0 and k not in UNSIGNED else str(v)


def alt(tag, text):
    return (f'   <crs:{tag}>\n    <rdf:Alt>\n     <rdf:li xml:lang="x-default">{escape(text)}</rdf:li>\n'
            f'    </rdf:Alt>\n   </crs:{tag}>\n')


def seq(tag, pts):
    items = "".join(f"     <rdf:li>{x}, {y}</rdf:li>\n" for x, y in pts)
    return f"   <crs:{tag}>\n    <rdf:Seq>\n{items}    </rdf:Seq>\n   </crs:{tag}>\n"


def full_name(f):
    return f"{f['brand']} {f['name']}"


def settings(f):
    """Campi crs: della pellicola (solo quelli che cambiano)."""
    # ogni preset scrive lo stesso insieme di campi: passando da una pellicola all'altra
    # non restano residui della precedente
    s = {}
    for k in ("Contrast2012", "Highlights2012", "Shadows2012", "Whites2012", "Blacks2012"):
        s[k] = f["light"].get(k, 0)
    s["Vibrance"] = f.get("color", {}).get("Vibrance", 0)
    s["Saturation"] = f.get("color", {}).get("Saturation", 0)
    s["Clarity2012"] = f.get("clarity", 0)
    s["ConvertToGrayscale"] = "True" if f.get("bw") else "False"
    for b in BANDS:
        h, sa, lu = f.get("hsl", {}).get(b, (0, 0, 0))
        s[f"HueAdjustment{b}"] = h
        s[f"SaturationAdjustment{b}"] = sa
        s[f"LuminanceAdjustment{b}"] = lu
    if f.get("bw"):
        for b in BANDS:
            s[f"GrayMixer{b}"] = f["mix"].get(b, 0)
    g = f.get("grade", dict(sh=(0, 0), hl=(0, 0)))
    s["SplitToningShadowHue"], s["SplitToningShadowSaturation"] = g["sh"]
    s["SplitToningHighlightHue"], s["SplitToningHighlightSaturation"] = g["hl"]
    s["SplitToningBalance"] = 0
    s["ColorGradeBlending"] = 50
    s["PostCropVignetteAmount"] = f.get("vignette", 0)
    s["PostCropVignetteStyle"] = 1
    s["PostCropVignetteMidpoint"] = 50
    s["PostCropVignetteFeather"] = 60
    s["PostCropVignetteRoundness"] = 0
    a, sz, fr = f["grain"]
    s["GrainAmount"], s["GrainSize"], s["GrainFrequency"] = a, sz, fr
    return s


def xmp(f, idx):
    # identificativo fisso (mai mostrato): reimportando, i preset si aggiornano invece di duplicarsi
    uid = uuid.uuid5(uuid.NAMESPACE_URL, "iride-fw/film/" + full_name(f)).hex.upper()
    attrs = [
        ('PresetType', 'Normal'), ('Cluster', ''), ('UUID', uid),
        ('SupportsAmount', 'True'), ('SupportsColor', 'True'), ('SupportsMonochrome', 'True'),
        ('SupportsHighDynamicRange', 'True'), ('SupportsNormalDynamicRange', 'True'),
        ('SupportsSceneReferred', 'True'), ('SupportsOutputReferred', 'True'),
        ('CameraModelRestriction', ''), ('Copyright', 'GPLv2 - Iride 6D'), ('ContactInfo', ''),
        ('Version', '15.4'), ('ProcessVersion', '11.0'),
    ]
    for k, v in settings(f).items():
        attrs.append((k, v if isinstance(v, str) else fmt(k, v)))
    attrs += [('ToneCurveName2012', 'Custom'), ('HasSettings', 'True')]
    head = ('<x:xmpmeta xmlns:x="adobe:ns:meta/" x:xmptk="Adobe XMP Core 7.0-c000 1.000000, 0000/00/00-00:00:00        ">\n'
            ' <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">\n'
            '  <rdf:Description rdf:about=""\n'
            '    xmlns:crs="http://ns.adobe.com/camera-raw-settings/1.0/"\n')
    body = "".join(f'   crs:{k}="{escape(str(v))}"\n' for k, v in attrs[:-1])
    body += f'   crs:{attrs[-1][0]}="{attrs[-1][1]}">\n'
    kids = alt("Name", full_name(f))
    kids += alt("ShortName", "")
    kids += alt("SortName", f"{idx + 1:02d} {full_name(f)}")
    kids += alt("Group", GROUP)
    kids += alt("Description", f"{f['desc']}. {f['use']} (ISO {f['iso']})")
    kids += seq("ToneCurvePV2012", f["curve"])
    for ch in ("Red", "Green", "Blue"):
        kids += seq(f"ToneCurvePV2012{ch}", IDENT)
    tail = "  </rdf:Description>\n </rdf:RDF>\n</x:xmpmeta>\n"
    return head + body + kids + tail


def fname(f, idx):
    safe = full_name(f).replace("/", "-")
    return f"Iride {idx + 1:02d} - {safe}.xmp"


# ------------------------------------------------------------------ ricette fatte sulla 6D (ML/IRIDE/MIAn.TXT)

GRAIN = [(0, 0, 0), (15, 20, 40), (30, 28, 50), (50, 40, 60)]   # nessuna, leggera, media, forte
VIGNETTE = [0, -10, -20, -35]


def read_card(path):
    """Scheda di una ricetta: righe "chiave: valore"."""
    with open(path, encoding="utf-8", errors="replace") as fh:
        lines = fh.read().replace("\r", "").split("\n")
    if not lines or not lines[0].startswith("IRIDE RICETTA"):
        raise ValueError(f"{path}: non e' una scheda di Iride")
    card = {}
    for ln in lines[1:]:
        if ":" in ln:
            k, v = ln.split(":", 1)
            card[k.strip().lower()] = v.strip()
    return card


def _int(card, key, default=0, lo=-99999, hi=99999):
    try:
        return max(lo, min(hi, int(card.get(key, default))))
    except ValueError:
        return default


def recipe_film(card):
    """Pellicola di partenza + modifiche della ricetta, nel formato di films.py."""
    import copy
    base_txt = card.get("base", "").lower()
    matches = [f for f in FILMS if f["name"].lower() in base_txt]
    if not matches:
        raise ValueError(f"pellicola di partenza sconosciuta: {card.get('base')!r}")
    base = max(matches, key=lambda f: len(f["name"]))
    f = copy.deepcopy(base)
    c, s, t = _int(card, "contrasto", 0, -4, 4), _int(card, "saturazione", 0, -4, 4), _int(card, "tonalita", 0, -4, 4)
    f["light"]["Contrast2012"] = f["light"].get("Contrast2012", 0) + 8 * c
    col = f.setdefault("color", {})
    col["Saturation"] = max(-100, min(100, col.get("Saturation", 0) + 8 * s))
    if t:   # tonalita' Canon: + verso il giallo, - verso il rosso (pelle)
        hsl = f.setdefault("hsl", {})
        for band, k in (("Red", 3), ("Orange", 4)):
            h, sa, lu = hsl.get(band, (0, 0, 0))
            hsl[band] = (h + k * t, sa, lu)
    if card.get("stile", "").lower().startswith("monocromo") and not f.get("bw"):
        f["bw"] = True
        f["mix"] = {}
    f["grain"] = GRAIN[_int(card, "grana", 0, 0, 3)]
    f["vignette"] = VIGNETTE[_int(card, "vignetta", 0, 0, 3)]
    nome = card.get("nome", "MIA")
    f["brand"] = f"Iride {nome}"
    f["name"] = f"da {base['brand']} {base['name']}"
    f["iso"] = _int(card, "iso", base["iso"], 50, 25600)
    f["desc"] = f"Ricetta {nome}: contrasto {c:+d}, saturazione {s:+d}, tonalita' {t:+d}"
    f["use"] = f"partita da {base['brand']} {base['name']}"
    return f


def recipe_main(cards, out):
    os.makedirs(out, exist_ok=True)
    for i, path in enumerate(cards):
        f = recipe_film(read_card(path))
        stem = os.path.splitext(os.path.basename(path))[0].upper()
        p = os.path.join(out, f"Iride {stem} - {f['name'].replace('/', '-')}.xmp")
        with open(p, "w", encoding="utf-8") as fh:
            fh.write(xmp(f, 50 + i))
        print(p)


def main():
    if "--ricetta" in sys.argv:
        args = [a for a in sys.argv[1:] if a != "--ricetta"]
        if len(args) < 2:
            sys.exit("Uso: make_presets.py --ricetta MIA1.TXT [MIA2.TXT ...] CARTELLA")
        recipe_main(args[:-1], args[-1])
        return
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    files = []
    for i, f in enumerate(FILMS):
        p = os.path.join(out, fname(f, i))
        with open(p, "w", encoding="utf-8") as fh:
            fh.write(xmp(f, i))
        files.append(p)
    zp = os.path.join(out, "Iride6D-preset-pellicole.zip")
    with zipfile.ZipFile(zp, "w", zipfile.ZIP_DEFLATED) as z:
        for p in files:
            z.write(p, os.path.basename(p))
    print(f"{len(files)} preset -> {zp}")


if __name__ == "__main__":
    main()
