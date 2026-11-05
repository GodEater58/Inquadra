/* film.inc - pellicole, rullino (carica, pausa, riprendi, riavvolgi, push/pull),
 * contatore delle pose, contatore nel Live View, disegno e animazioni dei rullini */

enum { CAT_COLOR, CAT_SLIDE, CAT_BW, CAT_CINE, CAT_COUNT };
static const char *const cat_names[CAT_COUNT] = { "COLORE", "DIA", "B/N", "CINE" };

struct film
{
    const char *brand, *name, *desc, *use;   /* carattere e uso ideale (due righe) */
    int cat, iso;
    const char *process;
    int style, contrast, saturation, tone, kelvin;
    int body, label, ink;
};

/* approssimazione del carattere della pellicola sul JPEG: il RAW resta neutro */
static const struct film films[] = {
    { "KODAK",    "Portra 160",
        "Grana finissima, colori tenui, pelle naturale", "Ritratti e moda con tanta luce o col flash",
        CAT_COLOR, 160,  "C-41", PS_PORTRAIT,  -1, -1,  0, 5200, COLOR_ALMOST_WHITE, COLOR_ORANGE,   COLOR_BLACK },
    { "KODAK",    "Portra 400",
        "Colori naturali, contrasto basso, pelle fedele", "La piu' versatile; sovraesposta va sul pastello",
        CAT_COLOR, 400,  "C-41", PS_PORTRAIT,  -1,  0,  1, 5300, COLOR_ALMOST_WHITE, COLOR_ORANGE,   COLOR_BLACK },
    { "KODAK",    "Portra 800",
        "Colori vividi, contrasto basso, grana visibile", "Per la poca luce: interni, sera, concerti",
        CAT_COLOR, 800,  "C-41", PS_PORTRAIT,   0,  1,  1, 5300, COLOR_ALMOST_WHITE, COLOR_DARK_RED, COLOR_WHITE },
    { "KODAK",    "Ektar 100",
        "Grana finissima, colori saturi e nitidi", "Paesaggi e viaggi; sulla pelle calca i rossi",
        CAT_COLOR, 100,  "C-41", PS_LANDSCAPE,  1,  2,  0, 5200, COLOR_ORANGE,       COLOR_RED,      COLOR_WHITE },
    { "KODAK",    "Gold 200",
        "Toni caldi e dorati, grana ben visibile", "Giornate di sole, vacanze, sapore nostalgico",
        CAT_COLOR, 200,  "C-41", PS_STD,        0,  1,  2, 6000, COLOR_YELLOW,       COLOR_ORANGE,   COLOR_BLACK },
    { "KODAK",    "UltraMax 400",
        "Colori brillanti ed equilibrati, un po' caldi", "Tuttofare economica: sole, nuvole, interni",
        CAT_COLOR, 400,  "C-41", PS_STD,        1,  1,  1, 5600, COLOR_YELLOW,       COLOR_BLUE,     COLOR_WHITE },
    { "FUJIFILM", "Superia 400",
        "Palette fresca, verdi intensi, grana visibile", "Tutti i giorni; rende bene col cielo grigio",
        CAT_COLOR, 400,  "C-41", PS_STD,        1,  1, -1, 4900, COLOR_GREEN2,       COLOR_ALMOST_WHITE, COLOR_BLACK },
    { "FUJIFILM", "C200",
        "Toni freschi e neutri, blu e verdi puliti", "Economica, rende al meglio in piena luce",
        CAT_COLOR, 200,  "C-41", PS_STD,        0,  0, -1, 5000, COLOR_GREEN1,       COLOR_ALMOST_WHITE, COLOR_BLACK },
    { "FUJIFILM", "Velvia 50",
        "Dia ipersatura, contrasto alto, nitidissima", "Paesaggi in luce morbida: tolleranza minima",
        CAT_SLIDE, 50,   "E-6",  PS_LANDSCAPE,  3,  4,  0, 5200, COLOR_DARK_RED,     COLOR_ALMOST_WHITE, COLOR_BLACK },
    { "FUJIFILM", "Provia 100F",
        "Dia neutra e precisa, colori pieni, grana fine", "Tuttofare in dia, regge anche la luce dura",
        CAT_SLIDE, 100,  "E-6",  PS_STD,        2,  1,  0, 5200, COLOR_BLUE,         COLOR_ALMOST_WHITE, COLOR_BLACK },
    { "KODAK",    "Ektachrome E100",
        "Dia pulita e neutra, con tendenza fredda", "Viaggi e citta'; vuole un'esposizione precisa",
        CAT_SLIDE, 100,  "E-6",  PS_STD,        2,  1, -1, 5000, COLOR_LIGHT_BLUE,   COLOR_YELLOW,   COLOR_BLACK },
    { "ILFORD",   "HP5 Plus",
        "B/N morbido, ombre ricche, grana classica", "Si tira bene fino a 3200: strada e interni",
        CAT_BW,    400,  "B/N",  PS_MONO,       1,  0,  0, 5200, COLOR_GRAY(80),     COLOR_BLACK,    COLOR_WHITE },
    { "KODAK",    "Tri-X 400",
        "B/N contrastato, neri pieni, grana grintosa", "Il classico del reportage e della strada",
        CAT_BW,    400,  "B/N",  PS_MONO,       3,  0,  0, 5200, COLOR_YELLOW,       COLOR_BLACK,    COLOR_WHITE },
    { "ILFORD",   "Delta 3200",
        "B/N per il buio, grana marcata e atmosfera", "ISO reale 1000: a 3200 e' gia' tirata",
        CAT_BW,    3200, "B/N",  PS_MONO,       2,  0,  0, 5200, COLOR_GRAY(70),     COLOR_DARK_RED, COLOR_WHITE },
    { "CINESTILL","800T",
        "Cinema al tungsteno, aloni rossi sulle luci", "Notte, neon e interni; di giorno vira al blu",
        CAT_CINE,  800,  "C-41", PS_STD,        0,  1,  0, 3200, COLOR_DARK_RED,     COLOR_GRAY(20), COLOR_WHITE },
    { "CINESTILL","50D",
        "Cinema diurna, grana finissima, colori puliti", "Sole e paesaggi; lievi aloni sulle luci forti",
        CAT_CINE,  50,   "C-41", PS_STD,        1,  1,  0, 5500, COLOR_LIGHT_BLUE,   COLOR_GRAY(20), COLOR_WHITE },
};
#define NUM_FILMS ((int)COUNT(films))

static const struct film *cur_film() { return &films[COERCE(P.roll_film, 0, NUM_FILMS - 1)]; }

