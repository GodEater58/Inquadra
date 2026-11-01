/* film.inc - pellicole, rullino (carica, pausa, riprendi, riavvolgi, push/pull),
 * contatore delle pose, contatore nel Live View, disegno e animazioni dei rullini */

enum { CAT_COLOR, CAT_SLIDE, CAT_BW, CAT_CINE, CAT_COUNT };
static const char *const cat_names[CAT_COUNT] = { "COLORE", "DIA", "B/N", "CINE" };

struct film
{
    const char *brand, *name, *desc;
    int cat, iso;
    const char *process;
    int style, contrast, saturation, tone, kelvin;
    int body, label, ink;
};

/* approssimazione del carattere della pellicola sul JPEG: il RAW resta neutro */
static const struct film films[] = {
    { "KODAK",    "Portra 160",      "Pelle morbida, colori tenui",       CAT_COLOR, 160,  "C-41", PS_PORTRAIT,  -1, -1,  0, 5200, COLOR_ALMOST_WHITE, COLOR_ORANGE,   COLOR_BLACK },
    { "KODAK",    "Portra 400",      "Toni caldi, alte luci delicate",    CAT_COLOR, 400,  "C-41", PS_PORTRAIT,  -1,  0,  1, 5300, COLOR_ALMOST_WHITE, COLOR_ORANGE,   COLOR_BLACK },
    { "KODAK",    "Portra 800",      "Poca luce, grana calda",            CAT_COLOR, 800,  "C-41", PS_PORTRAIT,   0,  1,  1, 5300, COLOR_ALMOST_WHITE, COLOR_DARK_RED, COLOR_WHITE },
    { "KODAK",    "Ektar 100",       "Colori saturi, grana finissima",    CAT_COLOR, 100,  "C-41", PS_LANDSCAPE,  1,  2,  0, 5200, COLOR_ORANGE,       COLOR_RED,      COLOR_WHITE },
    { "KODAK",    "Gold 200",        "Giallo estivo, sapore nostalgico",  CAT_COLOR, 200,  "C-41", PS_STD,        0,  1,  2, 6000, COLOR_YELLOW,       COLOR_ORANGE,   COLOR_BLACK },
    { "KODAK",    "UltraMax 400",    "Tuttofare, contrasto vivace",       CAT_COLOR, 400,  "C-41", PS_STD,        1,  1,  1, 5600, COLOR_YELLOW,       COLOR_BLUE,     COLOR_WHITE },
    { "FUJIFILM", "Superia 400",     "Ombre verdi, uso quotidiano",       CAT_COLOR, 400,  "C-41", PS_STD,        1,  1, -1, 4900, COLOR_GREEN2,       COLOR_ALMOST_WHITE, COLOR_BLACK },
    { "FUJIFILM", "C200",            "Economica, toni freschi",           CAT_COLOR, 200,  "C-41", PS_STD,        0,  0, -1, 5000, COLOR_GREEN1,       COLOR_ALMOST_WHITE, COLOR_BLACK },
    { "FUJIFILM", "Velvia 50",       "Dia satura, paesaggi",              CAT_SLIDE, 50,   "E-6",  PS_LANDSCAPE,  3,  4,  0, 5200, COLOR_DARK_RED,     COLOR_ALMOST_WHITE, COLOR_BLACK },
    { "FUJIFILM", "Provia 100F",     "Dia neutra e fedele",               CAT_SLIDE, 100,  "E-6",  PS_STD,        2,  1,  0, 5200, COLOR_BLUE,         COLOR_ALMOST_WHITE, COLOR_BLACK },
    { "KODAK",    "Ektachrome E100", "Dia pulita, blu freddi",            CAT_SLIDE, 100,  "E-6",  PS_STD,        2,  1, -1, 5000, COLOR_LIGHT_BLUE,   COLOR_YELLOW,   COLOR_BLACK },
    { "ILFORD",   "HP5 Plus",        "B/N morbido, grana classica",       CAT_BW,    400,  "B/N",  PS_MONO,       1,  0,  0, 5200, COLOR_GRAY(80),     COLOR_BLACK,    COLOR_WHITE },
    { "KODAK",    "Tri-X 400",       "B/N contrastato da reportage",      CAT_BW,    400,  "B/N",  PS_MONO,       3,  0,  0, 5200, COLOR_YELLOW,       COLOR_BLACK,    COLOR_WHITE },
    { "ILFORD",   "Delta 3200",      "B/N notturno, grana forte",         CAT_BW,    3200, "B/N",  PS_MONO,       2,  0,  0, 5200, COLOR_GRAY(70),     COLOR_DARK_RED, COLOR_WHITE },
    { "CINESTILL","800T",            "Tungsteno, aloni rossi in citta'",  CAT_CINE,  800,  "C-41", PS_STD,        0,  1,  0, 3200, COLOR_DARK_RED,     COLOR_GRAY(20), COLOR_WHITE },
    { "CINESTILL","50D",             "Luce diurna, cinema",               CAT_CINE,  50,   "C-41", PS_STD,        1,  1,  0, 5500, COLOR_LIGHT_BLUE,   COLOR_GRAY(20), COLOR_WHITE },
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
    P.roll_first_fn = current_file_number() + 1;
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

static unsigned int iride_clock(unsigned int ctx)
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
    /* niente da fare all'avvio: lo stato e' gia' in P */
}

