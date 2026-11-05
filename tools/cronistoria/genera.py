#!/usr/bin/env python3
"""Rigenera i dati della cronistoria (site/cronistoria-data.js) da CHANGELOG.md.

Le versioni fino alla 1.6 sono raccontate a mano in curata.json (errori, rifacimenti, correzioni).
Ogni versione nuova del CHANGELOG si aggiunge da sola: titolo dalla prima frase in grassetto, una riga
per punto, e il tipo dalle parole ("Corretto" = errore corretto, "ridisegnato/rifatto" = rifatto...).
I numeri (versioni, test, righe del modulo) si contano qui.

GitHub Actions lo esegue a ogni pubblicazione del sito (.github/workflows/pages.yml).
Uso a mano: python3 tools/cronistoria/genera.py
"""
import datetime
import json
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
MESI = ["gennaio", "febbraio", "marzo", "aprile", "maggio", "giugno", "luglio", "agosto",
        "settembre", "ottobre", "novembre", "dicembre"]
GIORNI = ["lunedì", "martedì", "mercoledì", "giovedì", "venerdì", "sabato", "domenica"]


def versioni_changelog():
    """[(versione, data, [punti])] dal CHANGELOG, dalla piu' recente."""
    out, cur = [], None
    for line in open(os.path.join(ROOT, "CHANGELOG.md"), encoding="utf-8"):
        m = re.match(r"^## (\S+)\s+[–-]\s+(\d+) (\w+) (\d{4})", line)
        if m:
            d = datetime.date(int(m.group(4)), MESI.index(m.group(3).lower()) + 1, int(m.group(2)))
            cur = [m.group(1), d, []]
            out.append(cur)
        elif cur is not None and line.startswith("- "):
            cur[2].append(line[2:].strip())
    return out


def pulito(md):
    """Testo semplice da una riga Markdown: niente link, grassetti, corsivi e codice."""
    t = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", md)
    t = re.sub(r"\*\*|__|`", "", t)
    t = re.sub(r"(?<!\w)\*([^*]+)\*(?!\w)", r"\1", t)
    return t.strip()


def prima_frase(t, n=170):
    t = re.split(r"(?<=[.;])\s|: (?=[A-Z])", t, maxsplit=1)[0].rstrip(".;:")
    return t if len(t) <= n else t[:n].rsplit(" ", 1)[0] + "…"


def tipo(md):
    t = md.lower()
    if re.match(r"\**corrett|\**corretto|\**risolt", t) or "non funzionava" in t:
        return "fix"
    if re.search(r"ridisegnat|rifatt|riscritt|rifatta da capo", t):
        return "redo"
    if re.match(r"\**(nuov|aggiunt)", t) or re.search(r"\*\*[^*]+\*\*", md):
        return "ok"
    return "mod"


def titolo(punti):
    for p in punti:
        m = re.search(r"\*\*([^*]+)\*\*", p)
        if m:
            return pulito(m.group(1)).rstrip(":")
    return ""


def test_totali(punti, prima):
    """Totale dei test: "Simulatore: 148 test" oppure "30 test nuovi (211)"."""
    for p in punti:
        m = re.search(r"test nuovi \((\d+)\)", p) or re.search(r"Simulatore:\s*(\d+) test(?! nuov)", p)
        if m:
            return int(m.group(1))
        m = re.search(r"(\d+) test nuovi", p)
        if m and prima is not None:
            return prima + int(m.group(1))
    return None


def quando(d):
    return f"{d.day} {MESI[d.month - 1]} · {GIORNI[d.weekday()]}"


def righe_modulo():
    tot = 0
    d = os.path.join(ROOT, "module", "iride")
    for f in os.listdir(d):
        if f.endswith((".c", ".inc")):
            tot += sum(1 for _ in open(os.path.join(d, f), encoding="utf-8", errors="replace"))
    return tot


def main():
    fasi = json.load(open(os.path.join(HERE, "curata.json"), encoding="utf-8"))
    note = {v["v"] for f in fasi for v in f["vers"]}
    ultimo_test = max(v["tests"] for f in fasi for v in f["vers"] if v.get("tests") is not None)
    nuove = [x for x in reversed(versioni_changelog()) if x[0] not in note]
    for v, d, punti in nuove:
        tests = test_totali(punti, ultimo_test)
        if tests is not None:
            ultimo_test = tests
        voci = [[tipo(p), prima_frase(pulito(p))] for p in punti if pulito(p)]
        ver = {"v": v, "t": titolo(punti), "tests": tests, "items": voci, "auto": True}
        # stessa giornata dell'ultima fase automatica: si aggiunge li'
        if fasi and fasi[-1].get("auto") and fasi[-1]["data"] == d.isoformat():
            fasi[-1]["vers"].append(ver)
        else:
            fasi.append({"when": quando(d), "title": ver["t"] or f"Versione {v}", "vers": [ver], "auto": True, "data": d.isoformat()})
    versioni = len(versioni_changelog())
    dati = {
        "fasi": fasi,
        "versioni": versioni,
        "prima": versioni_changelog()[-1][0],
        "ultima": versioni_changelog()[0][0],
        "giorni": (versioni_changelog()[0][1] - versioni_changelog()[-1][1]).days + 1,
        "righe": righe_modulo(),
        "test": ultimo_test,
    }
    out = os.path.join(ROOT, "site", "cronistoria-data.js")
    with open(out, "w", encoding="utf-8") as fh:
        fh.write("/* Generato da tools/cronistoria/genera.py: non modificare a mano */\n")
        fh.write("window.CRONISTORIA = " + json.dumps(dati, ensure_ascii=False, indent=0) + ";\n")
    print(f"{versioni} versioni, {len(nuove)} aggiunte dal CHANGELOG, {ultimo_test} test, {dati['righe']} righe -> {out}")


if __name__ == "__main__":
    main()