/* Il carattere della pellicola (Stile Foto) finisce solo nei JPEG e nell'anteprima del CR2:
   in solo RAW Lightroom/LightCraft mostrano l'anteprima e poi il RAW neutro.
   Qui la qualita' si LEGGE soltanto: cambiarla dal firmware e' segnalato da Magic Lantern
   come pericoloso (FEATURE_PICQ_DANGEROUS, "can do permanent damage"). */
static int picq_raw_only() { return (pic_quality & 0x60000) && !(pic_quality & 0x10000); }
#define RAW_HINT "Solo RAW: metti RAW+L per il look nel JPEG"

/* ISO di esposizione: nominale x 2^push, nei limiti della 6D senza estensioni */
static int film_iso_exposed(const struct film *fm, int push)
{
    int iso = fm->iso;
    for (int i = 0; i < push; i++) iso *= 2;
    for (int i = 0; i > push; i--) iso /= 2;
    return COERCE(iso, 100, 25600);
}

static int current_file_number()
{
    struct card_info *c = get_shooting_card();
    return c ? c->file_number : 0;
}

/* applica il carattere della pellicola (ricordando le impostazioni di prima) */
static void film_activate()
{
    const struct film *fm = cur_film();
    snap_take();
    style_set_sliders(fm->style, fm->contrast, fm->saturation, fm->tone);
    int raw = val2raw_iso(film_iso_exposed(fm, P.roll_push));
    if (raw > 0) lens_set_rawiso(raw);
    lens_set_kelvin(fm->kelvin);
    if (opt_review_off)
    {
        int off = 0;
        prop_request_change(PROP_IMAGE_REVIEW_TIME, &off, 4);
    }
    P.roll_paused = 0;
    P.roll_last_fn = current_file_number();
    persist_touch();
    prefix_refresh();
}

static void roll_log(const char *event)
{
    const struct film *fm = cur_film();
    char line[128];
    snprintf(line, sizeof(line), "%s,R%03d,%s %s,%d,%s%d,%d,%d,%d,%d",
        event, P.roll_num % 1000, fm->brand, fm->name, fm->iso, P.roll_push > 0 ? "+" : "", P.roll_push,
        P.roll_exp, P.roll_shots, P.roll_first_fn, P.roll_last_fn);
    log_line(line);
}

static void roll_load(int film, int exposures, int push)
{
    P.roll_num++;
    P.roll_film = film;
    P.roll_exp = exposures;
    P.roll_push = push;
    P.roll_shots = 0;
    P.roll_first_fn = MOD(current_file_number(), 9999) + 1;   /* dopo 9999 viene 0001 */
    P.roll_loaded = 1;
    film_activate();
    roll_log("CARICATO");
}

static void roll_pause()
{
    if (!roll_active()) return;
    restore_camera();
    P.roll_paused = 1;
    persist_touch();
    prefix_refresh();
}

static void roll_resume()
{
    if (!P.roll_loaded || !P.roll_paused) return;
    film_activate();
}

static void roll_rewind()
{
    if (!P.roll_loaded) return;
    roll_log("RIAVVOLTO");
    if (!P.roll_paused) restore_camera();
    P.roll_loaded = 0;
    P.roll_paused = 0;
    persist_touch();
    prefix_refresh();
}

/* ------------------------------------------------------------------ contatore (ogni secondo) */

/* lavori lenti su scheda (prefisso dei file, salvataggio, registro della sessione): girano in un
   task di Iride, non nel task dell'orologio di Magic Lantern, che serve anche alle altre
   funzioni di ML ogni secondo e non deve aspettare la scheda */
static void iride_bg_tick()
{
    if (prefix_pending && !app_running && job_running == JOB_NONE)
    {
        prefix_pending = 0;
        prefix_refresh();
    }
    /* scrittura su scheda solo a macchina ferma da 8 s: mai mentre Canon salva le foto */
    if (persist_dirty && !app_running && job_running == JOB_NONE && lens_info.job_state == 0 &&
        get_ms_clock() - persist_touch_ms > 8000)
        persist_save();

    /* scatti dal cavo (per esempio dalla montatura) nel registro della sessione */
    session_watch_external(current_file_number());
}

static void iride_bg_task(void *unused)
{
    TASK_LOOP
    {
        if (ml_shutdown_requested) break;
        iride_bg_tick();
        msleep(1000);
    }
}

/* ogni secondo, nel task dell'orologio di ML: solo il contatore del rullino (veloce) */
static unsigned int iride_clock(unsigned int ctx)
{
    if (!P.roll_loaded) return CBR_RET_CONTINUE;
    int fn = current_file_number();
    if (fn <= 0) return CBR_RET_CONTINUE;
    if (!roll_active() || job_running != JOB_NONE)
    {
        /* in pausa o durante i lavori astro le foto non consumano il rullino */
        if (P.roll_last_fn != fn) { P.roll_last_fn = fn; persist_touch(); }
        return CBR_RET_CONTINUE;
    }
    int delta = fn - P.roll_last_fn;
    if (delta == 0) return CBR_RET_CONTINUE;
    if (delta < 0) delta += 9999;
    P.roll_last_fn = fn;
    persist_touch();
    if (delta > 100) return CBR_RET_CONTINUE;   /* scheda cambiata o numerazione azzerata */

    int before = P.roll_shots;
    P.roll_shots = MIN(P.roll_shots + delta, 999);
    if (app_running) dirty = 1;

    if (before < P.roll_exp && P.roll_shots >= P.roll_exp)
    {
        beep_times(3);
        NotifyBox(5000, "Rullino %03d finito: riavvolgi da Iride", P.roll_num % 1000);
    }
    else if (P.roll_shots > P.roll_exp)
    {
        beep();
        NotifyBox(2500, "FUORI RULLINO  %d/%d", P.roll_shots, P.roll_exp);
    }
    else if (opt_wind)
    {
        NotifyBox(1500, "%s  %d/%d", cur_film()->name, P.roll_shots, P.roll_exp);
    }
    return CBR_RET_CONTINUE;
}

/* contatore nella barra informazioni del Live View */
static LVINFO_UPDATE_FUNC(roll_info_update)
{
    LVINFO_BUFFER(24);
    if (!roll_active()) return;
    snprintf(buffer, 24, "R%03d %d/%d", P.roll_num % 1000, P.roll_shots, P.roll_exp);
    if (P.roll_shots >= P.roll_exp) item->color_fg = COLOR_RED;
    else if (P.roll_exp - P.roll_shots <= 3) item->color_fg = COLOR_YELLOW;
}

static struct lvinfo_item roll_info_items[] = {
    {
        .name = "Iride rullino",
        .which_bar = LV_PREFER_TOP_BAR,
        .update = roll_info_update,
        .preferred_position = 2,
        .priority = 1,
    },
};

