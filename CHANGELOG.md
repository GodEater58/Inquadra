# Novità

## 1.2.1 – 2 novembre 2026
- **Corretta la riga della macchina nella Prova della 6D** (`IRIDEBEN.TXT`): sulla 6D il simbolo "%" dopo la batteria spostava tutti i valori seguenti di un posto, così la temperatura mostrava i MB liberi (30387) e la scheda risultava vuota. Stesso difetto, e stessa correzione, per l'autonomia della batteria nella Prova e nell'intervallometro (ore e minuti sbagliati)
- **Calendario più leggero**: la Prova della 6D dava 62 ms per fotogramma (circa 16 al secondo). Quasi tutto era il costo di oltre 5.500 piccoli riempimenti: la Luna disegnata a segmenti e le barre delle notti composte campione per campione, stellina per stellina. Ora Luna (quando è già in memoria) e barre si compongono in memoria e vanno sullo schermo una riga alla volta: 49 riempimenti invece di 5.599, disegno identico pixel per pixel. Vale anche per le Lune della Home, del menu Astro e di *Luna da fotografare*
- La ricerca di quando torna la Via Lattea prova 2 notti per giro invece di 6: stesso tempo totale, ma la schermata non si ferma per quasi un decimo di secondo alla volta
- Nuovo controllo automatico (`check_formats.py`) che blocca questo tipo di formato prima della compilazione

## 1.2 – 2 novembre 2026
- **Fuoco automatico sulle stelle**: in *Fuoco sulle stelle* il tasto **Q** gira il fuoco da solo. Misura l'HFR dopo ogni passo (media di 3), disegna la curva a V, trova il minimo con una parabola sui quadrati dell'HFR e ci torna arrivando sempre dallo stesso verso, così il gioco degli ingranaggi non sposta il risultato. Se all'inizio peggiora prova dall'altra parte; si ferma con un tasto, se perde la stella o dopo 40 passi. Obiettivo su AF
- **Intervallometro da campo**
  - prima di partire: durata, ora di fine, batteria (% e tempo rimasto), spazio e scatti che stanno sulla scheda, temperatura del sensore
  - non parte con la batteria sotto il 5% o la scheda piena; durante la sequenza si ferma da solo al 4% o a scheda piena e dice perché
  - **otturatore silenzioso**: foto a piena risoluzione senza alzare lo specchio, pose da 0,3 a 30 s (modulo silent di ML: già attivo nel pacchetto per la scheda; chi aggiorna solo `iride.mo` lo attiva in Menu ML › Modules › silent. I file non hanno i dati EXIF)
  - **pausa per il dithering** ogni 1-10 scatti (5-60 s), per spostare la montatura (per esempio NG-EQ36) tra uno scatto e l'altro
  - **registro della sessione** `ISmmddhh.CSV` nella cartella delle foto: file, tipo, ora, posa, ISO, diaframma, focale, temperatura, batteria, Luna e magnitudine limite per ogni scatto. Con l'etichetta file attiva registra anche gli scatti fatti dal cavo (per esempio dal comando della montatura). Lettore Python per AstroStack in `tools/astrostack/`
- **Via Lattea (NPF) con il punto del cielo**: verso il polo le stelle si muovono meno e la posa si allunga (divisa per il coseno della declinazione, da 0 a 75 gradi)
- **Luna da fotografare** (nuova voce di Astro): fase, altezza, sorgere e tramonto; tempo con la regola Looney 11 corretto per la fase e per l'assorbimento dell'atmosfera, anche per la luce cinerea. SET imposta tempo, ISO e diaframma
- **Dove puntare la Via Lattea** (nuova voce di Astro, e SET dal Calendario sulla notte scelta): il cielo da est a ovest con la fascia della Via Lattea e il suo centro, direzione da bussola e altezza ogni mezz'ora dalle 16 alle 8
- **Live View al buio** in *Strumenti Live View*: schiarisce lo schermo da 1 a 6 stop per inquadrare di notte, le foto non cambiano
- **Luogo e fusi orari in tutto il mondo**
  - 57 città pronte (25 italiane e 32 nel mondo, coordinate GeoNames) nella nuova riga *Luogo*; latitudine e longitudine restano regolabili e mostrano S e O
  - fusi con l'ora legale automatica: Europa centrale, occidentale e orientale, USA (est, centro, montagne, Pacifico), Australia sud-est, Nuova Zelanda; poi gli scarti fissi a quarti d'ora da UTC-12 a UTC+14 (India +5:30, Nepal +5:45...). Confrontati col database IANA dei fusi: 249.888 confronti 2026-2028, nessuna differenza
