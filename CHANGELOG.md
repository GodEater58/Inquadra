# Novità

## 1.0 – 1 novembre 2026
- **Descrizioni dei rullini riscritte e verificate** su schede tecniche e prove: due righe per pellicola, carattere e uso ideale. Corrette le imprecisioni: Delta 3200 (ISO reale 1000, a 3200 è già tirata), CineStill 800T (pellicola cinema al tungsteno, aloni rossi, di giorno vira al blu), Ektar (la grana più fine, calca i rossi sulla pelle), Superia (verdi intensi, palette fresca), Velvia (tolleranza minima, meglio in luce morbida), Ektachrome E100 (neutra con tendenza fredda) e le altre. Stesse descrizioni nei preset XMP
- **Animazioni nuove in tutta l'interfaccia** (tutte a tempo, saltabili con un tasto, più corte con *rapide*, assenti con *spente*):
  - avvio: un diaframma a 6 lamelle si apre ruotando, "IRIDE" sale lettera per lettera, poi linea e versione
  - Home: i riquadri entrano uno dopo l'altro; la cornice di selezione è elastica; l'icona scelta si anima (stella cadente, diaframma dell'obiettivo che scatta, pellicola che rientra e riesce, freccia che fa un giro, cursori che corrono, ingranaggi che girano)
  - zoom dal riquadro della Home alla schermata e ritorno con MENU: la schermata intera si ingrandisce e si rimpicciolisce dentro il riquadro, con le sue proporzioni
  - dialoghi dei rullini che salgono dal basso come un foglio e riscendono
  - avanti/indietro con parallasse (la schermata vecchia scivola a metà velocità)
  - barra di selezione elastica in tutte le liste; valori che entrano dal lato della freccia; interruttori ON/OFF con il pallino che corre
  - scaffale: sottolineatura della categoria che scivola, rullini che spuntano dal piano cambiando categoria, piccolo urto alla fine dello scaffale, testi che entrano dal lato del rullino
  - avvisi in basso che entrano con la loro tacca e si spengono piano
  - caricamento: contapose a rullo (S → 0 → 1 che scorrono), passaggi che entrano, barra di avanzamento e spunta finale; riavvolgimento con barra e spunta; alla fine la scena riscende e scopre lo scaffale (prima si tagliava di colpo)
  - Torna alla 6D: barra di ripristino, spunta, poi ritorno nella Home
- Corretto: dopo "Torna alla 6D" l'avviso "Fatto" non compariva
- Simulatore: 85 test (descrizioni che stanno nel dialogo, transizioni verificate sulla memoria video), fotogrammi di tutte le animazioni; `simulator/gif.py` crea le GIF del README

## 0.9 – 1 novembre 2026
- Il progetto si chiama **Iride 6D**: nomi aggiornati in firmware, istruzioni, preset (gruppo *Iride 6D - Pellicole*) e file delle release (`Iride6D-<versione>-SD.zip`, `Iride6D-preset-pellicole.zip`)
- Corretto: cambiando rullino si vedeva solo il riavvolgimento. Gli eventi interni di Canon (ISO, stile, bianco che cambiano) venivano presi per tasti e interrompevano il caricamento; ora passano a Canon come devono
- Un tasto premuto per saltare un'animazione viene consumato: non salta anche la successiva e non fa partire altre azioni
- Simulatore: 81 test

## 0.8 – 1 novembre 2026
- START/STOP dentro Iride accende o spegne il Live View e riapre Iride sulla stessa schermata (in modalità filmato resta il tasto di registrazione)
- Fuoco stelle: istruzioni e aiuto dei tasti aggiornati
- In Live View Iride riaccende la grafica (prima poteva restare invisibile) e tiene sveglio il menu Canon sotto, che sulla 6D altrimenti si chiude da solo
- Simulatore: 78 test

## 0.7 – 1 novembre 2026
- Corretto: aprendo Iride col cestino o all'accensione comparivano in basso gli indicatori audio di Magic Lantern ("L INT / R INT"), che rovinavano la riga d'aiuto. Il menu ML ora viene portato su Shoot → Iride
- Corretto: all'avvio il menu di Canon (Pulizia sensore… Vers. firmware) si vedeva sopra Iride. Iride aspetta che Canon abbia aperto il suo menu prima di disegnare
- Ridisegno completo periodico (invisibile grazie al doppio buffer) che cancella qualsiasi disegno arrivato da fuori
- Simulatore: 72 test

## 0.6 – 1 novembre 2026
- Icone della Home ridisegnate: cielo notturno con Via Lattea, costellazione, falce di luna, montagne e treppiede; reflex con obiettivo e anello arancione; rullino attuale con la pellicola e tre mini-foto; freccia di ripristino, cursori da mixer, ingranaggi
- All'accensione la 6D mostra subito la Home di Iride (opzione, attiva di fabbrica). Non si apre se all'avvio si tiene premuto a metà il pulsante di scatto o si usa un tasto
- Simulatore: 70 test

## 0.5 – 1 novembre 2026
- Rullini ridisegnati con tanti dettagli: perno della bobina, coperchi metallici zigrinati con riflesso, cilindro ombreggiato coi toni della tavolozza Canon, fessura di velluto, marca, nome, processo e ISO, filetti, codice DX e codice a barre
- Tre impaginazioni originali (fascia, diviso, righe); nessun logo o confezione reale riprodotti
- Linguetta perforata col taglio a mezza altezza; scaffale con piano in prospettiva e ombre di contatto
- Tema notte dedicato ai rullini (rosso, rosso scuro, nero)

## 0.4 – 1 novembre 2026
- Rullini: nuove animazioni di caricamento (rullino nel vano, linguetta che si aggancia alla bobina, pellicola perforata che avanza, sportello che si chiude, contapose S → 0 → 1) e di riavvolgimento (manovella, bobina che si svuota, rullino che esce)
- Scaffale: i rullini poggiano su una mensola, quello scelto si solleva e mostra la linguetta
- Il cestino apre Iride dalla schermata di scatto (opzione, attiva di fabbrica), alle stesse condizioni in cui Magic Lantern apre il suo menu; dentro Iride il cestino porta al menu di Magic Lantern
- Nessun lampo del menu ML all'apertura
- Opzioni → Diagnostica tasti: ultimi 12 tasti ricevuti, salvati in ML/LOGS/IRIDEKEY.TXT
- Simulatore: 65 test, fotogrammi delle animazioni

## 0.3 – 1 novembre 2026
- 16 preset pellicola XMP per LightCraft e Lightroom (`presets/`): danno al RAW il carattere del rullino
- Rullini: avviso se la qualità è in solo RAW (il look va solo nel JPEG). Iride legge la qualità ma non la cambia: Magic Lantern considera rischioso modificarla dal firmware
- Simulatore: 55 test

## 0.2 – 1 novembre 2026
- Interfaccia riscritta: doppio buffer senza sfarfallio, transizioni, selezione animata, schermata iniziale, tema notte rosso
- Home con Astro, Foto, Rullini, Torna alla 6D, Profili, Opzioni; apertura col tasto luce del display superiore
- Torna alla 6D: ripristino completo delle impostazioni toccate da Iride
- Astro: Star trails e Giorno-notte nell'intervallometro, allineamento polare, calendario del cielo (7 notti)
- Foto: HDR, focus stacking, Dual ISO, ETTR, movimento e trappola di fuoco, strumenti del Live View, filtri ND con posa B
- Rullini: push/pull, 12/24/36 pose, pausa e ripresa, contatore nella barra del Live View
- 5 profili personali, stato salvato in ML/SETTINGS/IRIDE.DAT
- Simulatore: 49 test, confronto astronomico con astropy

## 0.1 – 1 novembre 2026
- Prima versione: Via Lattea (NPF), intervallometro, fuoco stelle (HFR), etichetta file, scaffale dei rullini
