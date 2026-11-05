/* Generato da tools/cronistoria/genera.py: non modificare a mano */
window.CRONISTORIA = {
"fasi": [
{
"when": "1 novembre · sabato",
"title": "Nascita: un modulo sopra Magic Lantern",
"vers": [
{
"v": "prima",
"t": "L'idea e la 6D pronta",
"tests": null,
"items": [
[
"ok",
"Idea: i rullini simulati come nel video R-FILM per la 5D3, più un menu Astro come quello dell'app Iride per Android"
],
[
"mod",
"La 6D riportata dal firmware Canon 1.1.9 alla 1.1.6, quello richiesto da Magic Lantern"
]
]
},
{
"v": "0.1",
"t": "Prima versione",
"tests": 0,
"items": [
[
"ok",
"Via Lattea con la regola NPF, intervallometro, fuoco sulle stelle (HFR), etichetta dei file, scaffale dei rullini"
]
]
},
{
"v": "0.2",
"t": "Interfaccia riscritta",
"tests": 49,
"items": [
[
"redo",
"Interfaccia rifatta da capo: doppio buffer senza sfarfallio, transizioni, schermata iniziale, tema notte rosso"
],
[
"ok",
"Home con Astro, Foto, Rullini, Torna alla 6D, Profili, Opzioni; HDR, focus stacking, Dual ISO, ETTR, ND, polare, calendario"
],
[
"ok",
"Simulatore con 49 test e confronto astronomico con astropy"
]
]
},
{
"v": "0.3",
"t": "Preset per il PC",
"tests": 55,
"items": [
[
"ok",
"16 preset XMP per LightCraft e Lightroom: il carattere della pellicola anche nel RAW"
],
[
"ok",
"Avviso se la macchina è in solo RAW (Iride legge la qualità ma non la cambia: Magic Lantern lo considera rischioso)"
]
]
},
{
"v": "0.4",
"t": "Animazioni dei rullini",
"tests": 65,
"items": [
[
"ok",
"Caricamento e riavvolgimento animati, scaffale con mensola, cestino che apre Iride, diagnostica dei tasti"
]
]
},
{
"v": "0.5",
"t": "Rullini disegnati",
"tests": null,
"items": [
[
"redo",
"Rullini ridisegnati con perno, coperchi zigrinati, codice DX e tre impaginazioni originali, senza loghi reali"
]
]
},
{
"v": "0.6",
"t": "Icone e apertura all'accensione",
"tests": 70,
"items": [
[
"redo",
"Icone della Home ridisegnate"
],
[
"ok",
"All'accensione la 6D mostra subito la Home di Iride"
]
]
},
{
"v": "0.7",
"t": "Cose che sbucavano sopra Iride",
"tests": 72,
"items": [
[
"err",
"In basso comparivano gli indicatori audio di Magic Lantern sopra la riga d'aiuto"
],
[
"err",
"All'avvio il menu Canon si vedeva sopra Iride"
],
[
"fix",
"Menu ML portato su Shoot › Iride, attesa del menu Canon, ridisegno completo periodico"
]
]
},
{
"v": "0.8",
"t": "Live View",
"tests": 78,
"items": [
[
"ok",
"START/STOP accende e spegne il Live View e riapre Iride sulla stessa schermata"
],
[
"err",
"In Live View la grafica di Iride poteva restare invisibile"
]
]
},
{
"v": "0.9",
"t": "Il nome Iride 6D",
"tests": 81,
"items": [
[
"mod",
"Il progetto prende il nome Iride 6D"
],
[
"err",
"Cambiando rullino si vedeva solo il riavvolgimento: gli eventi interni di Canon venivano presi per tasti"
]
]
},
{
"v": "1.0",
"t": "Animazioni ovunque",
"tests": 86,
"items": [
[
"ok",
"Diaframma all'avvio, zoom dal riquadro alla schermata, fogli che salgono, barra elastica, contapose a rullo"
],
[
"redo",
"Allineamento polare e calendario del cielo rifatti"
],
[
"fix",
"Descrizioni dei rullini verificate: la Delta 3200 è una ISO 1000 reale, la CineStill 800T vira al blu di giorno"
],
[
"err",
"Dopo Torna alla 6D l'avviso \"Fatto\" non compariva"
]
]
}
]
},
{
"when": "2 novembre · domenica",
"title": "L'astrofotografia diventa seria",
"vers": [
{
"v": "1.1",
"t": "Mai uscita come release",
"tests": 108,
"items": [
[
"redo",
"Sottomenu ridisegnati come la Home, con anteprima della voce scelta"
],
[
"ok",
"Luna disegnata dal vero con mari e 45 crateri; magnitudine limite con la Luna (Krisciunas e Schaefer)"
],
[
"err",
"Sfarfallio nello zoom tra Home e schermate"
],
[
"err",
"Fuoco stelle: HFR bloccato a 12 con la stella sfocata, ricerca sotto il pannello, pixel caldi presi per stelle"
],
[
"err",
"Testi spostati da ML sul bordo quando uscivano dallo schermo; Luna non capovolta dall'emisfero sud"
],
[
"ok",
"Controllo a tappeto con AddressSanitizer e 6000 tasti a caso"
]
]
},
{
"v": "1.2",
"t": "Intervallometro da campo",
"tests": 148,
"items": [
[
"ok",
"Fuoco automatico sulle stelle a curva a V, otturatore silenzioso, dithering, registro della sessione"
],
[
"ok",
"Luna da fotografare, Dove puntare, 57 città e fusi orari controllati col database IANA, Prova della 6D"
]
]
},
{
"v": "1.2.1",
"t": "Primo dato dalla 6D vera",
"tests": null,
"items": [
[
"err",
"Nella Prova della 6D il \"%\" spostava i valori: la temperatura mostrava i MB liberi (30387)"
],
[
"fix",
"Nuovo controllo automatico dei formati (check_formats.py) prima della compilazione"
]
]
},
{
"v": "1.2.2",
"t": "Calendario più leggero",
"tests": null,
"items": [
[
"err",
"Il calendario costava 62 ms per fotogramma: oltre 5.500 piccoli riempimenti"
],
[
"fix",
"Luna e barre disegnate a blocchi"
]
]
}
]
},
{
"when": "3 novembre · lunedì",
"title": "Prestazioni e aspetto, misurati sulla macchina",
"vers": [
{
"v": "1.2.3",
"t": "Manovella",
"tests": null,
"items": [
[
"redo",
"Manovella di riavvolgimento ridisegnata come sulle reflex a pellicola"
]
]
},
{
"v": "1.3",
"t": "Carattere e angoli",
"tests": null,
"items": [
[
"redo",
"Il testo piccolo passa dal Terminus di ML a Inter Medium proporzionale con bordi sfumati"
],
[
"ok",
"Angoli arrotondati, interruttori a pillola, 15 icone nuove, striscia di stato nella Home"
]
]
},
{
"v": "1.3.1",
"t": "",
"tests": null,
"items": [
[
"mod",
"Angoli arrotondati anche dove mancavano"
]
]
},
{
"v": "1.3.2",
"t": "Disegno a blocchi",
"tests": null,
"items": [
[
"mod",
"La memoria dello schermo della 6D è lenta un byte alla volta: testo, Luna e icone scritti a righe intere"
]
]
},
{
"v": "1.3.3",
"t": "Testo piccolo più veloce",
"tests": null,
"items": [
[
"err",
"28 µs per lettera piccola contro 20 per quelle di ML"
],
[
"fix",
"Caratteri decompressi una volta, righe vuote saltate, sfumature in cache"
],
[
"mod",
"Repository: il ramo main di GodEater58/Inquadra diventa il firmware, poi tutto si chiama Iride 6D"
]
]
}
]
},
{
"when": "5 novembre · mattina",
"title": "Il bug sulla 6D e il core di Magic Lantern",
"vers": [
{
"v": "1.3.4",
"t": "Ciclo del Live View",
"tests": 153,
"items": [
[
"err",
"Sulla 6D: dopo un cambio di rullino il Live View si accendeva e tornava sui Rullini in un ciclo fino al pulsante di scatto"
],
[
"fix",
"Iride si riapre solo se il Live View è cambiato, START/STOP doppio ignorato, 5 test nuovi"
]
]
},
{
"v": "1.4",
"t": "Magic Lantern modificato",
"tests": 158,
"items": [
[
"ok",
"Core di ML modificato: modalità schermo intero, dialogo Canon tenuto sveglio, apertura senza attese fisse"
],
[
"ok",
"Tasti \"finti\" premuti da ML riconosciuti; funzioni del core collegate deboli (va anche con ML ufficiale)"
]
]
},
{
"v": "1.4.1",
"t": "Revisione completa",
"tests": 163,
"items": [
[
"ok",
"Salvataggio in due copie (IRIDE.DAT e IRIDE.BAK) con somma di controllo"
],
[
"err",
"Numeri del rullino e della sessione sbagliati oltre il file 9999"
],
[
"err",
"Cerchio pieno disegnato con un ciclo sbagliato e uno spostamento a sinistra di un numero negativo"
],
[
"fix",
"Cerchi a righe intere, verificati pixel per pixel da raggio 0 a 170"
]
]
},
{
"v": "1.4.2",
"t": "Una notte, un file",
"tests": 165,
"items": [
[
"ok",
"Registro della sessione per notte, lavori su scheda in un task, sanitizer in GitHub Actions"
],
[
"err",
"La release v1.4.2 uscì vuota: il tag puntava a un commit con la compilazione rotta"
],
[
"err",
"CI rossa: con un seme diverso i tasti a caso non visitavano abbastanza schermate"
],
[
"err",
"La sessione non può creare tag su GitHub (errore 403): da allora le release si creano dal sito"
],
[
"fix",
"Release ricreata sul commit giusto; controllo delle schermate visitate solo col seme di partenza"
]
]
},
{
"v": "1.4.3",
"t": "Profili di partenza",
"tests": 171,
"items": [
[
"ok",
"Ritratto, Paesaggio, Street, Sport e Notte nei posti vuoti, una volta sola"
],
[
"ok",
"Diaframma adattato ai limiti dell'obiettivo, con avviso"
],
[
"err",
"L'avviso del diaframma confrontava il valore dopo averlo già cambiato"
]
]
}
]
},
{
"when": "5 novembre · pomeriggio",
"title": "La vetrina: bianco, demo nel browser, retro della 6D",
"vers": [
{
"v": "1.5",
"t": "Bilanciamento del bianco",
"tests": 180,
"items": [
[
"ok",
"Il tasto WB che manca alla 6D, anche col SET dalla schermata di scatto"
],
[
"mod",
"Avvio appena ML è pronto (circa 1 s invece di 2,5)"
]
]
},
{
"v": "1.5.1",
"t": "Demo nel browser",
"tests": 180,
"items": [
[
"ok",
"Lo stesso codice di Iride compilato in WebAssembly e pubblicato su GitHub Pages"
],
[
"redo",
"Simulatore coi caratteri RBF veri di Magic Lantern"
],
[
"err",
"Coi caratteri veri: 4 testi uscivano dal loro spazio (piede della Home, Dove puntare, Bilanciamento, nota del Live View)"
]
]
},
{
"v": "demo",
"t": "Correzioni della demo",
"tests": null,
"items": [
[
"err",
"La freccia giù, dopo averla premuta, saltava in alto a sinistra"
],
[
"redo",
"Retro della 6D rifatto due volte: prima generico, poi ricalcato sulla foto ufficiale Canon"
],
[
"err",
"Il mirino non somigliava a quello della 6D: ridisegnato con pentaprisma, conchiglia e diottrie"
],
[
"err",
"START/STOP non faceva vedere nulla: ora il Live View resta visibile circa 2 s"
],
[
"err",
"Su telefono la pagina scorreva di lato"
]
]
},
{
"v": "1.5.2",
"t": "Tema notte completo",
"tests": 181,
"items": [
[
"err",
"Luna da fotografare e Dove puntare restavano bianche e arancioni col tema notte in Astro"
],
[
"ok",
"Demo con salvataggio nel browser e pulsante per ripartire da capo"
]
]
}
]
},
{
"when": "5 novembre · sera",
"title": "1.6: idee dalle ricerche online",
"vers": [
{
"v": "1.6",
"t": "Fai tutto",
"tests": 211,
"items": [
[
"ok",
"Dark proposti a fine sessione con la stessa posa, ISO e temperatura"
],
[
"ok",
"Avviso della riduzione disturbo lunghe esposizioni, riconosciuta dal tempo dello scatto"
],
[
"ok",
"Fino a 4 pellicole tue (ricette) con scheda di testo da condividere e preset XMP sul PC"
],
[
"ok",
"Interfaccia in inglese: circa 800 testi, registri sempre in italiano"
],
[
"ok",
"Piano di stacking per AstroStack, release con un clic, lista di prova sulla 6D"
],
[
"ok",
"Demo: stella da mettere a fuoco in Live View e PLAY per rivedere le foto"
],
[
"err",
"I test di apertura fallivano: la proposta dei dark si apriva al posto della Home"
],
[
"err",
"La macro della traduzione si applicava anche alla definizione di NotifyBox nel simulatore"
],
[
"err",
"In inglese il registro della sessione avrebbe scritto \"silent\" invece di \"silenzioso\", illeggibile per AstroStack"
],
[
"err",
"L'estrazione dei testi saltava tutte le righe con un tasto (MODULE_KEY): \"Profilo cancellato\" restava in italiano"
],
[
"fix",
"Scrittura dei file senza traduzione, test in inglese su tutte le schermate: nessun testo fuori dallo schermo"
]
]
}
]
}
],
"versioni": 28,
"prima": "0.1",
"ultima": "1.6",
"giorni": 5,
"righe": 11426,
"test": 211
};