- **Aiuto col tasto INFO** in ogni schermata (tranne Profili, dove INFO salva). L'allineamento polare cambia vista con SET o sinistra/destra
- **Prova della 6D** (Opzioni › Strumenti): misura i tempi di disegno e dei calcoli sulla macchina vera e li salva in `ML/LOGS/IRIDEBEN.TXT`. Gli avvisi mostrati da Iride finiscono anche nel registro `ML/LOGS/IRIDE.CSV`
- Calendario: gli orari della Via Lattea non escono più dal bordo destro mentre la riga entra
- Simulatore: 148 test (fuoco automatico con gioco degli ingranaggi da 0 a 4 passi, sessione, dithering, batteria, scheda piena, città e fusi, aiuto); controllo dei fusi con `check_tz.py`
- Comprende anche le novità della 1.1, che non era uscita come release: sottomenu ridisegnati, Luna disegnata dal vero, Calendario con la luce della Luna e il ritorno della Via Lattea, correzioni del fuoco sulle stelle (dettagli nel [CHANGELOG](https://github.com/GodEater58/Iride-6D/blob/main/CHANGELOG.md))

## 1.1 – 2 novembre 2026
- **Sottomenu ridisegnati** con la stessa cura della Home:
  - Astro e Foto: a sinistra le voci con un'icona ciascuna, a destra la voce scelta con un'illustrazione animata, a cosa serve e com'è adesso (per esempio "12.2 s con 24 mm f/2.8", "Polare alle ore 12:52", "attivo: 5 scatti da 1,5 EV"); pallino verde sulle funzioni accese
  - intestazione con il percorso: *Astro › Via Lattea*
  - impostazioni su pannelli, valori con frecce disegnate, azioni come pulsanti (pieni quando sono scelti)
  - schede con i conti fatti: regola NPF a confronto con la regola del 500, durata della sequenza e del video nell'intervallometro, gli scatti dell'HDR dal più scuro al più chiaro, i piani di fuoco dello stack, il tempo con il filtro ND, le righe del Dual ISO, l'istogramma dell'ETTR
  - Opzioni divise in sezioni (Aspetto, Apertura di Iride, Scatto, Luogo e ora, Strumenti) con scorrimento morbido e barra laterale
  - Profili come schede: modo di scatto nel tondo, tempo, diaframma, ISO e stile come etichette
  - Torna alla 6D: riepilogo, un'icona per ogni tipo di modifica e un pulsante grande
- **Calendario del cielo: la Luna conta anche per quanto illumina il cielo**
  - per ogni notte: fin dove si vedono le stelle a occhio nudo col cielo buio e con la Luna in cielo, per esempio *Stelle fino a mag 6,1; con la Luna 5,5 (sorge alle 23:06)*. Brillanza del cielo allo zenit col modello di Krisciunas e Schaefer (fase e altezza della Luna), magnitudine limite con la formula usata per gli SQM; confrontato con astropy, scarto 0,0
  - **linea del tempo più intuitiva**: una sola barra del cielo dove più scuro vuol dire meglio. Azzurro e blu il giorno e il crepuscolo, nero il cielo buio con le stelline, grigio quando c'è la Luna (più chiaro quanto più illumina, e con meno stelline). Le zone sono scritte dentro la barra: *sera*, *cielo buio*, *Luna 49%* con la sua fase disegnata, *alba*. Sotto, il centro della Via Lattea: barra verde quando si fotografa, linea sottile quando è alto ma c'è luce. Le righe dei 7 giorni usano gli stessi colori, senza legenda da imparare
  - la notte migliore si sceglie con le stelle visibili ora per ora, non solo con le ore senza Luna
  - nuova voce in Opzioni › Luogo e ora: **Cielo del luogo** (da 22,0 *molto buio* a 18,5 *centro città*, 21,0 *campagna* di partenza)
  - quando il centro della Via Lattea non c'è in nessuna delle 7 notti, Iride dice quando torna e a che ora: *torna dal 26/2, ore 04:50-05:20* (da Roma; prima notte con almeno mezz'ora utile, confrontata con astropy: un giorno di scarto). Prima diceva solo *non visibile col buio*
  - colonna Via Lattea senza zeri iniziali, così gli orari non escono dalla riga
- **Luna disegnata dal vero** nel Calendario del cielo (e più grande), nel calcolo iniziale e nell'icona del menu Astro:
  - mari e crateri nelle loro posizioni selenografiche, come si vedono dall'Italia (nord in alto, Mare Crisium a destra): Imbrium, Serenitatis, Tranquillitatis, Procellarum, Frigoris e gli altri, Tycho, Copernico, Platone, Clavius e altri 45 crateri con nome, più un centinaio di crateri minori sugli altipiani del sud
  - luce come sulla Luna vera: la Luna piena è piatta, il bordo di una falce è brillante e la luce cala verso il terminatore; vicino al terminatore si vedono il rilievo dei crateri e il bordo frastagliato
  - raggi di Tycho e Copernico verso la Luna piena, luce cinerea sulla parte in ombra, bordo sfumato
  - conti tutti interi e disegno in memoria finché la fase non cambia: niente rallentamenti sulla 6D
- **Corretto lo sfarfallio** dello zoom tra la Home e le schermate: il rettangolo veniva prima riempito di grigio sullo schermo visibile e poi ridisegnato. Ora ogni riga viene composta a parte e scritta una volta sola
- Animazioni più regolari: un fotogramma più lento del solito non le ferma più a metà
- Nelle impostazioni la barra di selezione, mentre scorre da una riga all'altra, non copre più i titoli di sezione
- In Live View Iride si ridisegna ogni 250 ms, così il menu Canon che ML risveglia ogni 2 secondi non si vede sopra
- Corretto: un testo che finisce con uno spazio poteva far leggere oltre la sua fine
- **Fuoco sulle stelle, tre correzioni nella misura:**
  - con la stella molto sfocata l'HFR si fermava a circa 12 (e poteva perfino calare girando nel verso sbagliato): ora l'apertura di misura si allarga da sola e il valore cresce sempre con la sfocatura (provato fino a 26)
  - la stella veniva cercata anche sotto il pannello in basso, dove non si vede: ora l'area di ricerca è esattamente il riquadro disegnato
  - un pixel caldo più luminoso di una stella debole poteva "vincere" e far dire "nessuna stella"; due pixel caldi vicini potevano essere presi per una stella. Ora il candidato si sceglie dai pixel intorno e servono almeno 4 pixel di stella
  - accanto all'HFR compare "meglio" o "peggio" rispetto alle misure precedenti; se la stella è satura il consiglio è di abbassare l'ISO
- Menu Astro e Foto: lo stato delle voci si ricalcola ogni 300 ms invece che a ogni fotogramma
- **Controllo bug a tappeto** (analisi statica con gcc e clang, AddressSanitizer e UndefinedBehaviorSanitizer, casi estremi):
  - un testo che usciva dallo schermo in alto veniva spostato da Magic Lantern sul bordo, così se ne vedeva un pezzo: succedeva con le scritte del rullino mentre vola via nel caricamento e con i titoli di sezione delle Opzioni mentre la lista scorre. Ora i testi fuori dallo schermo non si scrivono
  - Calendario: un valore rovinato del cielo del luogo nella configurazione non falsa più i conti; a latitudini dove il centro della Via Lattea non è mai alto col buio lo dice così (prima diceva che non sale sopra 10 gradi, non sempre vero)
  - Luna: dall'emisfero sud è disegnata capovolta, come la si vede da lì
  - nomi interni che si sovrapponevano ad altri (nessun effetto visibile, ma pericolosi per le modifiche future)
- Simulatore: 108 test, con 6000 tasti a caso su tutte le schermate controllati con AddressSanitizer, più 10 casi estremi (ora legale, capodanno, 29 febbraio, sole di mezzanotte, poli, emisfero sud, equatore, configurazione rovinata) e il controllo che nessun testo o forma esca dallo schermo

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
- **Allineamento polare rifatto**: reticolo con corona graduata ogni 15 minuti, croce, cerchio della Polare con la sua distanza dal polo; la lancetta arriva con un piccolo rimbalzo, la Polare lancia un impulso a ogni aggiornamento; occhio nudo / cannocchiale è un interruttore e il passaggio ruota la lancetta di mezzo giro; orario della prossima culminazione; nuova mappa del cielo a nord con Grande Carro, Cassiopea, Polare e orizzonte per trovarla (coordinate verificate)
- **Calendario del cielo rifatto**: su/giù sceglie una delle 7 notti; per la notte scelta fase lunare, buio, sorgere e tramonto della Luna (entro 1 minuto da astropy), centro della Via Lattea e una linea del tempo dalle 16 alle 8 con crepuscoli, Luna, Via Lattea, finestra buia senza Luna e il momento attuale; ogni riga ha la sua mini-notte; stella sulla notte migliore; tutto entra con animazioni. Tema notte solo in rosso scuro
- Corretto: dopo "Torna alla 6D" l'avviso "Fatto" non compariva
- Simulatore: 86 test (anche sorgere e tramonto della Luna confrontati con astropy) (descrizioni che stanno nel dialogo, transizioni verificate sulla memoria video), fotogrammi di tutte le animazioni; `simulator/gif.py` crea le GIF del README

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
