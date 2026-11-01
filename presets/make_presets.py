#!/usr/bin/env python3
"""Genera i preset XMP (crs:) delle pellicole di Iride 6D e un'anteprima indicativa.

Uso: make_presets.py OUT_DIR [IMMAGINE ...]
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
    kids += alt("Description", f"{f['desc']} (ISO {f['iso']})")
    kids += seq("ToneCurvePV2012", f["curve"])
    for ch in ("Red", "Green", "Blue"):
        kids += seq(f"ToneCurvePV2012{ch}", IDENT)
    tail = "  </rdf:Description>\n </rdf:RDF>\n</x:xmpmeta>\n"
    return head + body + kids + tail


def fname(f, idx):
    safe = full_name(f).replace("/", "-")
    return f"Iride {idx + 1:02d} - {safe}.xmp"


def main():
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
