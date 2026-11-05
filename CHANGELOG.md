# Novità

## 1.6.2 – 5 novembre 2026
- **Corretto: dopo aver guardato una foto, START/STOP riportava al menu Canon invece che a Iride**. Guardare la foto (PLAY) spegne il Live View per un momento: Iride ci provava subito, trovava la 6D in riproduzione e lasciava perdere. Ora aspetta: torna quando il Live View è spento e la schermata di scatto è libera per 1,2 secondi di fila, e se nel frattempo il Live View riparte (la 6D lo riaccende uscendo dalla riproduzione) continua ad aspettare
- Simulatore: 3 test nuovi (222)

## 1.6.1 – 5 novembre 2026
- **Live View da Iride rifatto**: START/STOP dentro Iride ora chiude Iride e lascia il Live View acceso, per inquadrare e scattare (prima Iride si rimetteva subito sopra e lo copriva). START/STOP di nuovo, anche dopo una foto, spegne il Live View e riporta Iride sulla schermata di prima. Solo *Fuoco stelle*, che lavora sopra l'immagine, si riapre subito
- **Corretto: dopo una foto in Live View, spegnendolo, Iride non tornava**. Ora torna, anche da *Fuoco stelle* chiusa col mezzo scatto
- **Registro del Live View** `ML/LOGS/IRIDELV.TXT`: tasti LV (veri o premuti da Magic Lantern), aperture e chiusure di Iride, Live View acceso e spento, con i millisecondi. Serve a capire sulla 6D il ciclo tra Live View e Rullini che il simulatore non riproduce
- Simulatore: 8 test nuovi (219)

## 1.6 – 5 novembre 2026
- **Dark a fine sessione**: finita una serie di light (Via Lattea, Deep sky, Star trails), alla prossima apertura Iride propone i dark con la stessa posa e lo stesso ISO, da fare subito col tappo, quando il sensore è ancora alla stessa temperatura. Mostra la temperatura di adesso e quella delle light e avvisa se il sensore si è scaldato. *Non ora* toglie la proposta
- **Avviso della riduzione disturbo lunghe esposizioni (LENR)**: Magic Lantern non ne conosce la proprietà, così Iride la riconosce dal tempo dello scatto (una posa da 8 s che tiene occupata la macchina per 16). Con la LENR accesa ogni scatto dura il doppio: Iride lo dice con tre bip e un avviso, lo scrive nel registro e lo ricorda nella scheda dell'intervallometro. Sotto i 4 s non giudica (niente falsi allarmi)
- **Le tue pellicole (ricette)**: nuova scheda **MIE** nello scaffale. Da qualunque pellicola, SET → *La tua versione* crea una copia da cambiare: ISO, stile, contrasto, saturazione, tonalità e bianco (li applica la 6D al JPEG), grana e vignettatura (per il preset del PC) e colore della scatola. Fino a 4 ricette; se il rullino è nella macchina la 6D segue le modifiche subito
  - **scheda sulla SD**: *Salva* scrive `ML/IRIDE/MIA1.TXT`, un testo semplice da condividere; *Leggi* carica la scheda di un amico (anche scritta a mano)
  - sul PC `presets/make_presets.py --ricetta MIA1.TXT cartella` la trasforma in un preset XMP completo (pellicola di partenza + modifiche + grana e vignettatura)