static void film_init()
{
    /* lo stato e' gia' in P: la pellicola deve esistere in questa versione */
    P.roll_film = COERCE(P.roll_film, 0, NUM_FILMS - 1);
}

/* ------------------------------------------------------------------ disegno dei rullini */

/* grafica originale di ogni rullino (solo i colori richiamano la pellicola: niente loghi) */
enum { ART_BAND, ART_SPLIT, ART_STRIPES };
static const unsigned char film_art[] = {
    ART_BAND, ART_BAND, ART_BAND, ART_SPLIT, ART_STRIPES, ART_SPLIT, ART_BAND, ART_STRIPES,
    ART_BAND, ART_BAND, ART_SPLIT, ART_BAND, ART_STRIPES, ART_SPLIT, ART_BAND, ART_BAND,
};

static int film_index(const struct film *fm) { return COERCE((int)(fm - films), 0, NUM_FILMS - 1); }

/* colore dei rullini: di giorno quello vero; di notte tre livelli (rosso, rosso scuro, nero)
   cosi' fascia, coperchi e ombre restano distinguibili */
static int cc(int c)
{
    if (!night || c == COLOR_EMPTY) return c;
    if (c >= COLOR_GRAY(0) && c <= COLOR_GRAY(100))
    {
        int g = (c - 38) * 100 / 41;
        return g < 20 ? COLOR_BLACK : g < 66 ? COLOR_DARK_RED : COLOR_RED;
    }
    if (c == COLOR_BLACK) return COLOR_BLACK;
    if (c == COLOR_WHITE || c == COLOR_ALMOST_WHITE || c == COLOR_YELLOW || c == COLOR_CYAN ||
        c == COLOR_ORANGE || c == COLOR_GREEN2 || c == COLOR_LIGHT_BLUE) return COLOR_RED;
    return COLOR_DARK_RED;
}

/* di notte la fascia prende il tono opposto al corpo */
static int night_label(const struct film *fm)
{
    if (!night || cc(fm->label) != cc(fm->body)) return fm->label;
    return cc(fm->body) == COLOR_RED ? COLOR_GRAY(40) : COLOR_WHITE;
}

/* colore chiaro? (per scegliere testo nero o bianco sopra) */
static int is_light(int c)
{
    if (c == COLOR_WHITE || c == COLOR_ALMOST_WHITE || c == COLOR_YELLOW || c == COLOR_CYAN || c == COLOR_ORANGE) return 1;
    return c >= COLOR_GRAY(55) && c <= COLOR_GRAY(100);
}
static int ink_on(int c) { return is_light(c) ? COLOR_BLACK : COLOR_WHITE; }

/* testo piccolo centrato, solo se ci sta */
static void can_text(int font, int color, int cx, int y, int maxw, const char *s)
{
    if (txt_w(font, s) <= maxw) txt_c(font, color, cx, y, s);
}

/* coperchio metallico: zigrinatura, riflesso, ombra e giunzione */
static void draw_cap(int x, int y, int w, int h, int fine)
{
    fill(cc(COLOR_GRAY(60)), x, y, w, h);
    if (h < 4) return;
    fill(cc(COLOR_GRAY(80)), x + 1, y, w - 2, 1);                       /* bordo alto in luce */
    fill(cc(COLOR_GRAY(30)), x, y + h - 1, w, 1);                       /* giunzione col corpo */
    if (fine)
        for (int xx = x + 2; xx < x + w - 2; xx += 3)
            fill(cc(xx < x + w * 7 / 10 ? COLOR_GRAY(48) : COLOR_GRAY(36)), xx, y + 2, 1, h - 4);
    fill(cc(COLOR_GRAY(86)), x + w * 18 / 100, y + 1, MAX(w / 16, 1), h - 2);   /* riflesso */
    fill(cc(COLOR_GRAY(34)), x + w - MAX(w / 9, 2), y + 1, MAX(w / 9, 2), h - 2); /* lato in ombra */
}

/* codice DX (due file di contatti argento/neri) e codice a barre: decorativi, dall'ISO */
static void draw_dx(const struct film *fm, int x, int y, int cw, int ch)
{
    unsigned bits = (unsigned)fm->iso * 2654435761u;
    for (int r = 0; r < 2; r++)
        for (int c = 0; c < 6; c++)
        {
            int on = c == 0 || ((bits >> (r * 6 + c)) & 1);
            fill(cc(on ? COLOR_GRAY(74) : COLOR_BLACK), x + c * (cw + 1), y + r * (ch + 1), cw, ch);
        }
}

static void draw_barcode(const struct film *fm, int x, int y, int w, int h)
{
    fill(cc(COLOR_WHITE), x, y, w, h);
    unsigned bits = (unsigned)(fm->iso + 7) * 2246822519u ^ (unsigned)film_index(fm) * 3266489917u;
    int xx = x + 2;
    for (int i = 0; xx < x + w - 2; i++)
    {
        int bw = ((bits >> (i % 30)) & 1) ? 2 : 1;
        if (i & 1) fill(COLOR_BLACK, xx, y + 1, bw, h - 2);
        xx += bw;
    }
}

/* ombra del cilindro: per ogni colore il tono piu' scuro che esiste nella tavolozza Canon;
   -1 = nessun tono adatto, si usa una retinatura leggera */
static int shade_of(int c, int level)
{
    if (night) return -1;
    if (c == COLOR_WHITE || c == COLOR_ALMOST_WHITE) return level == 1 ? COLOR_GRAY(84) : COLOR_GRAY(68);
    if (c >= COLOR_GRAY(0) && c <= COLOR_GRAY(100))
    {
        int g = (c - 38) * 100 / 41;
        return COLOR_GRAY(MAX(g - (level == 1 ? 12 : 26), 0));
    }
    switch (c)
    {
        case COLOR_YELLOW:     return level == 1 ? COLOR_ORANGE : -1;
        case COLOR_ORANGE:     return -1;
        case COLOR_RED:        return level == 1 ? COLOR_DARK_RED : -1;
        case COLOR_GREEN2:     return level == 1 ? COLOR_GREEN1 : -1;
        case COLOR_LIGHT_BLUE: return level == 1 ? COLOR_BLUE : -1;
        case COLOR_CYAN:       return level == 1 ? COLOR_LIGHT_BLUE : COLOR_BLUE;
        case COLOR_BLACK:      return COLOR_BLACK;
    }
    return -1;
}

