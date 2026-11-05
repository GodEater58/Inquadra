# Iride 6D

[![Prova Iride nel browser](https://img.shields.io/badge/%E2%96%B6%20Prova%20Iride-nel%20browser-ff8c00?style=for-the-badge)](https://godeater58.github.io/Iride-6D/)
[![Release](https://img.shields.io/github/v/release/GodEater58/Iride-6D?style=for-the-badge&label=release&color=2a2a2f)](https://github.com/GodEater58/Iride-6D/releases/latest)
[![Build](https://img.shields.io/github/actions/workflow/status/GodEater58/Iride-6D/build.yml?style=for-the-badge&label=test%20e%20firmware)](https://github.com/GodEater58/Iride-6D/actions/workflows/build.yml)

**Firmware personalizzato per Canon EOS 6D** (firmware Canon 1.1.6): una versione di [Magic Lantern](https://www.magiclantern.fm/) modificata per Iride, più il modulo Iride.
Un'interfaccia a tutto schermo, animata, con menu dedicati ad **astrofotografia**, **foto**, **rullini** in stile pellicola e **profili**, più un tasto che riporta la macchina com'era.

*English: custom full-screen UI for the Canon EOS 6D (fw 1.1.6) built on a modified Magic Lantern (core patched for a full-screen module mode) — astrophotography tools (NPF with declination, field intervalometer with silent shutter, dithering pauses and CSV session log, HFR star focus with V-curve autofocus, polar alignment, sky calendar, Moon exposure, Milky Way planner, worldwide cities and time zones), photo tools (HDR, focus stacking, Dual ISO, ETTR, ND), film-roll simulation with your own recipes and user profiles. UI in Italian or English (Options › Lingua / Language).*

> ⚠️ **Software sperimentale.** Prime prove su una 6D reale: rullini e nomi dei file funzionano. Il resto è verificato su un simulatore che esegue lo stesso codice (211 test) ed è compilato contro il firmware 6D 1.1.6. Usalo a tuo rischio. Non è affiliato né approvato da Canon.

<p align="center"><a href="https://godeater58.github.io/Iride-6D/"><img src="docs/screenshots/animazioni-v16.gif" width="540" alt="Avvio, Home, scaffale dei rullini e dialogo"></a><br>
<b><a href="https://godeater58.github.io/Iride-6D/">▶ Provala nel browser</a></b>: è lo stesso codice che gira sulla 6D, con i caratteri veri di Magic Lantern. Si usa con la tastiera o coi tasti di una 6D disegnata.</p>

<details>
<summary><b>Tutte le schermate</b> (clicca per aprire)</summary>


| ![Home](docs/screenshots/01_home-v16.png) | ![Astro](docs/screenshots/02_astro-v16.png) | ![Rullini](docs/screenshots/14_rullini-v16.png) |
| ![Allineamento polare](docs/screenshots/06_polare-v16.png) | ![Calendario del cielo](docs/screenshots/07_cielo-v16.png) | ![Tema notte](docs/screenshots/21_cielo_notte-v16.png) |
| ![Fuoco stelle](docs/screenshots/05_stelle-v16.png) | ![Foto](docs/screenshots/08_foto-v16.png) | ![Torna alla 6D](docs/screenshots/19_torna_6d-v16.png) |
| ![Fuoco automatico](docs/screenshots/05b_fuoco_auto-v16.png) | ![Luna da fotografare](docs/screenshots/25_luna_foto-v16.png) | ![Dove puntare la Via Lattea](docs/screenshots/26_dove_puntare-v16.png) |
| ![Intervallometro da campo](docs/screenshots/04_intervallometro-v16.png) | ![Luogo e fuso](docs/screenshots/18d_opzioni_luogo-v16.png) | ![Aiuto con INFO](docs/screenshots/05c_aiuto-v16.png) |
| ![Live View al buio](docs/screenshots/11b_lv_al_buio-v16.png) | ![Luce cinerea](docs/screenshots/25b_luna_cinerea-v16.png) | ![Prova della 6D](docs/screenshots/24_prova-v16.png) |
| ![Profili](docs/screenshots/17_profili-v16.png) | ![Opzioni](docs/screenshots/18_opzioni-v16.png) | ![Filtri ND](docs/screenshots/12_filtri_nd-v16.png) |
| ![Bilanciamento del bianco](docs/screenshots/27_bianco-v16.png) | ![Menu Foto](docs/screenshots/08b_foto_bianco-v16.png) | ![Dark a fine sessione](docs/screenshots/04e_dark_fine_sessione-v16.png) |
| ![Le tue pellicole](docs/screenshots/14e_rullini_mie-v16.png) | ![La tua pellicola](docs/screenshots/14f_tua_pellicola-v16.png) | ![Avviso LENR](docs/screenshots/04d_intervallometro_lenr-v16.png) |
| ![In inglese: Home](docs/screenshots/en_home-v16.png) | ![In inglese: intervallometro](docs/screenshots/en_intervallometro-v16.png) | |

</details>

<p align="center"><img src="docs/screenshots/menu-v16.gif" width="540" alt="Menu Astro con l'anteprima della voce scelta"></p>

## Cronistoria

<p align="center"><a href="https://godeater58.github.io/Iride-6D/cronistoria.html"><img src="docs/screenshots/cronistoria-v16.png" width="620" alt="Cronistoria di Iride 6D: dalla 0.1 alla 1.6"></a><br>
<b><a href="https://godeater58.github.io/Iride-6D/cronistoria.html">Dalla 0.1 alla 1.6, in cinque giorni</a></b>: ogni versione con le novità, gli errori trovati e corretti e le parti rifatte da capo, e i test che crescono da 0 a 211.</p>

## Funzioni

| Area | Cosa fa |
|---|---|
| 🌌 **Astro** | Via Lattea con regola NPF e punto del cielo (applica tempo, diaframma, ISO, bianco) · intervallometro con preset Via Lattea, Deep sky, Star trails, Timelapse, Giorno-notte, Dark, Flat, Bias, con otturatore silenzioso, pausa per il dithering, controllo di batteria, scheda e temperatura e registro della sessione in CSV (lettore e piano di stacking per AstroStack in [tools/astrostack](tools/astrostack/)) · **dark proposti a fine sessione** con la stessa posa, ISO e temperatura · avviso se la riduzione disturbo lunghe esposizioni della 6D è accesa · fuoco sulle stelle con misura HFR in Live View e fuoco automatico a curva a V · Luna da fotografare (regola Looney 11 con fase e altezza) · dove puntare la Via Lattea ora per ora · allineamento polare su reticolo con mappa del cielo a nord (Grande Carro e Cassiopea) · calendario di 7 notti con linea del tempo della notte (crepuscoli, Luna, centro della Via Lattea), stelle visibili col cielo buio e con la Luna, quando torna il centro della Via Lattea e notte migliore · nomi file LGHT/DARK/FLAT/BIAS per lo stacking |
| 📷 **Foto** | Bracketing HDR · focus stacking · Dual ISO · ETTR · scatto su movimento e trappola di fuoco · strumenti del Live View (peaking, zebra, istogramma, falsi colori, zoom, spot) · filtri ND con posa B automatica · **bilanciamento del bianco**, il tasto che manca alla 6D: modi Canon e Kelvin con la scala, anche col SET dalla schermata di scatto |
| 🎞️ **Rullini** | 16 pellicole (colore, dia, B/N, cine) · **4 pellicole tue** (ricette: ISO, stile, contrasto, bianco, grana, vignettatura) da salvare e condividere come scheda di testo · push/pull · 12/24/36 pose · pausa e ripresa · contatore nella barra del Live View · registro su scheda · avviso se la macchina è in solo RAW · [preset per LightCraft e Lightroom](presets/) |
| 💾 **Profili** | 5 posti per le impostazioni preferite, con 5 profili di partenza da ritoccare: Ritratto (Av f/2.8), Paesaggio (Av f/8 ISO 100), Street (Av f/5.6 ISO Auto), Sport (Tv 1/1000), Notte (M 1/30 f/4 ISO 3200); se l'obiettivo non arriva al diaframma del profilo si usa il suo limite |
| 🔁 **Torna alla 6D** | Rimette tutto com'era prima di Iride: stile immagine e cursori, ISO, tempo, diaframma, bianco, anteprima, impostazioni di Magic Lantern, nomi dei file |
| 🌍 **Luogo** | 57 città pronte, latitudine e longitudine libere, fusi orari di tutto il mondo con l'ora legale automatica (controllati col database IANA) |
| ✨ **Interfaccia** | Carattere piccolo proporzionale con i bordi sfumati (al posto del Terminus di ML) · angoli arrotondati su pannelli, schede, pulsanti e interruttori · icone disegnate per le liste di Astro e Foto · aiuto col tasto INFO in ogni schermata · Sottomenu con anteprima della voce scelta (icona animata, a cosa serve, com'è adesso), percorso nell'intestazione, schede con i conti già fatti, Opzioni in sezioni · animazioni senza sfarfallio (doppio buffer): diaframma all'avvio, riquadri della Home che entrano e icone che si animano, zoom dal riquadro alla schermata, dialoghi che salgono come fogli, barra di selezione elastica, interruttori e valori che scorrono, avvisi che entrano e si spengono · caricamento e riavvolgimento del rullino con contapose a rullo · italiano o inglese · tema notte rosso, compare all'accensione, si apre col cestino (o col tasto luce) · diagnostica dei tasti |

### Magic Lantern modificato

Iride non è più solo un'interfaccia appoggiata sopra Magic Lantern: dalla 1.4 il pacchetto contiene anche un Magic Lantern (`autoexec.bin`) con il core modificato per Iride ([`module/magiclantern.patch`](module/magiclantern.patch), file `src/menu.c`). Le modifiche:

| Modifica al core | A cosa serve |
|---|---|
| **Modalità schermo intero** per i moduli | Mentre Iride è aperta Canon non può disegnare sullo schermo: niente grafica Canon che sbuca sopra Iride |
| **Dialogo Canon tenuto sveglio dal core** | In Live View ML lo risveglia da solo; il mezzo scatto chiude il menu anche senza ridisegni |
| **Segnali per il modulo** | ML dice quando il menu è pronto e quando Canon potrebbe aver disegnato: Iride si apre senza attese fisse e ridisegna solo quando serve |

Iride riconosce anche i tasti "finti" che Magic Lantern preme da solo (per esempio il tasto LV con cui accende il Live View), così non li scambia per tasti veri.

Le funzioni nuove del core sono collegate "deboli": `iride.mo` funziona anche col Magic Lantern ufficiale, solo senza questi vantaggi. Il firmware Canon non viene toccato: tutto gira dalla scheda, come Magic Lantern.

### Tasto luce

La 6D riceve il tasto luce del display superiore come evento `0x20`, e Magic Lantern lo usa nella riproduzione. Non è ancora confermato che lo invii anche in modalità di scatto. **Opzioni → Diagnostica tasti** mostra gli ultimi tasti ricevuti e li salva in `ML/LOGS/IRIDEKEY.TXT`.

### Rullini

<p align="center"><img src="docs/screenshots/rullini_tutti-v06.png" width="720" alt="I 16 rullini"></p>

Ogni rullino è disegnato in macchina, con grafiche originali: perno, coperchi zigrinati, cilindro ombreggiato, fessura di velluto, codice DX e codice a barre.
Sotto lo scaffale due righe per pellicola: il carattere e l'uso ideale (verificati su schede tecniche e prove: per esempio la Delta 3200 è una ISO 1000 reale esposta a 3200, la CineStill 800T è una pellicola cinema al tungsteno che di giorno vira al blu).

<p align="center"><img src="docs/screenshots/caricamento-v16.gif" width="540" alt="Caricamento di un rullino"></p>
<p align="center"><img src="docs/screenshots/riavvolgimento-v16.gif" width="540" alt="Riavvolgimento con la manovella"></p>

### Rullini e RAW

Il look della pellicola la 6D lo applica solo al **JPEG**: con la qualità in solo RAW, LightCraft e Lightroom mostrano un attimo l'anteprima con il look e poi il RAW neutro.
Due soluzioni: scattare in **RAW+L** (Iride avvisa se sei in solo RAW, ma non cambia la qualità da solo: Magic Lantern lo considera rischioso) oppure usare i **[16 preset XMP](presets/)**, uno per pellicola, che danno lo stesso carattere al RAW.

I calcoli astronomici girano in macchina, senza internet, e sono confrontati con [astropy](https://www.astropy.org/): Polare entro 0,4 s di angolo orario, Sole entro 0,004°, Luna entro 0,24°, orari di tramonto e buio al minuto, sorgere e tramonto della Luna entro 1 minuto.

## Installazione

1. La 6D deve avere il **firmware Canon 1.1.6** (vedi [Magic Lantern – 6D](https://builds.magiclantern.fm/6D.html)). Il firmware Canon non è incluso in questo repository.
2. Scarica `Iride6D-<versione>-SD.zip` dalla pagina **Releases** (per aggiornare: `autoexec.bin` e `ML/modules/iride.mo`; con il solo `iride.mo` Iride funziona ma senza lo schermo intero del core).
3. Formatta la scheda nella 6D e copia tutto il contenuto dello zip nella radice.
4. Ghiera su M, Menu → Versione firmware → aggiorna: parte l'installatore di Magic Lantern.
5. All'accensione compare la Home di Iride (si può spegnere in Opzioni). Poi si apre col **cestino**: dentro Iride il cestino porta al menu di Magic Lantern, un altro cestino lo chiude. Si può usare anche il tasto luce del display superiore (vedi «Tasto luce» qui sopra).

Istruzioni complete in [docs/LEGGIMI.txt](docs/LEGGIMI.txt); per la prima volta sulla macchina c'è una **[lista di prova da spuntare](docs/PROVA_6D.md)**. Se la macchina si blocca: togli e rimetti la batteria. Magic Lantern gira dalla scheda e non modifica la ROM.

## Compilare

```
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi python3-docutils zip
git clone https://github.com/reticulatedpines/magiclantern_simplified ml
cd ml && git checkout ed3e7c0dfacfdeaf3349dee73edf9e3621217462   # build ufficiale 6D, 20/06/2025
cp -r ../module/iride modules/
git apply ../module/magiclantern.patch
cd platform/6D.116 && make -j8      # -> build/magiclantern.zip
```

La compilazione e i test girano anche in automatico con GitHub Actions a ogni modifica (`.github/workflows/build.yml`). Per una release basta un clic: **Actions → Build → Run workflow** con *Pubblica la release*: compila, crea il tag della versione di `iride.c` (`IRIDE_VERSION`) e pubblica pacchetto SD, `iride.mo` e preset pellicola con le note prese dal CHANGELOG. Funziona anche come prima, creando una Release con tag `vX.Y` dal sito.

## Demo nel browser

[`site/`](site/) è la pagina della demo ([godeater58.github.io/Iride-6D](https://godeater58.github.io/Iride-6D/)): il simulatore e il codice di Iride compilati in WebAssembly con Emscripten (`simulator/web/`), con il testo disegnato dai caratteri RBF di Magic Lantern e l'ora del computer per Luna, cielo e Polare. GitHub Actions la ricompila e la pubblica a ogni modifica (`.github/workflows/pages.yml`). Per provarla in locale:

```
sudo apt install emscripten
sh simulator/web/build.sh            # -> site/iride.js e site/iride.wasm
cd site && python3 -m http.server    # poi http://localhost:8000
```

## Simulatore e test

`simulator/` esegue il codice reale di Iride su PC, al posto delle funzioni di Magic Lantern:

```
cd simulator && mkdir -p out
gcc -std=gnu99 -Ishim -o preview preview.c -lm && ./preview   # 211 test
pip install astropy pillow
python3 check_astro.py   # confronto astronomico con astropy
python3 check_tz.py      # fusi orari a confronto col database IANA
python3 render.py        # schermate in png/
python3 gif.py ../docs/screenshots   # GIF del README (caricamento, animazioni)
```

## Struttura

```
module/iride/       il modulo (iride.c include ui, moon, icons, store, film, campo, astro, sky2, photo, bench, screens)
tools/astrostack/   lettore delle sessioni CSV e piano di stacking per AstroStack
tools/i18n/         traduzioni inglesi (en.py) e generatore di module/iride/lang_en.inc
module/*.patch      aggiunge iride ai moduli e al pacchetto della 6D, e lo schermo intero al core di ML
simulator/          test su PC e generatore delle schermate (caratteri veri di ML in simulator/fonts)
simulator/web/      la demo nel browser (WebAssembly)
site/               la pagina della demo per GitHub Pages
presets/            preset pellicola XMP per LightCraft e Lightroom (e il loro generatore)
docs/               istruzioni e schermate
```

## Crediti e licenza

- Basato su [Magic Lantern](https://www.magiclantern.fm/) ([sorgenti](https://github.com/reticulatedpines/magiclantern_simplified)), GPL.
- Formula NPF: [Lightstalking](https://www.lightstalking.com/npf-rule/). Le funzioni Astro riprendono l'app Android Iride dello stesso autore.
- Licenza **GPL v2**, come Magic Lantern: vedi [LICENSE](LICENSE).

Canon ed EOS sono marchi di Canon Inc. Questo progetto non è affiliato a Canon.
