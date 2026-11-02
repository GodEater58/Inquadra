#!/usr/bin/env python3
"""Controlla i formati di printf nel modulo.

Sulla 6D snprintf e my_fprintf usano il vsnprintf di Canon, che per "%%" stampa "%"
ma consuma anche un argomento: tutti i valori dopo finiscono spostati di uno
(trovato con la Prova della 6D: "temperatura 30387 gradi", in realta' i MB liberi).
Un "%%" in fondo al formato non fa danni; prima di un altro valore si'.
Al suo posto: "%s" con l'argomento "%"."""
import glob
import os
import re
import sys

LIT = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
CONV = re.compile(r'%(%|[-+ #0]*[0-9*]*(?:\.[0-9*]+)?(?:hh|h|ll|l|z)?[diouxXcsfeEgGp])')


def literals(src):
    """Stringhe C, unendo quelle scritte una dopo l'altra (solo spazi in mezzo)."""
    out, last_end, cur, line = [], None, None, 1
    for m in LIT.finditer(src):
        between = src[last_end:m.start()] if last_end is not None else None
        if cur is not None and between is not None and between.strip() == "":
            cur[0] += m.group(1)
        else:
            if cur is not None:
                out.append(cur)
            cur = [m.group(1), src.count("\n", 0, m.start()) + 1]
        last_end = m.end()
    if cur is not None:
        out.append(cur)
    return out


def main(root):
    bad = 0
    files = sorted(glob.glob(os.path.join(root, "*.inc")) + glob.glob(os.path.join(root, "*.c")))
    for path in files:
        src = open(path, encoding="utf-8", errors="replace").read()
        for text, line in literals(src):
            seen_pct = False
            for c in CONV.finditer(text):
                if c.group(1) == "%":
                    seen_pct = True
                elif seen_pct:
                    print(f"{os.path.basename(path)}:{line}: \"%%\" prima di un altro valore: \"{text}\"")
                    bad += 1
                    break
    print(f"formati controllati in {len(files)} file: {bad} da correggere")
    return 1 if bad else 0


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "module", "iride")))