/* fascia orizzontale del cilindro: colore pieno, ombra a destra, riflesso a sinistra */
static void cyl_zone(int x, int y, int w, int h, int c)
{
    if (h <= 0) return;
    fill(cc(c), x, y, w, h);
    if (w < 8 || c == COLOR_BLACK) return;
    int a = x + w * 70 / 100, b = x + w * 87 / 100, e = x + w - 1;
    int s1c = shade_of(c, 1);
    if (s1c >= 0) fill(cc(s1c), b, y, e - b, h);   /* la zona piu' scura parte dal primo tono */
    for (int tone = 1; tone <= 2; tone++)
    {
        int x0 = tone == 1 ? a : b, x1 = tone == 1 ? b : e;
        int sc = shade_of(c, tone);
        if (sc >= 0) fill(cc(sc), x0, y, x1 - x0, h);
        else
        {
            /* retinatura sopra il tono precedente (o il colore pieno) */
            int step = tone == 1 ? 3 : 2;
            for (int i = x0; i < x1; i += step) fill(COLOR_BLACK, i, y, 1, h);
        }
    }
    fill(COLOR_BLACK, e, y, 1, h);                                   /* bordi */
    int s1 = shade_of(c, 1);
    fill(s1 >= 0 ? cc(s1) : COLOR_BLACK, x, y, 1, h);
    if (c != COLOR_WHITE && c != COLOR_ALMOST_WHITE)                 /* riflesso */
    {
        int hx = x + w * 16 / 100;
        fill(cc(COLOR_WHITE), hx, y, 1, h);
        if (w >= 40) fill(cc(COLOR_ALMOST_WHITE), hx + 2, y, 1, h);
    }
}

static int canister_height(int scale) { return 3 * MAX(10 * scale / 100, 3) + 126 * scale / 100; }

static void draw_canister(const struct film *fm, int cx, int top, int scale, int leader)
{
    int w = 76 * scale / 100;
    int h = 126 * scale / 100;
    int x = cx - w / 2;
    int cap = MAX(10 * scale / 100, 3);
    int fine = scale >= 78;                     /* dettagli fini solo da vicino */
    int label = cc(fm->label), ink = cc(fm->ink);
    int art = film_art[film_index(fm)];
    int by = top + 2 * cap;
    int d = MAX(2 * scale / 100, 1);

    /* perno della bobina con la sua tacca */
    int sw = MAX(18 * scale / 100, 4);
    fill(cc(COLOR_GRAY(50)), cx - sw / 2, top + cap / 3, sw, cap - cap / 3);
    fill(cc(COLOR_GRAY(72)), cx - sw / 2 + 1, top + cap / 3, MAX(sw / 5, 1), cap - cap / 3);
    if (fine) fill(cc(COLOR_GRAY(22)), cx - 1, top + cap / 3, 2, cap / 2);

    draw_cap(x - d, top + cap, w + 2 * d, cap, fine);

    /* corpo e grafica: ogni fascia ha la sua ombra */
    int band_y = by + h * 32 / 100, band_h = h * 36 / 100;
    int t = MAX(h / 60, 1);
    if (art == ART_SPLIT)
    {
        int sy = by + h * 58 / 100;
        cyl_zone(x, by, w, sy - by, fm->body);
        cyl_zone(x, sy, w, by + h - sy, night_label(fm));
        cyl_zone(x, sy - t, w, t, COLOR_WHITE);
        band_y = by + h * 24 / 100; band_h = h * 32 / 100;
    }
    else
    {
        cyl_zone(x, by, w, h, fm->body);
        cyl_zone(x, band_y, w, band_h, night_label(fm));
        cyl_zone(x, band_y - 3 * t, w, t, night_label(fm));                 /* filetti */
        cyl_zone(x, band_y + band_h + 2 * t, w, t, night_label(fm));
        if (art == ART_STRIPES)
            for (int i = 0; i < 3; i++)
                cyl_zone(x, by + h * 6 / 100 + i * MAX(h / 22, 2), w, t, night_label(fm));
    }

    /* bassa: codice DX e codice a barre (oppure un accenno da lontano) */
    if (fine)
    {
        int cw = MAX(w / 16, 2), ch = MAX(h / 34, 2);
        draw_dx(fm, x + w * 8 / 100, by + h - 2 * ch - h * 6 / 100, cw, ch);
        int bw = w * 26 / 100;
        draw_barcode(fm, x + w * 50 / 100, by + h - h * 6 / 100 - 2 * ch - 1, bw, 2 * ch + 1);
    }
    else if (scale >= 50)
        fill(cc(COLOR_GRAY(74)), x + w * 10 / 100, by + h * 86 / 100, w * 30 / 100, MAX(h / 30, 1));

    /* fessura di velluto da cui esce la pellicola */
    int lipw = MAX(4 * scale / 100, 2);
    fill(COLOR_BLACK, x + w - lipw, by + h * 14 / 100, lipw, h * 72 / 100);
    if (fine) fill(cc(COLOR_GRAY(14)), x + w - lipw / 2 - 1, by + h * 16 / 100, 1, h * 68 / 100);

    /* scritte */
    int tw = w - lipw - 6;
    int tcx = cx - lipw / 2;
    if (fine)
    {
        int ty = art == ART_STRIPES ? by + h * 18 / 100 - 4 : by + h * 5 / 100;
        int c_top = ink_on(fm->body);
        if (art != ART_STRIPES) can_text(FONT_SMALL, cc(c_top), tcx, ty, tw, fm->brand);
        char nm[16];
        int i = 0;
        while (fm->name[i] && fm->name[i] != ' ' && i < 15) { nm[i] = fm->name[i]; i++; }
        nm[i] = 0;
        int ny = art == ART_STRIPES ? ty : ty + 13;
        if (band_y - ny >= 13) can_text(FONT_SMALL, cc(c_top), tcx, ny, tw, nm);
        char proc[12];
        snprintf(proc, sizeof(proc), "%s", fm->process);
        int c_low = ink_on(art == ART_SPLIT ? fm->label : fm->body);
        can_text(FONT_SMALL, cc(c_low), tcx, by + h * 70 / 100, tw, proc);
    }
    char iso[8];
    snprintf(iso, sizeof(iso), "%d", fm->iso);
    int font = scale >= 90 ? FONT_LARGE : scale >= 60 ? FONT_MED : FONT_SMALL;
    int fh = scale >= 90 ? 32 : scale >= 60 ? 23 : 12;
    int isoc = art == ART_SPLIT ? cc(ink_on(fm->body)) : (ink == label ? C_TEXT : ink);
    if (night) isoc = cc(art == ART_SPLIT ? fm->body : night_label(fm)) == COLOR_RED ? COLOR_BLACK : COLOR_RED;
    if (txt_w(font, iso) > tw && font != FONT_SMALL) { font = font == FONT_LARGE ? FONT_MED : FONT_SMALL; fh = font == FONT_MED ? 23 : 12; }
    txt_c(font, isoc, tcx, band_y + (band_h - fh) / 2, iso);

    draw_cap(x - d, by + h, w + 2 * d, cap, fine);

    if (leader > 0)
    {
        /* linguetta: pellicola perforata che esce dalla fessura, taglio a mezza altezza */
        int lx = x + w - lipw / 2;
        int ly = by + h * 30 / 100;
        int lh = h * 40 / 100;
        int tongue = MIN(leader * 6 / 10, 26 * scale / 100);
        int full = leader - tongue;
        int base = cc(COLOR_GRAY(22)), rim = cc(COLOR_GRAY(40)), hole = cc(COLOR_GRAY(72));
        fill(base, lx, ly, full, lh);
        fill(base, lx + full, ly, tongue - 3, lh / 2 + 2);
        fill(base, lx + full + tongue - 3, ly, 3, lh / 2 - 3);          /* smusso */
        fill(rim, lx, ly, leader - 3, 1);
        fill(rim, lx, ly + lh - 1, full, 1);
        int hs = MAX(4 * scale / 100, 2), pitch = hs + MAX(3 * scale / 100, 2);
        for (int px = lx + 3; px + hs < lx + leader - 3; px += pitch)
        {
            fill(hole, px, ly + 3, hs, hs + 1);
            if (px + hs < lx + full) fill(hole, px, ly + lh - hs - 4, hs, hs + 1);
        }
    }
}