/* ------------------------------------------------------------------ disegno dei rullini */

static void draw_canister(const struct film *fm, int cx, int top, int scale, int leader)
{
    int w = 76 * scale / 100;
    int h = 126 * scale / 100;
    int x = cx - w / 2;
    int cap = MAX(10 * scale / 100, 3);
    int body = tc(fm->body), label = tc(fm->label), ink = tc(fm->ink);
    if (night && label == body) label = COLOR_BLACK;

    fill(tc(COLOR_GRAY(45)), cx - 7 * scale / 100, top, 14 * scale / 100, cap);
    fill(tc(COLOR_GRAY(65)), x + w / 10, top + cap, w * 8 / 10, cap);
    fill(body, x, top + 2 * cap, w, h);
    fill(tc(COLOR_GRAY(30)), x + w - MAX(w / 12, 2), top + 2 * cap, MAX(w / 12, 2), h);   /* ombra */
    fill(tc(COLOR_GRAY(65)), x + w / 10, top + 2 * cap + h, w * 8 / 10, cap);

    int by = top + 2 * cap + h * 32 / 100;
    int bh = h * 36 / 100;
    fill(label, x, by, w, bh);
    char iso[8];
    snprintf(iso, sizeof(iso), "%d", fm->iso);
    int font = scale >= 90 ? FONT_LARGE : scale >= 60 ? FONT_MED : FONT_SMALL;
    int fh = scale >= 90 ? 32 : scale >= 60 ? 23 : 12;
    txt_c(font, ink == label ? C_TEXT : ink, cx, by + (bh - fh) / 2, iso);

    if (leader > 0)
    {
        /* linguetta della pellicola, lunga "leader" pixel */
        int lx = x + w;
        int ly = top + 2 * cap + h * 40 / 100;
        int lh = h * 26 / 100;
        fill(tc(COLOR_GRAY(22)), lx, ly, leader, lh);
        for (int px = lx + 4; px + 4 < lx + leader; px += 9)
        {
            fill(tc(COLOR_GRAY(55)), px, ly + 3, 4, 4);
            fill(tc(COLOR_GRAY(55)), px, ly + lh - 7, 4, 4);
        }
    }
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
    frame_rect(tc(COLOR_GRAY(30)), SC_X0, SC_Y0, SC_X1 - SC_X0, SC_Y1 - SC_Y0, 2);
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

/* manovella di riavvolgimento sul coperchio del rullino */
static void draw_crank(int top, int turn_milli)
{
    float a = (float)turn_milli * 0.0062832f;
    int px = SC_CAN_X, py = top + 3;
    int ex = px + (int)(30.0f * cosf(a)), ey = py + (int)(9.0f * sinf(a));
    draw_line(px, py, ex, ey, C_ACCENT);
    draw_line(px, py + 1, ex, ey + 1, C_ACCENT);
    fill_circle(ex, ey, 6, C_ACCENT);
    fill_circle(px, py, 3, tc(COLOR_GRAY(70)));
}

/* sportello che si chiude (da destra) con finestrella promemoria e contapose */
static void draw_door(const struct film *fm, int dx, const char *counter, int nudge)
{
    if (dx >= SC_X1) return;
    fill(tc(COLOR_GRAY(20)), dx, SC_Y0, SC_X1 - dx, SC_Y1 - SC_Y0);
    for (int y = SC_Y0 + 14; y < SC_Y1 - 6; y += 16) fill(tc(COLOR_GRAY(24)), dx + 8, y, SC_X1 - dx - 16, 2);
    fill(tc(COLOR_GRAY(34)), dx, SC_Y0, 3, SC_Y1 - SC_Y0);
    /* finestrella promemoria: si vede l'etichetta del rullino */
    int mx = dx + 96, my = 112, mw = 116, mh = 62;
    if (mx < SC_X1)
    {
        fill(tc(COLOR_GRAY(40)), mx - 4, my - 4, mw + 8, mh + 8);
        fill(tc(fm->body), mx, my, mw, mh);
        fill(tc(fm->label), mx, my + mh * 30 / 100, mw, mh * 40 / 100);
        char iso[8];
        snprintf(iso, sizeof(iso), "%d", fm->iso);
        int ink = tc(fm->ink) == tc(fm->label) ? C_TEXT : tc(fm->ink);
        txt_c(FONT_MED, ink, mx + mw / 2, my + mh / 2 - 12, iso);
    }
    /* contapose */
    int cx = dx + 470, cy = 108;
    if (cx < SC_X1 && counter)
    {
        fill(tc(COLOR_GRAY(40)), cx - 4, cy - 4, 88, 66);
        fill(COLOR_BLACK, cx, cy, 80, 58);
        txt_c(FONT_LARGE, C_ACCENT, cx + 40, cy + 12 + nudge, counter);
        txt_c(FONT_SMALL, tc(COLOR_GRAY(60)), cx + 40, cy + 74, "POSE");
    }
}

static void scene_text(const char *big, const char *line2, const char *line3, int big_color)
{
    if (big) txt_c(FONT_LARGE, big_color, 360, 300, big);
    if (line2) txt_c(FONT_MED, C_DIM, 360, 344, line2);
    if (line3) txt_c(FONT_MED, C_DIM, 360, 376, line3);
}

/* caricamento: il rullino scende nel vano, la linguetta corre fino alla bobina e si aggancia,
   la pellicola avanza, lo sportello si chiude e il contapose fa S -> 0 -> 1 */
#define LOAD_FULL 2150
static void anim_load(const struct film *fm)
{
    int dur = anim_ms(LOAD_FULL);
    int t0 = get_ms_clock();
    int done = 0;
    char info[96];
    snprintf(info, sizeof(info), "%s %s - %d pose%s", fm->brand, fm->name, P.roll_exp,
        P.roll_push ? (P.roll_push > 0 ? " - push" : " - pull") : "");
    while (!done)
    {
        int t = get_ms_clock() - t0;
        if (t >= dur || key_pending()) { t = dur; done = 1; }
        int tv = dur ? t * LOAD_FULL / dur : LOAD_FULL;

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

        const char *cnt = tv < 1700 ? "S" : tv < 1850 ? "0" : "1";
        int nudge = 0;
        if (tv >= 1700)
        {
            int since = tv < 1850 ? tv - 1700 : tv - 1850;
            nudge = since < 90 ? (90 - since) / 15 : 0;
        }
        draw_door(fm, lerp(SC_X1, SC_X0, e_door), cnt, nudge);
        draw_header("Rullini");

        if (tv < LOAD_FULL)
        {
            const char *step = tv < 320 ? "Inserisco il rullino" : tv < 950 ? "Aggancio la linguetta alla bobina"
                : tv < 1350 ? "Avanzamento della pellicola" : tv < 1700 ? "Chiudo lo sportello" : "Avanzo al primo fotogramma";
            scene_text("Caricamento...", step, 0, C_TEXT);
        }
        else
        {
            char big[32];
            snprintf(big, sizeof(big), "Rullino %03d pronto", P.roll_num % 1000);
            scene_text(big, info, opt_review_off ? "Anteprima spenta: vedrai le foto dopo, come con la pellicola" : 0, C_TEXT);
        }
        draw_footer("");
        frame_show(0);
        msleep(10);
    }
    for (int i = 0; i < 65 && !key_pending(); i++) msleep(20);
}

/* riavvolgimento: la manovella gira, la pellicola torna indietro e la bobina si svuota,
   la coda corre dentro il rullino, che poi esce dal vano */
#define REW_FULL 1850
static void anim_rewind(const struct film *fm, int shots)
{
    int dur = anim_ms(REW_FULL);
    int t0 = get_ms_clock();
    int done = 0;
    int wound0 = 3 + COERCE(shots, 0, 36) * 12 / 36;
    char info[96];
    snprintf(info, sizeof(info), "%s %s - %d pose su %d", fm->brand, fm->name, shots, P.roll_exp);
    while (!done)
    {
        int t = get_ms_clock() - t0;
        if (t >= dur || key_pending()) { t = dur; done = 1; }
        int tv = dur ? t * REW_FULL / dur : REW_FULL;

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
        if (e_out < 1000) draw_crank(top, e_rew * 9);
        if (e_in < 1000) fill(C_BG, 0, 46 + 398 * e_in / 1000, SCR_W, 398 - 398 * e_in / 1000);
        draw_header("Rullini");

        char big[32];
        int left = shots - shots * e_rew / 1000;
        if (tv < 1550)
        {
            snprintf(big, sizeof(big), "%d", left);
            scene_text("Riavvolgimento...", 0, 0, C_TEXT);
            txt_c(FONT_LARGE, C_ACCENT, 360, 340, big);
        }
        else
        {
            snprintf(big, sizeof(big), "Rullino %03d riavvolto", P.roll_num % 1000);
            scene_text(big, info, "Pronto per lo sviluppo", C_TEXT);
        }
        draw_footer("");
        frame_show(0);
        msleep(10);
    }
    for (int i = 0; i < 60 && !key_pending(); i++) msleep(20);
}
