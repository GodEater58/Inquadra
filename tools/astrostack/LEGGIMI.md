# Iride 6D → AstroStack

Iride scrive un registro per ogni notte nella cartella delle foto (`DCIM/100CANON/IS261102.CSV` vuol dire la notte che inizia la sera del 2 novembre 2026: fino a mezzogiorno del giorno dopo si scrive lì). Fino alla 1.4.1 il nome era `ISmmddhh.CSV`, un file per ora: il lettore li legge entrambi. Ogni riga è uno scatto: tipo (LGHT, DARK, FLAT, BIAS), posa, ISO, diaframma, focale, temperatura interna della 6D, batteria, Luna e magnitudine limite stimata, otturatore e origine (intervallometro di Iride o cavo, per esempio dalla montatura).

`iride_session.py` legge questi file in AstroStack:

| Funzione | A cosa serve |
|---|---|
| `load_folder(cartella)` | nome del file → dati dello scatto (anche per i JPEG di RAW+JPEG) |
| `darks_for(light, scatti)` | i dark adatti a una light: stessa posa e ISO, temperatura entro 3 gradi |
| `abbina_silenziosi(dng, sessioni)` | abbina i DNG degli scatti silenziosi alle righe del registro, in ordine |
| `riepilogo(sessione)` | "IS261102.CSV: 120 LGHT, 30 DARK, 20-24 gradi" |
| `piano_stack(cartella)` | gli stack già pronti: light con la stessa posa, ISO e focale, con dark alla stessa temperatura, flat della stessa focale, bias dello stesso ISO e gli avvisi su quello che manca |
| `gruppo.migliori(0.9)` | il 90 % delle light col cielo migliore (magnitudine limite alta, Luna bassa) |

## Dove collegarlo in AstroStack

- **sorter**: se c'è un registro, il tipo dello scatto viene da lì invece che dal nome del file.
- **master_library**: per ogni gruppo di light, usare `darks_for` per scegliere i dark alla stessa temperatura.
- **frames_table**: colonne in più con temperatura, Luna e magnitudine limite, utili per scartare i frame peggiori.

Senza registro AstroStack continua a funzionare come prima.

Prova veloce: `python iride_session.py cartella_delle_foto` (con `--json` il piano di stacking per AstroStack).

### Esempio in AstroStack

```python
from iride_session import piano_stack

for g in piano_stack(cartella):
    lights = [os.path.join(cartella, f.file) for f in g.migliori(0.9)]
    darks = [os.path.join(cartella, f.file) for f in g.dark]
    for a in g.avvisi:
        log.warning(a)              # "nessun flat a questa focale", "solo 6 dark"...
    pipeline.stack(lights, darks=darks)
```

I dark fatti da Iride a fine sessione (1.6) hanno già posa, ISO e temperatura giuste: `piano_stack` li abbina da solo.