/* icona Rullini della Home: il rullino attuale con la pellicola che esce e tre fotogrammi
   con mini-foto (paesaggio, tramonto, cielo stellato). Area 200 x 100 da (x, y).
   t >= 0: la pellicola rientra di scatto nel rullino e riesce, come quando la si carica */
static void icon_rolls(const struct film *fm, int x, int y, int t)
{
    const int scale = 62;
    int cx = x + 50, top = y + 4;
    int h = 126 * scale / 100, cap = MAX(10 * scale / 100, 3);
    int by = top + 2 * cap;
    int sy = by + h * 30 / 100, sh = h * 40 / 100;
    int sx0 = cx + 76 * scale / 100 / 2 - 1, sx1 = x + 198;
    int base = cc(COLOR_GRAY(22)), hole = cc(COLOR_GRAY(72));

    /* fine della striscia: rientra (0..260 ms) e riesce con un piccolo rimbalzo */
    int ex = sx1;
    if (t >= 0 && t < 820)
    {
        int k = t < 260 ? ease_k(t, 260, EZ_IN) : 1000 - ease_k(t - 300, 520, EZ_BACK);
        ex = sx1 - (sx1 - sx0 - 18) * COERCE(k, -40, 1000) / 1000;
    }
    if (ex > sx0 + 18)
    {
        /* striscia con taglio finale a mezza altezza */
        fill(base, sx0, sy, ex - sx0 - 16, sh);
        fill(base, ex - 16, sy, 16, sh / 2 + 1);
        for (int px = sx0 + 3; px < ex - 3; px += 6)
        {
            fill(hole, px, sy + 2, 3, 3);
            if (px < ex - 18) fill(hole, px, sy + sh - 5, 3, 3);
        }
        /* fotogrammi con le mini-foto (solo quelli gia' fuori) */
        int fy = sy + 7, fh = sh - 14, fw = fh * 3 / 2;
        for (int k = 0; k < 3; k++)
        {
            int fx = sx0 + 10 + k * (fw + 6);
            if (fx + fw > ex - 18) break;
            if (k == 0)
            {
                fill(cc(COLOR_LIGHT_BLUE), fx, fy, fw, fh / 2);
                fill(cc(COLOR_GREEN1), fx, fy + fh / 2, fw, fh - fh / 2);
                fill(cc(COLOR_GREEN2), fx + fw / 3, fy + fh / 2 - 3, fw / 3, 3);
            }
            else if (k == 1)
            {
                fill(cc(COLOR_ORANGE), fx, fy, fw, fh * 6 / 10);
                fill(cc(COLOR_DARK_RED), fx, fy + fh * 6 / 10, fw, fh - fh * 6 / 10);
                disc(fx + fw / 2, fy + fh * 6 / 10, 4, cc(COLOR_YELLOW));
                fill(cc(COLOR_DARK_RED), fx, fy + fh * 6 / 10 + 1, fw, fh - fh * 6 / 10 - 1);
            }
            else
            {
                fill(COLOR_BLACK, fx, fy, fw, fh);
                for (int i = 0; i < 9; i++) fill(cc(COLOR_WHITE), fx + 2 + (i * 7) % (fw - 3), fy + 1 + (i * 5) % (fh - 2), 1, 1);
            }
        }
    }
    draw_canister(fm, cx, top, scale, 0);
}

/* ------------------------------------------------------------------ scena "dorso aperto" */

/* geometria: la 6D vista da dietro con lo sportello aperto, come una reflex a pellicola */
#define SC_CY       170     /* asse della pellicola */
#define SC_FILM_H   84      /* altezza della striscia (35 mm in scala col rullino) */
#define SC_CAN_X    124     /* rullino nel vano di sinistra */
#define SC_CAN_TOP  87
#define SC_LIP_X    (SC_CAN_X + 36)
#define SC_GATE_X   345     /* finestrella 36x24 */
#define SC_DRUM_X   598     /* bobina di avvolgimento (cilindro verticale) */
#define SC_DRUM_R   30
#define SC_X0       36      /* cornice della scena */
#define SC_Y0       62
#define SC_X1       684
#define SC_Y1       280

static int film_edge(const struct film *fm)
{
    /* filo di colore sul bordo: richiama la stampa sul bordo di ogni processo */
    if (fm->cat == CAT_BW) return tc(COLOR_GRAY(70));
    if (fm->cat == CAT_SLIDE) return tc(COLOR_LIGHT_BLUE);
    if (fm->cat == CAT_CINE) return tc(COLOR_DARK_RED);
    return tc(COLOR_ORANGE);
}

/* striscia da x0 a x1. off: scorrimento in pixel; tongue: linguetta tagliata in testa;
   frames: fotogrammi gia' esposti visibili */
