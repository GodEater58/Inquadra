# Prova di Iride sulla 6D vera

Una lista da spuntare con la macchina in mano. Serve a scoprire i problemi **in casa**, non sotto le stelle.
Durata: circa 40 minuti. Batteria carica, scheda con almeno 2 GB liberi, obiettivo AF (meglio uno zoom).

Se qualcosa non va: **non** formattare la scheda. Togli la batteria, poi mandami:

- la cartella `ML/LOGS` (soprattutto `IRIDE.CSV`, `IRIDEBEN.TXT` e gli eventuali `CRASH*.LOG`)
- una foto dello schermo col problema
- il numero del punto qui sotto

Una ✗ in una riga non blocca il resto: segna e vai avanti.

---

## 0. Prima di iniziare

- [ ] Firmware Canon **1.1.6** (Menu ⚙ → Versione firmware)
- [ ] Sulla scheda: `autoexec.bin`, `ML/modules/iride.mo`, `ML/SETTINGS/IRIDE.EN`
- [ ] Ghiera su **M**, stile immagine **Standard**, bilanciamento **AWB**: annota ISO e tempo, servono al punto 9

## 1. Avvio e schermo intero (core modificato dalla 1.4, mai provato prima)

- [ ] All'accensione Iride si apre da sola entro 1–2 s, con l'animazione
- [ ] **Nessun lampeggio** del menu Canon dietro Iride, neanche tenendo premuta una freccia per 5 s
- [ ] Le animazioni sono fluide (lo scaffale dei rullini scorre senza scatti)
- [ ] Dopo 1 minuto fermo su una schermata la macchina **non** si spegne né torna al menu Canon
- [ ] MENU dalla Home chiude Iride e si torna alla schermata di scatto
- [ ] Cestino riapre Iride; dentro Iride il cestino porta al menu di Magic Lantern, un altro cestino torna a Iride

## 2. Tasti

- [ ] Opzioni → Diagnostica tasti: ogni tasto premuto compare nella lista (frecce, SET, Q, INFO, MENU, PLAY, ghiere)
- [ ] La colonna "finto" resta vuota premendo i tasti a mano
- [ ] SET dalla schermata di scatto apre il bilanciamento del bianco; MENU lo chiude

## 3. Live View (il bug del rullino del 1.3)

Prima di questo punto cancella `ML/LOGS/IRIDELV.TXT` dalla scheda (se c'è): Iride ci scrive tutto quello che succede col Live View. Se qualcosa non va, è il file da mandarmi.

- [ ] Rullini → carica Portra 400 → START/STOP: Iride si chiude e il Live View **resta acceso** (avviso "START/STOP per tornare a Iride")
- [ ] Scatta una foto in Live View, poi START/STOP: il Live View si spegne e **Iride torna sui Rullini**
- [ ] Cambia rullino (Ektar 100) e premi di nuovo START/STOP: **nessun ciclo** tra Live View e scaffale
- [ ] Astro → Fuoco stelle → START/STOP: Iride resta sopra il Live View; mezzo scatto, foto, START/STOP: Iride torna su Fuoco stelle

## 4. Rullini

- [ ] Carica Portra 400, 36 pose: ISO, bianco (5300 K) e stile (Ritratto) cambiano sulla 6D
- [ ] Scatta 2 foto: il contatore passa a 2/36 e i file si chiamano `R001xxxx.JPG/CR2`
- [ ] Q sullo scaffale → Pausa: tornano le impostazioni normali; Riprendi: tornano quelle della pellicola
- [ ] Spegni e riaccendi la 6D: il rullino è ancora caricato con 2/36
- [ ] **Pellicola tua** (1.6): su una pellicola SET → "La tua versione": cambia contrasto e grana, Salva la scheda. Sulla scheda compare `ML/IRIDE/MIA1.TXT`
- [ ] Carica la tua pellicola dalla scheda MIE e scatta una foto: ISO, bianco e stile sono quelli scelti

## 5. Profili

- [ ] SET su Street (ghiera su Av): Av, ISO auto, 1/250 come limite
- [ ] Su Paesaggio con un obiettivo che non arriva a f/5.6 compare l'avviso "diaframma limitato"
- [ ] INFO risalva il profilo a modo tuo; spegni e riaccendi: è rimasto

## 6. Foto

- [ ] Strumenti Live View: peaking acceso, si vede in Live View; spento, sparisce
- [ ] Bracketing HDR 3 scatti: un solo mezzo scatto + scatto fa 3 foto a esposizioni diverse
- [ ] Filtri ND: 1/60 + ND1000 = 16 s, SET imposta il tempo

## 7. Astro (anche di giorno, col tappo)

- [ ] Via Lattea: con 24 mm f/2.8 la NPF dà circa 12 s; Applica mette il tempo
- [ ] Calendario e Dove puntare si aprono e mostrano la notte di oggi
- [ ] Tema notte: Opzioni → Tema → notte rossa. **Tutte** le schermate Astro (anche Luna e Dove puntare) sono rosse
- [ ] Intervallometro, preset Via Lattea, 3 scatti da 5 s: parte, il conteggio va a 3/3, si sente il doppio bip
- [ ] Sulla scheda c'è `DCIM/100CANON/ISaammgg.CSV` con 3 righe LGHT
- [ ] **Dark a fine sessione** (1.6): riaprendo Iride compare la proposta dei dark; SET con il tappo fa i dark con la stessa posa e ISO
- [ ] **Avviso LENR** (1.6): Menu Canon → Riduz. disturbo lunghe esposiz. = **Attiva**, poi 2 scatti da 5 s: Iride avvisa che ogni scatto dura il doppio. Rimetti **Disattiva**
- [ ] Cestino durante una sequenza la ferma ("interrotta")

## 8. Prova della 6D

- [ ] Opzioni → Lingua / Language → English: menu e aiuti in inglese; poi rimetti italiano
- [ ] Opzioni → Prova della 6D → SET: finisce senza bloccarsi
- [ ] `ML/LOGS/IRIDEBEN.TXT` esiste (allegalo comunque: dà i tempi veri della 6D)

## 9. Torna alla 6D

- [ ] Home → Torna alla 6D → SET: stile, bianco, ISO e tempo tornano come al punto 0
- [ ] Il rullino è in pausa, i file tornano a chiamarsi `IMG_xxxx`
- [ ] Menu ML → Modules: niente rimasto acceso per sbaglio (Dual ISO, ETTR, peaking)

## 10. Spegnimento e sicurezza

- [ ] Sportello della scheda aperto con Iride aperta: la 6D si spegne pulita (al riavvio rullino e profili ci sono)
- [ ] Batteria tolta a macchina accesa: al riavvio Iride riparte (usa la copia `IRIDE.BAK` se serve)

---

Fatto tutto? Scrivimi i numeri con ✗ (o "tutto ok"): con quello so cosa sistemare prima della prossima versione.
