#!/usr/bin/env python3
"""Rigenera module/iride/lang_en.inc da tools/i18n/en.py e dice cosa manca da tradurre.

Uso: python3 tools/i18n/gen.py [--mancanti]
"""
import codecs
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', '..', 'module', 'iride')
sys.path.insert(0, HERE)
from en import EN  # noqa: E402

SKIP_FILES = {'font_small.inc', 'icons_small.inc', 'bench.inc', 'lang_en.inc'}
LIT = r'"(?:[^"\\\n]|\\.)*"'
SEQ = re.compile(r'(%s)((?:\s*(?:/\*.*?\*/)?\s*%s)*)' % (LIT, LIT), re.S)
SPEC = re.compile(r'%[-+ #0]*\d*(?:\.\d+)?[a-zA-Z%]')


def literals():
    """Stringhe del codice (le letterali vicine unite, come fa il compilatore)."""
    out = set()
    for name in sorted(os.listdir(SRC)):
        if not (name.endswith('.inc') or name.endswith('.c')) or name in SKIP_FILES:
            continue
        text = open(os.path.join(SRC, name)).read()
        for m in SEQ.finditer(text):
            s = ''.join(p[1:-1] for p in re.findall(LIT, m.group(0)))
            if re.search(r'[A-Za-z]', s):
                out.add(s)
    return out


def main():
    now = literals()
    rows, bad = [], []
    for k, v in EN.items():
        if SPEC.findall(k) != SPEC.findall(v):
            bad.append(k)
            continue
        rows.append((codecs.decode(k, 'unicode_escape').encode('latin-1'), k, v))
    rows.sort()
    if bad:
        sys.exit('formati diversi (%d e %s devono restare uguali e in ordine):\n  ' + '\n  '.join(bad))
    with open(os.path.join(SRC, 'lang_en.inc'), 'w') as fh:
        fh.write('/* lang_en.inc - Iride in inglese: testo italiano -> testo inglese.\n'
                 ' * Generato da tools/i18n/gen.py da tools/i18n/en.py: non modificare a mano.\n'
                 ' * Ordinato per strcmp: tr() cerca per bisezione. I formati tengono gli stessi %d/%s. */\n\n')
        fh.write('static const char *const lang_en[][2] = {\n')
        for _, k, v in rows:
            fh.write(f'    {{ "{k}", "{v}" }},\n')
        fh.write('};\n#define LANG_EN_N ((int)COUNT(lang_en))\n')
    print(f'{len(rows)} traduzioni in lang_en.inc')
    if '--mancanti' in sys.argv:
        for s in sorted(now - set(EN)):
            print('  da tradurre:', s)
        for s in sorted(set(EN) - now):
            print('  non nel codice (macro o lettera singola?):', s)


if __name__ == '__main__':
    main()