static void draw_strip(const struct film *fm, int x0, int x1, int off, int tongue, int frames)
{
    if (x1 - x0 < 2) return;
    const int h = SC_FILM_H, y = SC_CY - h / 2;
    const int hw = 6, hh = 8, pitch = 11, m = 6;
    int base = tc(COLOR_GRAY(16)), hole = tc(COLOR_GRAY(62)), edge = film_edge(fm);
    int body_end = tongue ? MAX(x0, x1 - 46) : x1;

    fill(base, x0, y, body_end - x0, h);
    if (tongue && x1 > body_end)
    {
        /* linguetta: mezza altezza superiore con lo smusso finale */
        fill(base, body_end, y, x1 - body_end - 10, h / 2 + 4);
        fill(base, x1 - 10, y, 10, h / 2 - 10);
    }
    /* fotogrammi 24x36 tra le file di fori */
    if (frames)
    {
        int fy = y + m + hh + 7, fh = h - 2 * (m + hh + 7);
        int fw = fh * 3 / 2, fp = fw + 9;
        int pf = MOD(off, fp);
        for (int px = x0 + pf - fp; px < body_end; px += fp)
        {
            int a = MAX(px + 4, x0), b = MIN(px + 4 + fw, body_end);
            if (b > a) fill(tc(COLOR_GRAY(6)), a, fy, b - a, fh);
        }
    }
    /* perforazioni: si muovono con la pellicola */
    int ph = MOD(off, pitch);
    for (int px = x0 + ph - pitch; px < x1; px += pitch)
    {
        int a = MAX(px, x0), b = MIN(px + hw, x1);
        if (b > a) fill(hole, a, y + m, b - a, hh);
        b = MIN(px + hw, body_end);
        if (b > a) fill(hole, a, y + h - m - hh, b - a, hh);
    }
    fill(edge, x0, y + 2, body_end - x0, 1);
    fill(edge, x0, y + h - 3, body_end - x0, 1);
}

/* interno: cornice, vani, binari e finestrella dell'otturatore */
static void draw_chamber()
{
    ring_r(tc(COLOR_GRAY(30)), SC_X0, SC_Y0, SC_X1 - SC_X0, SC_Y1 - SC_Y0, R_CARD + 2, 2);
    fill(tc(COLOR_GRAY(8)), 62, 74, 124, 196);                 /* vano del rullino */
    fill(tc(COLOR_GRAY(8)), SC_DRUM_X - 56, 74, 112, 196);     /* vano della bobina */
    /* binari della pellicola */
    fill(tc(COLOR_GRAY(30)), 200, SC_CY - SC_FILM_H / 2 - 9, 330, 4);
    fill(tc(COLOR_GRAY(30)), 200, SC_CY + SC_FILM_H / 2 + 5, 330, 4);
    /* finestrella con le lamelle dell'otturatore */
    int ww = 108, wh = 72, x = SC_GATE_X - ww / 2, y = SC_CY - wh / 2;
    fill(tc(COLOR_GRAY(24)), x - 4, y - 4, ww + 8, wh + 8);
    fill(tc(COLOR_GRAY(12)), x, y, ww, wh);
    for (int ly = y + 9; ly < y + wh; ly += 12) fill(tc(COLOR_GRAY(20)), x, ly, ww, 2);
}

/* bobina vista di fianco: cilindro che gira (righe che scorrono), con la pellicola avvolta */
static void draw_drum(int turn_milli, int wound)
{
    int cx = SC_DRUM_X, r = SC_DRUM_R, top = SC_CY - 60, h = 120;
    fill(tc(COLOR_GRAY(52)), cx - r, top, 2 * r, h);
    fill(tc(COLOR_GRAY(66)), cx - r + 6, top, 6, h);                  /* riflesso */
    /* nervature: 8 intorno al cilindro, visibili solo sul davanti */
    for (int k = 0; k < 8; k++)
    {
        float a = (float)turn_milli * 0.0062832f + (float)k * 0.785398f;
        float c = cosf(a);
        if (c <= 0.15f) continue;
        int x = cx + (int)((float)r * sinf(a));
        fill(tc(COLOR_GRAY(34)), x - 1, top + 4, 3, h - 8);
    }
    if (wound > 0)
    {
        int R = r + wound;
        fill(tc(COLOR_GRAY(16)), cx - R, SC_CY - SC_FILM_H / 2, 2 * R, SC_FILM_H);
        fill(tc(COLOR_GRAY(30)), cx - R + 4, SC_CY - SC_FILM_H / 2, 3, SC_FILM_H);    /* riflesso */
        fill(tc(COLOR_GRAY(22)), cx - R + 10, SC_CY - SC_FILM_H / 2, 2, SC_FILM_H);
        fill(tc(COLOR_GRAY(10)), cx + R - 5, SC_CY - SC_FILM_H / 2, 5, SC_FILM_H);    /* ombra */
        /* bordi perforati della pellicola avvolta */
        fill(tc(COLOR_GRAY(26)), cx - R, SC_CY - SC_FILM_H / 2 + 6, 2 * R, 8);
        fill(tc(COLOR_GRAY(26)), cx - R, SC_CY + SC_FILM_H / 2 - 14, 2 * R, 8);
    }
    fill(tc(COLOR_GRAY(42)), cx - r - 6, top - 8, 2 * r + 12, 8);       /* flange */
    fill(tc(COLOR_GRAY(42)), cx - r - 6, top + h, 2 * r + 12, 8);
}

/* manovella di riavvolgimento sul coperchio del rullino, come sulle reflex a pellicola:
 * perno, pomello zigrinato, braccio che si apre e gira in piano (visto un po' dall'alto,
 * quindi su un'ellisse) e impugnatura dritta all'estremita'. Dietro il pomello il braccio
 * passa sotto; quando gira veloce lascia due sagome sfumate. */
static void crank_bar(int x0, int y0, int x1, int y1, int c, int t)
{
    int dx = x1 - x0, dy = y1 - y0, n = MAX(ABS(dx), ABS(dy));
    if (n == 0) { fill(c, x0 - t / 2, y0 - t / 2, t, t); return; }
    for (int i = 0; i <= n; i++) fill(c, x0 + dx * i / n - t / 2, y0 + dy * i / n - t / 2, t, t);
}

static void crank_handle(int ex, int ey, int c_body, int c_hi, int c_cap)
{
    fill(c_body, ex - 3, ey - 11, 7, 12);               /* impugnatura */
    fill(c_hi, ex - 2, ey - 10, 2, 10);                 /* riflesso */
    fill(c_cap, ex - 3, ey - 13, 7, 2);                 /* tappo */
    fill(c_cap, ex - 2, ey - 14, 5, 1);
}

