# Iride 6D

**Firmware personalizzato per Canon EOS 6D** (firmware Canon 1.1.6), costruito come modulo di [Magic Lantern](https://www.magiclantern.fm/).
Un'interfaccia a tutto schermo, animata, con menu dedicati ad **astrofotografia**, **foto**, **rullini** in stile pellicola e **profili**, più un tasto che riporta la macchina com'era.

*English: custom full-screen UI for the Canon EOS 6D (fw 1.1.6) running on Magic Lantern — astrophotography tools (NPF with declination, field intervalometer with silent shutter, dithering pauses and CSV session log, HFR star focus with V-curve autofocus, polar alignment, sky calendar, Moon exposure, Milky Way planner, worldwide cities and time zones), photo tools (HDR, focus stacking, Dual ISO, ETTR, ND), film-roll simulation and user profiles. UI in Italian.*

> ⚠️ **Software sperimentale.** Prime prove su una 6D reale: rullini e nomi dei file funzionano. Il resto è verificato su un simulatore che esegue lo stesso codice (148 test) ed è compilato contro il firmware 6D 1.1.6. Usalo a tuo rischio. Non è affiliato né approvato da Canon.

| | | |
|---|---|---|
<p align="center"><img src="docs/screenshots/animazioni-v13.gif" width="540" alt="Avvio, Home, scaffale dei rullini e dialogo"></p>

| ![Home](docs/screenshots/01_home-v13.png) | ![Astro](docs/screenshots/02_astro-v13.png) | ![Rullini](docs/screenshots/14_rullini-v13.png) |
| ![Allineamento polare](docs/screenshots/06_polare-v13.png) | ![Calendario del cielo](docs/screenshots/07_cielo-v13.png) | ![Tema notte](docs/screenshots/21_cielo_notte-v13.png) |
| ![Fuoco stelle](docs/screenshots/05_stelle-v13.png) | ![Foto](docs/screenshots/08_foto-v13.png) | ![Torna alla 6D](docs/screenshots/19_torna_6d-v13.png) |
| ![Fuoco automatico](docs/screenshots/05b_fuoco_auto-v13.png) | ![Luna da fotografare](docs/screenshots/25_luna_foto-v13.png) | ![Dove puntare la Via Lattea](docs/screenshots/26_dove_puntare-v13.png) |
| ![Intervallometro da campo](docs/screenshots/04_intervallometro-v13.png) | ![Luogo e fuso](docs/screenshots/18d_opzioni_luogo-v13.png) | ![Aiuto con INFO](docs/screenshots/05c_aiuto-v13.png) |
| ![Live View al buio](docs/screenshots/11b_lv_al_buio-v13.png) | ![Luce cinerea](docs/screenshots/25b_luna_cinerea-v13.png) | ![Prova della 6D](docs/screenshots/24_prova-v13.png) |
| ![Profili](docs/screenshots/17_profili-v13.png) | ![Opzioni](docs/screenshots/18_opzioni-v13.png) | ![Filtri ND](docs/screenshots/12_filtri_nd-v13.png) |

<p align="center"><img src="docs/screenshots/menu-v13.gif" width="540" alt="Menu Astro con l'anteprima della voce scelta"></p>

## Funzioni

| Area | Cosa fa |
|---|---|
| 🌌 **Astro** | Via Lattea con regola NPF e punto del cielo (applica tempo, diaframma, ISO, bianco) · intervallometro con preset Via Lattea, Deep sky, Star trails, Timelapse, Giorno-notte, Dark, Flat, Bias, con otturatore silenzioso, pausa per il dithering, controllo di batteria, scheda e temperatura e registro della sessione in CSV (lettore per AstroStack in [tools/astrostack](tools/astrostack/)) · fuoco sulle stelle con misura HFR in Live View e fuoco automatico a curva a V · Luna da fotografare (regola Looney 11 con fase e altezza) · dove puntare la Via Lattea ora per ora · allineamento polare su reticolo con mappa del cielo a nord (Grande Carro e Cassiopea) · calendario di 7 notti con linea del tempo della notte (crepuscoli, Luna, centro della Via Lattea), stelle visibili col cielo buio e con la Luna, quando torna il centro della Via Lattea e notte migliore · nomi file LGHT/DARK/FLAT/BIAS per lo stacking |
| 📷 **Foto** | Bracketing HDR · focus stacking · Dual ISO · ETTR · scatto su movimento e trappola di fuoco · strumenti del Live View (peaking, zebra, istogramma, falsi colori, zoom, spot) · filtri ND con posa B automatica |
| 🎞️ **Rullini** | 16 pellicole (colore, dia, B/N, cine) · push/pull · 12/24/36 pose · pausa e ripresa · contatore nella barra del Live View · registro su scheda · avviso se la macchina è in solo RAW · [preset per LightCraft e Lightroom](presets/) |
| 💾 **Profili** | 5 posti per le impostazioni preferite |
| 🔁 **Torna alla 6D** | Rimette tutto com'era prima di Iride: stile immagine e cursori, ISO, tempo, diaframma, bianco, anteprima, impostazioni di Magic Lantern, nomi dei file |
| 🌍 **Luogo** | 57 città pronte, latitudine e longitudine libere, fusi orari di tutto il mondo con l'ora legale automatica (controllati col database IANA) |
| ✨ **Interfaccia** | Carattere piccolo proporzionale con i bordi sfumati (al posto del Terminus di ML) · angoli arrotondati su pannelli, schede, pulsanti e interruttori · icone disegnate per le liste di Astro e Foto · aiuto col tasto INFO in ogni schermata · Sottomenu con anteprima della voce scelta (icona animata, a cosa serve, com'è adesso), percorso nell'intestazione, schede con i conti già fatti, Opzioni in sezioni · animazioni senza sfarfallio (doppio buffer): diaframma all'avvio, riquadri della Home che entrano e icone che si animano, zoom dal riquadro alla schermata, dialoghi che salgono come fogli, barra di selezione elastica, interruttori e valori che scorrono, avvisi che entrano e si spengono · caricamento e riavvolgimento del rullino con contapose a rullo · tema notte rosso, compare all'accensione, si apre col cestino (o col tasto luce) · diagnostica dei tasti |

### Tasto luce

La 6D riceve il tasto luce del display superiore come evento `0x20`, e Magic Lantern lo usa nella riproduzione. Non è ancora confermato che lo invii anche in modalità di scatto. **Opzioni → Diagnostica tasti** mostra gli ultimi tasti ricevuti e li salva in `ML/LOGS/IRIDEKEY.TXT`.

### Rullini

<p align="center"><img src="docs/screenshots/rullini_tutti-v06.png" width="720" alt="I 16 rullini"></p>

Ogni rullino è disegnato in macchina, con grafiche originali: perno, coperchi zigrinati, cilindro ombreggiato, fessura di velluto, codice DX e codice a barre.
Sotto lo scaffale due righe per pellicola: il carattere e l'uso ideale (verificati su schede tecniche e prove: per esempio la Delta 3200 è una ISO 1000 reale esposta a 3200, la CineStill 800T è una pellicola cinema al tungsteno che di giorno vira al blu).

<p align="center"><img src="docs/screenshots/caricamento-v13.gif" width="540" alt="Caricamento di un rullino"></p>
<p align="center"><img src="docs/screenshots/riavvolgimento-v13.gif" width="540" alt="Riavvolgimento con la manovella"></p>

### Rullini e RAW

Il look della pellicola la 6D lo applica solo al **JPEG**: con la qualità in solo RAW, LightCraft e Lightroom mostrano un attimo l'anteprima con il look e poi il RAW neutro.
Due soluzioni: scattare in **RAW+L** (Iride avvisa se sei in solo RAW, ma non cambia la qualità da solo: Magic Lantern lo considera rischioso) oppure usare i **[16 preset XMP](presets/)**, uno per pellicola, che danno lo stesso carattere al RAW.

I calcoli astronomici girano in macchina, senza internet, e sono confrontati con [astropy](https://www.astropy.org/): Polare entro 0,4 s di angolo orario, Sole entro 0,004°, Luna entro 0,24°, orari di tramonto e buio al minuto, sorgere e tramonto della Luna entro 1 minuto.

## Installazione

1. La 6D deve avere il **firmware Canon 1.1.6** (vedi [Magic Lantern – 6D](https://builds.magiclantern.fm/6D.html)). Il firmware Canon non è incluso in questo repository.
2. Scarica `Iride6D-<versione>-SD.zip` dalla pagina **Releases** (per aggiornare basta `iride.mo` in `ML/modules`).
3. Formatta la scheda nella 6D e copia tutto il contenuto dello zip nella radice.
4. Ghiera su M, Menu → Versione firmware → aggiorna: parte l'installatore di Magic Lantern.
5. All'accensione compare la Home di Iride (si può spegnere in Opzioni). Poi si apre col **cestino**: dentro Iride il cestino porta al menu di Magic Lantern, un altro cestino lo chiude. Si può usare anche il tasto luce del display superiore (vedi «Tasto luce» qui sopra).

Istruzioni complete in [docs/LEGGIMI.txt](docs/LEGGIMI.txt). Se la macchina si blocca: togli e rimetti la batteria. Magic Lantern gira dalla scheda e non modifica la ROM.

## Compilare

```
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi python3-docutils zip
git clone https://github.com/reticulatedpines/magiclantern_simplified ml
cd ml && git checkout ed3e7c0dfacfdeaf3349dee73edf9e3621217462   # build ufficiale 6D, 20/06/2025
cp -r ../module/iride modules/
git apply ../module/magiclantern.patch
cd platform/6D.116 && make -j8      # -> build/magiclantern.zip
```

La compilazione e i test girano anche in automatico con GitHub Actions a ogni modifica (`.github/workflows/build.yml`). Pubblicando una **Release** con tag `vX.Y` (dal sito: Releases → Draft a new release, oppure `git tag v0.5 && git push origin v0.5`) GitHub Actions ci aggiunge da solo pacchetto SD, `iride.mo` e preset pellicola, con le note prese dal CHANGELOG.

## Simulatore e test

`simulator/` esegue il codice reale di Iride su PC, al posto delle funzioni di Magic Lantern:

```
cd simulator && mkdir -p out
gcc -std=gnu99 -Ishim -o preview preview.c -lm && ./preview   # 148 test
pip install astropy pillow
python3 check_astro.py   # confronto astronomico con astropy
python3 check_tz.py      # fusi orari a confronto col database IANA
python3 render.py        # schermate in png/
python3 gif.py ../docs/screenshots   # GIF del README (caricamento, animazioni)
```

## Struttura

```
module/iride/       il modulo (iride.c include ui, moon, icons, store, film, campo, astro, sky2, photo, bench, screens)
tools/astrostack/   lettore delle sessioni CSV per AstroStack
module/*.patch      aggiunge iride ai moduli e al pacchetto della 6D
simulator/          test su PC e generatore delle schermate
presets/            preset pellicola XMP per LightCraft e Lightroom (e il loro generatore)
docs/               istruzioni e schermate
```

## Crediti e licenza

- Basato su [Magic Lantern](https://www.magiclantern.fm/) ([sorgenti](https://github.com/reticulatedpines/magiclantern_simplified)), GPL.
- Formula NPF: [Lightstalking](https://www.lightstalking.com/npf-rule/). Le funzioni Astro riprendono l'app Android Iride dello stesso autore.
- Licenza **GPL v2**, come Magic Lantern: vedi [LICENSE](LICENSE).

Canon ed EOS sono marchi di Canon Inc. Questo progetto non è affiliato a Canon.
