"""Lettore dei registri di sessione di Iride 6D, per AstroStack.

Iride scrive nella cartella delle foto (DCIM/xxxCANON) un file ISaammgg.CSV per ogni notte
(dalla 1.4.2; prima ISmmddhh.CSV, uno per ora: si leggono tutti e due):
righe di commento con luogo, cielo e obiettivo, poi una riga per scatto con tipo (LGHT, DARK,
FLAT, BIAS), posa, ISO, diaframma, focale, temperatura interna, batteria, Luna e magnitudine
limite stimata, otturatore (meccanico o silenzioso) e origine (intervallometro o cavo).

Uso tipico in AstroStack:

    from iride_session import load_folder, darks_for

    info = load_folder("D:/Foto/2026-11-02")       # nome file -> Frame
    f = info.get("LGHT1246.CR2")
    if f:
        print(f.kind, f.exposure_s, f.iso, f.temp_c)
    buoni = darks_for(f, info.values())             # dark con stessa posa/ISO e temperatura vicina

Funziona con Python 3.8 e successivi, senza dipendenze.
"""

from __future__ import annotations

import csv
import io
import os
import re
from dataclasses import dataclass, field
from typing import Dict, Iterable, List, Optional

KINDS = ("LGHT", "DARK", "FLAT", "BIAS")


@dataclass
class Frame:
    """Uno scatto del registro."""
    number: int
    file: str                 # nome del file sulla scheda, oppure "silenzioso"
    kind: str                 # LGHT, DARK, FLAT o BIAS
    time: str                 # ora locale hh:mm:ss
    exposure_s: float
    iso: int
    aperture: Optional[float]
    focal_mm: int
    temp_c: int               # temperatura interna della 6D (segue quella del sensore)
    battery: int              # percentuale, -1 se non nota
    moon_pct: int
    moon_alt: float
    nelm: float               # magnitudine limite stimata allo zenit
    shutter: str              # meccanico o silenzioso
    origin: str               # intervallometro o cavo
    session: "Session" = field(repr=False, default=None)


@dataclass
class Session:
    """Un file di sessione."""
    path: str
    meta: Dict[str, str]
    frames: List[Frame]

    def by_kind(self, kind: str) -> List[Frame]:
        return [f for f in self.frames if f.kind == kind]

    def temperature_range(self) -> Optional[tuple]:
        t = [f.temp_c for f in self.frames]
        return (min(t), max(t)) if t else None


def _num(v: str, cast=float, default=None):
    try:
        return cast(v)
    except (TypeError, ValueError):
        return default


def read_session(path: str) -> Session:
    """Legge un registro di Iride (ISaammgg.CSV o il vecchio ISmmddhh.CSV)."""
    with open(path, "r", encoding="ascii", errors="replace") as fh:
        text = fh.read()
    meta: Dict[str, str] = {}
    body = []
    for line in text.splitlines():
        if line.startswith("#"):
            m = re.match(r"#\s*Iride 6D (\S+) - sessione del (\S+)", line)
            if m:
                meta["versione"], meta["data"] = m.group(1), m.group(2)
            m = re.match(r"#\s*luogo (.+?), cielo del luogo ([\d.]+)", line)
            if m:
                meta["luogo"], meta["cielo"] = m.group(1), m.group(2)
            m = re.match(r"#\s*obiettivo (.+)", line)
            if m:
                meta["obiettivo"] = m.group(1).strip()
        elif line.strip():
            body.append(line)
    sess = Session(path=path, meta=meta, frames=[])
    for row in csv.DictReader(io.StringIO("\n".join(body))):
        f = Frame(
            number=_num(row.get("numero"), int, 0),
            file=row.get("file", ""),
            kind=row.get("tipo", "LGHT"),
            time=row.get("ora", ""),
            exposure_s=_num(row.get("posa_s"), float, 0.0),
            iso=_num(row.get("iso"), int, 0),
            aperture=_num(row.get("diaframma"), float, None),
            focal_mm=_num(row.get("focale_mm"), int, 0),
            temp_c=_num(row.get("temperatura_c"), int, 0),
            battery=_num(row.get("batteria"), int, -1),
            moon_pct=_num(row.get("luna_pct"), int, 0),
            moon_alt=_num(row.get("luna_alt"), float, 0.0),
            nelm=_num(row.get("mag_limite"), float, 0.0),
            shutter=row.get("otturatore", ""),
            origin=row.get("origine", ""),
            session=sess,
        )
        sess.frames.append(f)
    return sess


def find_sessions(folder: str) -> List[Session]:
    """Tutti i registri di Iride in una cartella (anche nelle sottocartelle)."""
    out = []
    for root, _dirs, files in os.walk(folder):
        for name in sorted(files):
            if re.fullmatch(r"IS\d{6}\.CSV", name, flags=re.IGNORECASE):
                out.append(read_session(os.path.join(root, name)))
    return out


def load_folder(folder: str) -> Dict[str, Frame]:
    """Nome del file (maiuscolo) -> dati dello scatto, per tutte le sessioni della cartella.

    Gli scatti silenziosi non hanno un nome Canon: si abbinano per ordine con abbina_silenziosi().
    """
    out: Dict[str, Frame] = {}
    for s in find_sessions(folder):
        for f in s.frames:
            if f.file and f.file != "silenzioso":
                out[f.file.upper()] = f
                # RAW+JPEG: stesso numero, l'altra estensione
                stem, _, ext = f.file.upper().rpartition(".")
                out.setdefault(stem + (".JPG" if ext == "CR2" else ".CR2"), f)
    return out


def abbina_silenziosi(dng_files: Iterable[str], sessions: Iterable[Session]) -> Dict[str, Frame]:
    """Abbina i DNG degli scatti silenziosi alle righe "silenzioso", nell'ordine in cui sono stati fatti."""
    dng = sorted(dng_files)
    rows = [f for s in sessions for f in s.frames if f.file == "silenzioso"]
    return {os.path.basename(d).upper(): r for d, r in zip(dng, rows)}


def darks_for(light: Frame, candidates: Iterable[Frame], max_dt: int = 3) -> List[Frame]:
    """Dark adatti a una light: stessa posa (entro il 2%) e ISO, temperatura entro max_dt gradi.

    Ordinati per differenza di temperatura, i piu' vicini prima.
    """
    if light is None:
        return []
    good, seen = [], set()
    for d in candidates:
        if id(d) in seen:                  # RAW+JPEG: la stessa riga compare due volte
            continue
        seen.add(id(d))
        if d.kind != "DARK" or d.iso != light.iso:
            continue
        if light.exposure_s and abs(d.exposure_s - light.exposure_s) > 0.02 * light.exposure_s:
            continue
        dt = abs(d.temp_c - light.temp_c)
        if dt <= max_dt:
            good.append((dt, d))
    return [d for _dt, d in sorted(good, key=lambda x: x[0])]


def riepilogo(s: Session) -> str:
    """Una riga leggibile per l'interfaccia di AstroStack."""
    parti = [f"{len(s.by_kind(k))} {k}" for k in KINDS if s.by_kind(k)]
    tr = s.temperature_range()
    t = f", {tr[0]}-{tr[1]} gradi" if tr else ""
    return f"{os.path.basename(s.path)}: " + ", ".join(parti) + t


if __name__ == "__main__":
    import sys
    for sess in find_sessions(sys.argv[1] if len(sys.argv) > 1 else "."):
        print(riepilogo(sess))