static void draw_crank(int top, int turn_milli, int open)
{
    static int prev_turn = 0;
    int speed = ABS(turn_milli - prev_turn);            /* millesimi di giro dall'ultimo fotogramma */
    prev_turn = turn_milli;
    int cx = SC_CAN_X, hy = top - 11;                   /* centro del pomello */
    int len = lerp(9, 27, open);
    float a = (float)turn_milli * 0.0062832f;
    int metal = tc(COLOR_GRAY(58)), metal_d = tc(COLOR_GRAY(30)), metal_l = tc(COLOR_GRAY(74));
    int knob = tc(COLOR_GRAY(22)), knob_l = tc(COLOR_GRAY(40));

    /* sagome del braccio nei giri veloci */
    if (open >= 1000 && speed > 25)
        for (int g = 2; g >= 1; g--)
        {
            float ag = a - (float)g * 0.45f;
            int gx = cx + (int)((float)len * cosf(ag)), gy = hy + (int)(8.0f * sinf(ag));
            crank_bar(cx, hy - 2, gx, gy - 2, tc(COLOR_GRAY(g == 2 ? 12 : 18)), 2);
            fill(tc(COLOR_GRAY(g == 2 ? 12 : 18)), gx - 1, gy - 11, 3, 10);
        }

    int ex = cx + (int)((float)len * cosf(a)), ey = hy + (int)(8.0f * sinf(a));
    int behind = sinf(a) < 0.0f;

    /* perno che esce dal rullino */
    fill(metal_d, cx - 3, hy + 3, 7, top - hy - 2);
    fill(metal, cx - 2, hy + 3, 2, top - hy - 2);

    if (behind && open > 0)
    {
        crank_bar(cx, hy - 1, ex, ey - 1, metal_d, 4);
        crank_bar(cx, hy - 2, ex, ey - 2, metal, 2);
        crank_handle(ex, ey - 1, knob, knob_l, metal_d);
    }
    /* pomello zigrinato: cilindro con le righe che girano */
    fill(knob, cx - 10, hy - 3, 21, 8);
    for (int k = 0; k < 7; k++)
    {
        float ak = a + (float)k * 0.8976f;
        if (cosf(ak) > 0.0f) continue;                  /* solo le righe sul davanti */
        int rx = cx + (int)(9.0f * sinf(ak));
        fill(knob_l, rx, hy - 2, 1, 6);
    }
    fill(metal_d, cx - 9, hy - 5, 19, 2);               /* faccia di sopra */
    fill(metal, cx - 7, hy - 6, 15, 1);
    fill(tc(COLOR_GRAY(12)), cx - 10, hy + 5, 21, 1);   /* ombra sul coperchio */
    if (!behind && open > 0)
    {
        crank_bar(cx, hy - 3, ex, ey - 1, metal_d, 4);
        crank_bar(cx, hy - 4, ex, ey - 2, metal_l, 2);
        crank_handle(ex, ey - 1, knob, knob_l, C_ACCENT);
    }
    disc(cx, hy - 4, 2, behind ? metal_l : metal);   /* rivetto del braccio */
}

/* sportello che si chiude (da destra) con finestrella promemoria e contapose.
   Il contapose e' un rullo: labels[k] sta al centro quando pos = k * 1000 */
static void draw_door(const struct film *fm, int dx, const char *const *labels, int nlab, int pos)
{
    if (dx >= SC_X1) return;
    int door = tc(COLOR_GRAY(20)), stripe = tc(COLOR_GRAY(24));
    fill(door, dx, SC_Y0, SC_X1 - dx, SC_Y1 - SC_Y0);
    for (int y = SC_Y0 + 14; y < SC_Y1 - 6; y += 16) fill(stripe, dx + 8, y, SC_X1 - dx - 16, 2);
    fill(tc(COLOR_GRAY(34)), dx, SC_Y0, 3, SC_Y1 - SC_Y0);
    /* finestrella promemoria: si vede l'etichetta del rullino */
    int mx = dx + 96, my = 112, mw = 116, mh = 62;
    if (mx + mw + 4 <= SC_X1)
    {
        fill(tc(COLOR_GRAY(40)), mx - 4, my - 4, mw + 8, mh + 8);
        fill(tc(fm->body), mx, my, mw, mh);
        fill(tc(fm->label), mx, my + mh * 30 / 100, mw, mh * 40 / 100);
        char iso[8];
        snprintf(iso, sizeof(iso), "%d", fm->iso);
        int ink = tc(fm->ink) == tc(fm->label) ? C_TEXT : tc(fm->ink);
        txt_c(FONT_MED, ink, mx + mw / 2, my + mh / 2 - 12, iso);
    }
    /* contapose: le cifre scorrono in verticale dentro la finestrella */
    int cx = dx + 470, cy = 108;
    if (cx + 84 <= SC_X1 && labels)
    {
        fill(tc(COLOR_GRAY(40)), cx - 4, cy - 4, 88, 66);
        fill(COLOR_BLACK, cx, cy, 80, 58);
        for (int k = 0; k < nlab; k++)
        {
            int y = cy + 12 + (k * 1000 - pos) * 44 / 1000;
            if (y > cy - 34 && y < cy + 58) txt_c(FONT_LARGE, C_ACCENT, cx + 40, y, labels[k]);
        }
        /* maschere sopra e sotto la finestrella (le cifre che escono spariscono dietro lo sportello) */
        fill(door, cx - 4, cy - 40, 88, 36);
        fill(door, cx - 4, cy + 62, 88, 34);
        for (int y = SC_Y0 + 14; y < SC_Y1 - 6; y += 16)
            if ((y >= cy - 40 && y < cy - 4) || (y >= cy + 62 && y < cy + 96)) fill(stripe, cx - 4, y, 88, 2);
        fill(tc(COLOR_GRAY(40)), cx - 4, cy - 4, 88, 4);
        fill(tc(COLOR_GRAY(40)), cx - 4, cy + 58, 88, 4);
        fill(tc(COLOR_GRAY(30)), cx, cy + 27, 3, 4);                 /* indice */
        txt_c(FONT_SMALL, tc(COLOR_GRAY(60)), cx + 40, cy + 74, "POSE");
    }
}

static void scene_text(const char *big, const char *line2, const char *line3, int big_color)
{
    if (big) txt_c(FONT_LARGE, big_color, 360, 300, big);
    if (line2) txt_c(FONT_MED, C_DIM, 360, 344, line2);
    if (line3) txt_c(FONT_MED, C_DIM, 360, 376, line3);
}

/* finale delle scene: spunta che si disegna e testo che sale */
static void scene_done(int t, const char *big, const char *line2, const char *line3)
{
    int w = txt_w(FONT_LARGE, big);
    draw_check(360 - w / 2 - 12, 316, 17, t);
    txt_in(FONT_LARGE, C_TEXT, 360 - w / 2 + 28, 300, big, ease_k(t, 220, EZ_OUT), 14, 0);
    if (line2) txt_c_in(FONT_MED, C_DIM, 360, 344, line2, ease_k(t - 90, 240, EZ_OUT), 0, 10);
    if (line3) txt_c_in(FONT_MED, C_DIM, 360, 376, line3, ease_k(t - 170, 240, EZ_OUT), 0, 10);
}

/* barra di avanzamento sottile sotto la scena */
static void scene_progress(int permille)
{
    fill(C_LINE, 210, 420, 300, 3);
    fill(C_ACCENT, 210, 420, 300 * COERCE(permille, 0, 1000) / 1000, 3);
}

