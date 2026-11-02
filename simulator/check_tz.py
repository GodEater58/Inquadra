#!/usr/bin/env python3
"""Confronta i fusi orari di Iride (out/tz.csv, scritto dal simulatore) col database IANA
dei fusi orari, quello che usano computer e telefoni (modulo zoneinfo di Python).
Per ogni citta' dell'elenco e ogni giorno dal 2026 al 2028, alle 0, 4, 12 e 23."""
import csv
import sys
from datetime import datetime
from zoneinfo import ZoneInfo

ZONE = {
    "Aosta": "Europe/Rome", "Torino": "Europe/Rome", "Genova": "Europe/Rome", "Milano": "Europe/Rome",
    "Trento": "Europe/Rome", "Bolzano": "Europe/Rome", "Venezia": "Europe/Rome", "Padova": "Europe/Rome",
    "Rovigo": "Europe/Rome", "Ferrara": "Europe/Rome", "Verona": "Europe/Rome", "Trieste": "Europe/Rome",
    "Bologna": "Europe/Rome", "Firenze": "Europe/Rome", "Ancona": "Europe/Rome", "Perugia": "Europe/Rome",
    "Roma": "Europe/Rome", "L'Aquila": "Europe/Rome", "Campobasso": "Europe/Rome", "Napoli": "Europe/Rome",
    "Bari": "Europe/Rome", "Potenza": "Europe/Rome", "Catanzaro": "Europe/Rome", "Palermo": "Europe/Rome",
    "Cagliari": "Europe/Rome",
    "Reykjavik": "Atlantic/Reykjavik", "Lisbona": "Europe/Lisbon", "Londra": "Europe/London",
    "Madrid": "Europe/Madrid", "Tenerife": "Atlantic/Canary", "Parigi": "Europe/Paris", "Berlino": "Europe/Berlin",
    "Vienna": "Europe/Vienna", "Atene": "Europe/Athens", "Helsinki": "Europe/Helsinki", "Istanbul": "Europe/Istanbul",
    "Mosca": "Europe/Moscow", "Citta' del Capo": "Africa/Johannesburg", "Nairobi": "Africa/Nairobi",
    "Dubai": "Asia/Dubai", "Nuova Delhi": "Asia/Kolkata", "Kathmandu": "Asia/Kathmandu", "Bangkok": "Asia/Bangkok",
    "Pechino": "Asia/Shanghai", "Tokyo": "Asia/Tokyo", "Perth": "Australia/Perth", "Sydney": "Australia/Sydney",
    "Auckland": "Pacific/Auckland", "Honolulu": "Pacific/Honolulu", "Los Angeles": "America/Los_Angeles",
    "Phoenix": "America/Phoenix", "Denver": "America/Denver", "Citta' del Messico": "America/Mexico_City",
    "Chicago": "America/Chicago", "New York": "America/New_York", "Buenos Aires": "America/Argentina/Buenos_Aires",
    "San Paolo": "America/Sao_Paulo",
}


def main(path="out/tz.csv"):
    n = bad = 0
    seen = set()
    with open(path, newline="") as f:
        for name, y, m, d, h, off in csv.reader(f):
            seen.add(name)
            if name not in ZONE:
                print("citta' senza fuso IANA nel controllo:", name)
                bad += 1
                continue
            y, m, d, h, off = map(int, (y, m, d, h, off))
            want = datetime(y, m, d, h, tzinfo=ZoneInfo(ZONE[name])).utcoffset().total_seconds() / 60
            n += 1
            if int(want) != off:
                bad += 1
                if bad < 20:
                    print(f"{name} {y}-{m:02d}-{d:02d} {h:02d}:00  Iride {off:+d} min, IANA {int(want):+d} min")
    missing = set(ZONE) - seen
    if missing:
        print("nel controllo ma non in Iride:", ", ".join(sorted(missing)))
        bad += 1
    print(f"fusi orari: {n} confronti su {len(seen)} citta', {bad} differenze")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))