- **Interfaccia in inglese**: Opzioni › *Lingua / Language*. Menu, righe, valori, avvisi e aiuti di Iride in inglese (circa 800 testi); i registri sulla scheda restano in italiano, così AstroStack li legge sempre. Le traduzioni stanno in `tools/i18n/en.py`
- **Giorno-notte**: durante la sequenza la barra in alto mostra tempo e ISO, per vedere la rampa dell'esposizione di Magic Lantern
- **AstroStack**: `piano_stack()` raggruppa le light per posa, ISO e focale e sceglie dark (stessa temperatura), flat e bias, con gli avvisi su quello che manca; `python3 iride_session.py cartella --json` dà il piano già pronto
- **Release con un clic**: Actions › Build › *Run workflow* con *Pubblica la release* compila, crea il tag della versione scritta in `iride.c` e pubblica la release con le note del CHANGELOG
- **Prova sulla 6D vera**: lista da spuntare in [docs/PROVA_6D.md](docs/PROVA_6D.md), anche nel pacchetto SD (`PROVA_6D.txt`)
- Demo nel browser: in Live View si vede il cielo con una stella sfocata, e *Fuoco stelle* la mette a fuoco davvero (a mano o con Q); **PLAY** rivede le foto scattate, con nome del file, pellicola, ISO e bianco del rullino
- **Cronistoria** sul sito: [godeater58.github.io/Iride-6D/cronistoria.html](https://godeater58.github.io/Iride-6D/cronistoria.html), tutte le versioni con novità, errori e rifacimenti
- Simulatore: 30 test nuovi (211): LENR, dark a fine sessione, ricette e schede, inglese su tutte le schermate senza testi fuori dallo schermo

## 1.5.2 – 5 novembre 2026
- **Corretto il tema "notte in Astro"**: le schermate *Luna da fotografare* e *Dove puntare* (aggiunte nella 1.2) restavano bianche e arancioni invece che rosse, e di notte rovinavano l'adattamento al buio. Un test nuovo controlla che tutte le schermate di Astro diventino rosse
- Demo nel browser:
  - retro ricalcato sulla foto della 6D (ghiera dei modi, MENU e INFO, mirino con conchiglia e diottrie, START/STOP, AF-ON e blocco AE, zoom e play accanto allo schermo, Q, ghiera con SET, cestino e LOCK); su telefono una versione compatta
  - START/STOP mostra il Live View per circa 2 secondi, come Magic Lantern sulla 6D, prima che Iride si riapra
  - rullino, profili e opzioni restano salvati nel browser anche ricaricando la pagina (pulsante *Ricomincia da capo* per azzerarli)
  - avvisi di Magic Lantern visibili sulla schermata di scatto; messaggio quando il cestino aprirebbe il menu ML
  - croce e ghiere ripetono il comando tenendole premute; freccia giù che restava in alto a sinistra; niente scorrimento laterale su telefono

## 1.5.1 – 5 novembre 2026
- **Prova Iride nel browser**: [godeater58.github.io/Iride-6D](https://godeater58.github.io/Iride-6D/). Lo stesso codice di Iride e il simulatore dei test, compilati in WebAssembly: si usa con la tastiera o coi tasti di una 6D disegnata, con l'ora del computer per Luna, cielo e Polare. GitHub Actions la ricompila e la pubblica a ogni modifica
- **Caratteri veri di Magic Lantern nel simulatore**: larghezze, schermate del README e demo usano i caratteri RBF della 6D (`simulator/fonts`) invece di un carattere del PC. Così i controlli "testo fuori dallo schermo" valgono anche sulla macchina, e hanno trovato quattro testi che sulla 6D uscivano dal loro spazio:
  - piede della Home: "cestino = menu ML" usciva dal bordo destro (ora "cestino = ML")
  - menu Astro: "Dove puntare la Via Lattea" usciva dalla riga (ora "Dove puntare"; nella scheda a destra il titolo lungo usa un carattere più piccolo)
  - menu Foto: "Bilanciamento del bianco" finiva sotto il pallino verde (ora "Bilanciamento bianco")
  - Strumenti Live View: la nota in basso usciva dal riquadro
- Schermate e GIF del README rifatte con i caratteri veri; schermate raccolte in una sezione a scomparsa

## 1.5 – 5 novembre 2026
- **Bilanciamento del bianco** (Foto › Bilanciamento del bianco): la 6D non ha un tasto per il bianco ed è una delle mancanze più citate dagli utenti. Automatico, luce diurna, ombra, nuvoloso, tungsteno, fluorescente, flash e Kelvin da 2500 a 10000 K (passi di 100, 500 con la ghiera superiore), con la scala delle temperature dal caldo al freddo
  - **SET dalla schermata di scatto** apre subito il bianco (Opzioni › *SET apre il bianco*, anche nel menu ML; spento di serie per non togliere il SET a chi l'ha già assegnato in Canon). MENU o *Fatto* tornano a scattare. I SET "premuti" da Magic Lantern restano a Canon
- **Avvio più rapido**: la Home compare appena Magic Lantern ha finito di partire (circa un secondo) invece che dopo 2,5 secondi fissi; poi la schermata di scatto si controlla ogni decimo di secondo invece che ogni quarto
- Menu Foto con 8 voci (righe un poco più basse, come in Astro) e icona nuova per il bianco
- Simulatore: 9 test nuovi (SET con opzione spenta, SET di ML, cambio di modo e Kelvin con i limiti, MENU dalle due entrate)

## 1.4.3 – 5 novembre 2026
- **Profili di partenza**: alla prima accensione i posti vuoti dei Profili hanno 5 impostazioni pronte da ritoccare
  - Ritratto: Av, f/2.8, ISO 200, stile Ritratto
  - Paesaggio: Av, f/8, ISO 100, stile Paesaggio
  - Street: Av, f/5.6, ISO Auto, stile Standard
  - Sport: Tv, 1/1000, ISO Auto, stile Standard
  - Notte in città a mano: M, 1/30, f/4, ISO 3200, stile Standard
- Con la ghiera sul modo del profilo si carica tutto, con un altro modo stile, ISO e bianco. Si cambiano come sempre: impostazioni sulla 6D, poi INFO sul profilo
- **Diaframma adattato all'obiettivo**, per tutti i profili: se l'obiettivo non arriva al diaframma del profilo (f/2.8 su uno zoom f/4) si usa il suo limite, il più aperto o il più chiuso, e Iride lo dice ("Caricato con f/4: il più aperto dell'obiettivo")
- I profili già salvati restano com'erano: i profili di partenza vanno solo nei posti vuoti, una volta sola e senza doppioni dello stesso nome; uno cancellato non ricompare
- Simulatore: 6 test nuovi sui profili di partenza e sul diaframma adattato

## 1.4.2 – 5 novembre 2026
- **Registro della sessione: un file per notte** invece che uno per ora. Si chiama `ISaammgg.CSV` con la data della sera (`IS261102.CSV` = notte del 2 novembre 2026); fino a mezzogiorno del giorno dopo si continua a scrivere lì, anche a cavallo di capodanno. Il lettore per AstroStack legge sia i file nuovi sia i vecchi `ISmmddhh.CSV`
- Dopo un riavvio a metà notte (cambio di batteria) la numerazione degli scatti nel registro riprende dall'ultima riga invece di ricominciare da 1
- **Lavori su scheda in un task di Iride**: salvataggio, prefisso dei file e registro degli scatti dal cavo non girano più nel task dell'orologio di Magic Lantern, che ogni secondo serve anche alle altre funzioni di ML e non deve aspettare la scheda
- Zoom indietro verso la Home: la memoria per la copia della schermata si chiede una volta e si tiene finché Iride è aperta, invece di chiederla e renderla a ogni ritorno
- GitHub Actions: a ogni modifica il simulatore gira anche con AddressSanitizer e UBSan (2 serie da 8.000 tasti a caso)
- Simulatore: 2 test nuovi (notte a cavallo di capodanno, riavvio a metà notte)

## 1.4.1 – 5 novembre 2026
Revisione completa del codice: simulatore con AddressSanitizer e UndefinedBehaviorSanitizer (6 serie da 15.000 tasti a caso), analisi statica con cppcheck, compilazione per la 6D con avvisi extra.
- **Salvataggio più sicuro** (`IRIDE.DAT`)
  - due copie scritte a turno (`IRIDE.DAT` e `IRIDE.BAK`) col numero del salvataggio: se la batteria si stacca o la scheda esce mentre si scrive resta la copia di prima, e non si perdono rullino e impostazioni di "Torna alla 6D"
  - corretto: la somma di controllo era calcolata su P e poi P veniva copiato; se nel frattempo il contatore del rullino cambiava, il file risultava rovinato al riavvio e si ripartiva dai valori di fabbrica. Ora si calcola sulla copia
  - corretto: una modifica arrivata durante la scrittura veniva segnata come già salvata
  - un salvataggio alla volta anche quando lo chiedono insieme contatore, chiusura di Iride e spegnimento
  - scheda piena o protetta: si riprova dopo 30 s invece che ogni secondo
  - dopo la lettura i valori fuori posto vengono rimessi nei limiti
- **Corretto il registro della sessione** per gli scatti dal cavo: passando da 9999 a 0001 contava due foto invece di una
- Corretto il primo fotogramma del rullino caricato col contatore Canon a 9999
- **Cerchi pieni 10-20 volte più veloci**: quello di Magic Lantern scrive colonne un pixel alla volta ripassando sugli stessi punti (270 µs per raggio 20 misurati sulla 6D, circa 15 ms per il reticolo dell'allineamento polare). Il nuovo li disegna a righe intere con gli stessi pixel, verificato punto per punto da raggio 0 a 170. Più leggeri Allineamento polare, Home, icone e Luna
- Prova della 6D: tempi giusti anche quando l'orologio in microsecondi passa il limite
- Simulatore: 5 test nuovi sul salvataggio (copie a turno, scrittura interrotta, valori fuori posto), stima del costo di disegno per schermata (`IRIDE_PROFILE=1`), tasti a caso configurabili (`IRIDE_FUZZ_SEED`, `IRIDE_FUZZ_N`)

## 1.4 – 5 novembre 2026
- **Magic Lantern modificato per Iride** (`module/magiclantern.patch`, `src/menu.c`): nuova *modalità schermo intero* per i moduli che disegnano tutto lo schermo sopra il menu ML
  - finito il primo disegno del dialogo Canon sotto il menu, ML impedisce a Canon di copiare la sua grafica sullo schermo: niente più disegni Canon che sbucano sopra Iride
  - ML tiene sveglio da solo il dialogo Canon in Live View e chiude il menu col mezzo scatto anche senza richieste di ridisegno
  - ML dice a Iride quando il dialogo è pronto e quando Canon potrebbe aver disegnato
- **Iride più leggera e più rapida ad aprirsi** col nuovo core: niente più attesa fissa di 350 ms all'apertura (si aspetta solo il tempo che serve), niente risveglio del menu Canon ogni mezzo secondo in Live View, ridisegno di controllo ogni 2 s invece che ogni 150-250 ms: più tempo per le animazioni
- **Tasti finti di ML riconosciuti**: il tasto LV che Magic Lantern "preme" per accendere o spegnere il Live View, se arriva a Iride aperta, va a Canon e non viene più preso per un START/STOP (la causa del Live View avanti e indietro, corretto in modo provvisorio nella 1.3.4)
- La *Diagnostica tasti* segna i tasti "da ML"; `IRIDEKEY.TXT` dice se il core ha lo schermo intero
- Compatibile anche col Magic Lantern ufficiale: le funzioni nuove sono collegate "deboli", con il solo `iride.mo` Iride lavora come nella 1.3.4
- Per aggiornare copia `autoexec.bin` e `ML/modules/iride.mo` dal pacchetto
- Simulatore: 5 test nuovi (tasto LV finto, schermo intero, ML senza la patch)

## 1.3.4 – 5 novembre 2026
- **Corretto il Live View che andava avanti e indietro dopo un cambio di rullino**: premendo START/STOP dentro Iride la 6D provava ad accendere il Live View, tornava subito sui Rullini e ricominciava finché non si premeva il pulsante di scatto. Magic Lantern accende il Live View "premendo" un tasto LV finto; subito dopo un cambio di rullino Canon è occupata a salvare stile, ISO e bianco, il tasto finto arrivava in ritardo e lo riceveva Iride, già riaperta, che lo prendeva per un nuovo START/STOP. Ora Iride si riapre solo quando il Live View è partito (o spento) davvero, e per un secondo e mezzo dopo la riapertura ignora un altro START/STOP
- Simulatore: 5 test nuovi (Canon occupata, tasto LV in ritardo, doppio START/STOP)

## 1.3.3 – 3 novembre 2026
- **Testo piccolo più veloce**: la Prova della 6D (1.3.2) dava 28 microsecondi per lettera piccola contro 20 per le lettere medie di Magic Lantern. Ora i caratteri sono decompressi una volta sola, si saltano le righe vuote di ogni lettera e le sfumature si calcolano una volta per sfondo
- **Prova della 6D più fedele**: ogni schermata fa un giro a vuoto prima di essere misurata. Il Calendario risultava 54 ms perché il primo dei tre fotogrammi ridisegnava da zero la Luna grande (100 ms), che nell'uso resta in memoria
- Dai "mattoni" misurati sulla 6D: un riempimento costa 1,3 microsecondi, una riga di 600 pixel 3 µs, scritta un byte alla volta 30 µs e a blocchi 4 µs, una linea di 200 pixel 32 µs, un cerchio pieno di raggio 20 270 µs

## 1.3.2 – 3 novembre 2026
- **Disegno a blocchi**: la memoria dello schermo della 6D è lenta un byte alla volta e veloce a blocchi. Il testo piccolo ora legge lo sfondo a righe intere, sfuma in memoria e riscrive a righe intere; Luna, barre del cielo e icone si scrivono a tratti con memcpy invece che pixel per pixel. La 1.3.1 sulla 6D vera dava Home 21,6 ms, Intervallometro 15,5, Calendario 52,3, Polare 47,0
- **Prova della 6D: i "mattoni" del disegno** nel file `IRIDEBEN.TXT`: riempimenti da 1 pixel, righe col riempimento, righe scritte e lette un byte alla volta e a blocchi, lettere piccole di Iride e medie di Magic Lantern, linee, cerchi. Servono per ottimizzare con i tempi veri della macchina

## 1.3.1 – 3 novembre 2026
- **Angoli arrotondati anche dove mancavano**: riquadro dell'aiuto (INFO), barre del cielo nel Calendario (estremità tonde e cornice), grafico dell'HFR e riquadro di ricerca della stella in *Fuoco stelle*, grafico di *Dove puntare* e il segno dell'ora scelta, vano della scena dei rullini, campioni di esposizione dell'HDR, righe del Dual ISO

## 1.3 – 3 novembre 2026
- **Carattere piccolo nuovo**: le scritte piccole (etichette, titoli di sezione, schede, profili) usavano il Terminus 12 di Magic Lantern, a spaziatura fissa, che accanto ai caratteri grandi sembrava "da terminale". Ora sono in Inter Medium 13 px, proporzionale, con i bordi sfumati su 4 livelli: sui grigi i pixel di bordo prendono il grigio giusto letto dallo schermo; su arancione e sui rossi del tema notte restano netti. Generato da `tools/font/make_font.py` (Inter, SIL Open Font License)
- **Angoli arrotondati** su righe delle impostazioni e dei menu, barra di selezione, riquadri della Home e cornice che li sceglie, schede, etichette, pulsanti, barre di avanzamento; gli interruttori ON/OFF sono a pillola
- **Icone nuove nelle liste di Astro e Foto**: 15 icone disegnate a 8 volte la grandezza e ridotte a 26×26 con l'antialias sui colori della 6D (`tools/icons/make_icons.py`); col tema notte diventano solo rosse
- **Home**: obiettivo e impostazioni in una striscia di stato con l'obiettivo disegnato e le etichette GHIERA, ISO, STILE, FILE
- **Profili**: il posto libero è un'etichetta tratteggiata con il "+" ("INFO salva qui le impostazioni di adesso") invece della frase lunga ripetuta
- Di notte le etichette sono solo contornate, senza rosso su rosso
- Filtri ND: le scritte SENZA e CON non toccano più il titolo della scheda
- Simulatore: controllo delle sovrapposizioni del testo piccolo; schermate e GIF rifatte

## 1.2.3 – 3 novembre 2026
- **Manovella di riavvolgimento ridisegnata**: prima era una linea arancione con un pallino. Ora è come sulle reflex a pellicola: perno, pomello zigrinato con le righe che girano, braccio metallico con l'impugnatura dritta (tappo arancione). Gira in piano vista un po' dall'alto, passa dietro il pomello nella metà lontana, si apre prima di girare e si richiude a fine riavvolgimento; nei giri veloci lascia due sagome sfumate

## 1.2.2 – 2 novembre 2026
- **Calendario più leggero**: la Prova della 6D dava 62 ms per fotogramma (circa 16 al secondo). Quasi tutto era il costo di oltre 5.500 piccoli riempimenti: la Luna disegnata a segmenti e le barre delle notti composte campione per campione, stellina per stellina. Ora Luna (quando è già in memoria) e barre si compongono in memoria e vanno sullo schermo una riga alla volta: 49 riempimenti invece di 5.599, disegno identico pixel per pixel. Vale anche per le Lune della Home, del menu Astro e di *Luna da fotografare*
- La ricerca di quando torna la Via Lattea prova 2 notti per giro invece di 6: stesso tempo totale, ma la schermata non si ferma per quasi un decimo di secondo alla volta

## 1.2.1 – 2 novembre 2026
- **Corretta la riga della macchina nella Prova della 6D** (`IRIDEBEN.TXT`): sulla 6D il simbolo "%" dopo la batteria spostava tutti i valori seguenti di un posto, così la temperatura mostrava i MB liberi (30387) e la scheda risultava vuota. Stesso difetto, e stessa correzione, per l'autonomia della batteria nella Prova e nell'intervallometro (ore e minuti sbagliati)
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