/* caricamento: il rullino scende nel vano, la linguetta corre fino alla bobina e si aggancia,
   la pellicola avanza, lo sportello si chiude e il contapose scorre S -> 0 -> 1 */
#define LOAD_FULL 2150
#define LOAD_END  (LOAD_FULL + 600)        /* piu' la spunta e il testo finale */
static void anim_load(const struct film *fm)
{
    int dur = anim_ms(LOAD_END);
    int t0 = get_ms_clock();
    int done = 0, skipped = 0;
    char info[96];
    snprintf(info, sizeof(info), "%s %s - %d pose%s", fm->brand, fm->name, P.roll_exp,
        P.roll_push ? (P.roll_push > 0 ? " - push" : " - pull") : "");
    static const char *const counter[3] = { "S", "0", "1" };
    static const short step_at[5] = { 0, 320, 950, 1350, 1700 };
    static const char *const steps[5] = { "Inserisco il rullino", "Aggancio la linguetta alla bobina",
        "Avanzamento della pellicola", "Chiudo lo sportello", "Avanzo al primo fotogramma" };
    while (!done)
    {
        int t = get_ms_clock() - t0;
        if (t >= dur) { t = dur; done = 1; }
        if (anim_skip()) { t = dur; done = 1; skipped = 1; }
        int tv = dur ? t * LOAD_END / dur : LOAD_END;

        int e_drop = phase(tv, 0, 320, EZ_BACK);
        int e_pull = phase(tv, 320, 630, EZ_IO);
        int e_wind = phase(tv, 950, 400, EZ_IO);
        int e_door = phase(tv, 1350, 350, EZ_IO);
        int top = lerp(-110, SC_CAN_TOP, e_drop);
        int x0 = SC_LIP_X, x_end = SC_DRUM_X - SC_DRUM_R;

        frame_begin();
        fill(C_BG, 0, 46, SCR_W, 398);
        draw_chamber();
        /* la pellicola passa sotto la bobina: prima la striscia, poi la bobina sopra */
        if (e_pull > 0) draw_strip(fm, x0, lerp(x0 + 8, x_end + 14, e_pull), e_wind * 150 / 1000, e_pull < 1000, 0);
        draw_drum(e_wind * 16 / 10, e_wind * 4 / 1000);
        draw_canister(fm, SC_CAN_X, top, 100, 0);

        int pos = phase(tv, 1700, 170, EZ_BACK) + phase(tv, 1860, 170, EZ_BACK);
        draw_door(fm, lerp(SC_X1, SC_X0, e_door), counter, 3, pos);
        draw_header("Rullini");

        if (tv < LOAD_FULL)
        {
            int k = 0;
            while (k < 4 && tv >= step_at[k + 1]) k++;
            scene_text("Caricamento...", 0, 0, C_TEXT);
            txt_c_in(FONT_MED, C_DIM, 360, 344, steps[k], phase(tv, step_at[k], 220, EZ_OUT), 22, 0);
            scene_progress(tv * 1000 / LOAD_FULL);
        }
        else
        {
            char big[32];
            snprintf(big, sizeof(big), "Rullino %03d pronto", P.roll_num % 1000);
            scene_done(tv - LOAD_FULL, big, info, opt_review_off ? "Anteprima spenta: vedrai le foto dopo, come con la pellicola" : 0);
        }
        draw_footer("");
        frame_show(0);
        msleep(10);
    }
    if (!skipped) for (int i = 0; i < 35 && !anim_skip(); i++) msleep(20);
}

/* riavvolgimento: la manovella gira, la pellicola torna indietro e la bobina si svuota,
   la coda corre dentro il rullino, che poi esce dal vano */
#define REW_FULL 1850
#define REW_END  (REW_FULL + 600)
static void anim_rewind(const struct film *fm, int shots)
{
    int dur = anim_ms(REW_END);
    int t0 = get_ms_clock();
    int done = 0, skipped = 0;
    int wound0 = 3 + COERCE(shots, 0, 36) * 12 / 36;
    char info[96];
    snprintf(info, sizeof(info), "%s %s - %d pose su %d", fm->brand, fm->name, shots, P.roll_exp);
    while (!done)
    {
        int t = get_ms_clock() - t0;
        if (t >= dur) { t = dur; done = 1; }
        if (anim_skip()) { t = dur; done = 1; skipped = 1; }
        int tv = dur ? t * REW_END / dur : REW_END;

        int e_in = phase(tv, 0, 150, EZ_OUT);
        int e_rew = phase(tv, 150, 1100, EZ_IO);
        int e_tail = phase(tv, 1250, 300, EZ_IN);
        int e_out = phase(tv, 1550, 300, EZ_BACK);
        int top = lerp(SC_CAN_TOP, SC_CAN_TOP - 34, e_out);
        int x0 = SC_LIP_X, x_end = SC_DRUM_X - SC_DRUM_R - wound0 * (1000 - e_rew) / 1000;

        frame_begin();
        fill(C_BG, 0, 46, SCR_W, 398);
        draw_chamber();
        int x1 = lerp(x_end + 14, x0, e_tail);
        if (e_tail < 1000) draw_strip(fm, x0, x1, -e_rew * 440 / 1000, 0, 1);
        draw_drum(-e_rew * 6, wound0 * (1000 - e_rew) / 1000);
        draw_canister(fm, SC_CAN_X, top, 100, 0);
        /* la manovella si apre prima di girare e si richiude quando la coda e' dentro */
        int open = MIN(phase(tv, 0, 220, EZ_BACK), 1000 - phase(tv, 1260, 240, EZ_IO));
        if (e_out < 1000) draw_crank(top, e_rew * 9, COERCE(open, 0, 1000));
        if (e_in < 1000) fill(C_BG, 0, 46 + 398 * e_in / 1000, SCR_W, 398 - 398 * e_in / 1000);
        draw_header("Rullini");

        if (tv < 1550)
        {
            char big[16];
            int left = shots - shots * e_rew / 1000;
            snprintf(big, sizeof(big), "%d", left);
            scene_text("Riavvolgimento...", 0, 0, C_TEXT);
            txt_c(FONT_LARGE, C_ACCENT, 360, 340, big);
            scene_progress(tv * 1000 / 1550);
        }
        else
        {
            char big[32];
            snprintf(big, sizeof(big), "Rullino %03d riavvolto", P.roll_num % 1000);
            scene_done(tv - 1550, big, info, "Pronto per lo sviluppo");
        }
        draw_footer("");
        frame_show(0);
        msleep(10);
    }
    if (!skipped) for (int i = 0; i < 35 && !anim_skip(); i++) msleep(20);
}
