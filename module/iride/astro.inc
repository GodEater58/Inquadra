/* astro.inc - Via Lattea (NPF), intervallometro, fuoco stelle (HFR),
 * allineamento polare, calendario del cielo, etichetta dei file */

/* ------------------------------------------------------------------ NPF */

#define PITCH_NM 6542   /* 35,8 mm / 5472 px */
static const int ap_steps[] = { 10, 12, 14, 18, 20, 22, 25, 28, 32, 35, 40, 45, 56, 63, 80 };
static const int bulb_steps[] = { 0, 30, 45, 60, 90, 120, 180, 240, 300, 420, 600, 900, 1200, 1800 };
static const int delay_steps[] = { 0, 2, 5, 10, 30, 60, 120 };

/* valore successivo/precedente di una scala, anche partendo da un valore fuori scala */
static int step_in(const int *steps, int n, int value, int dir)
{
    int i = 0;
    while (i < n - 1 && steps[i] < value) i++;
    if (steps[i] == value) i += dir;
    else if (steps[i] > value && dir < 0) i--;
    else if (steps[i] < value && dir > 0) i++;
    return steps[COERCE(i, 0, n - 1)];
}

static int npf_tenths(int focal, int ap10)
{
    if (focal <= 0) return 0;
    float ap = (float)ap10 / 10.0f;
    float pitch = (float)PITCH_NM / 1000.0f;
    return (int)((35.0f * ap + 30.0f * pitch) / (float)focal * 10.0f);
}

/* punto del cielo: piu' vicino al polo celeste, piu' lente le stelle (t / cos della declinazione) */
static const int npf_decs[] = { 0, 15, 29, 45, 60, 75 };
static const char *const npf_dec_names[] = { "equatore (caso peggiore)", "15 gradi", "centro Via Lattea, 29",
    "45 gradi", "60 gradi", "verso la Polare, 75" };
#define NPF_DECS ((int)COUNT(npf_decs))
static int npf_dec() { return npf_decs[COERCE(opt_npf_dec, 0, NPF_DECS - 1)]; }

/* NPF completa: t = (35 N + 30 p) / (f cos d) */
static int npf_tenths_dec(int focal, int ap10, int dec)
{
    float c = cosf((float)MIN(ABS(dec), 85) * 0.017453293f);
    return (int)((float)npf_tenths(focal, ap10) / c + 0.5f);
}

/* tempo Canon a 1/3 di stop piu' lungo che non supera t secondi (tolleranza 0,1 EV) */
static int npf_shutter_code(float t)
{
    int raw = (int)(56.0f - log2f(t) * 8.0f);
    raw = MAX(raw, 16);
    while (raw < 160 && ((raw % 8 != 0 && raw % 8 != 3 && raw % 8 != 5) || raw2shutterf(raw) > t * 1.07f))
        raw++;
    return raw;
}

static void npf_enter()
{
    if (lens_info.focal_len > 0) P.npf_focal = lens_info.focal_len;
    if (lens_info.raw_aperture_min) P.npf_ap10 = RAW2VALUE(aperture, lens_info.raw_aperture_min);
    P.npf_focal = COERCE(P.npf_focal, 8, 800);
    P.npf_ap10 = COERCE(P.npf_ap10, 10, 220);
}

static void npf_apply()
{
    if (shooting_mode != SHOOTMODE_M) { say(C_WARN, "Metti la ghiera su M e riprova"); return; }
    snap_take();
    int t10 = npf_tenths_dec(P.npf_focal, P.npf_ap10, npf_dec());
    float t = (float)MIN(t10, 300) / 10.0f;
    if (t < 1.0f) t = 1.0f;
    int raw_sh = npf_shutter_code(t);
    int ok = 1;
    if (lens_info.raw_aperture_min)
    {
        int ra = val2raw_aperture(P.npf_ap10);
        if (ra > 0) ok &= lens_set_rawaperture(ra);
    }
    ok &= lens_set_rawshutter(raw_sh);
    int raw_iso = val2raw_iso(P.npf_ap10 <= 20 ? 3200 : 6400);
    if (raw_iso > 0) ok &= lens_set_rawiso(raw_iso);
    lens_set_kelvin(3900);
    char buf[96];
    snprintf(buf, sizeof(buf), "%s %s  %s  ISO %s  3900K", ok ? "Applicato:" : "In parte:",
        lens_format_shutter(raw_sh), lens_format_aperture(lens_info.raw_aperture), lens_format_iso(lens_info.raw_iso));
    say(ok ? C_OK : C_WARN, buf);
}

static const char *const npf_labels[] = { "Focale", "Diaframma", "Punto del cielo", "Applica (ghiera su M)" };

static void npf_value(int row, char *buf, int len, int *kind)
{
    char a[12];
    if (row == 0) snprintf(buf, len, "< %d mm >", P.npf_focal);
    else if (row == 1) { fmt_tenths(a, sizeof(a), P.npf_ap10); snprintf(buf, len, "< f/%s >", a); }
    else if (row == 2) snprintf(buf, len, "< %s >", npf_dec_names[COERCE(opt_npf_dec, 0, NPF_DECS - 1)]);
    else *kind = VK_ACTION;
}

static void npf_change(int row, int dir, int big)
{
    if (row == 0) P.npf_focal = COERCE(P.npf_focal + dir * (big ? 5 : 1), 8, 800);
    else if (row == 1) P.npf_ap10 = step_in(ap_steps, COUNT(ap_steps), P.npf_ap10, dir);
    else if (row == 2) opt_npf_dec = COERCE(opt_npf_dec + dir, 0, NPF_DECS - 1);
    persist_touch();
}

static void npf_activate(int row)
{
    if (row == 2) { opt_npf_dec = MOD(opt_npf_dec + 1, NPF_DECS); return; }
    npf_apply();
}

static void npf_extra()
{
    char buf[96], a[12], b[12];
    int dec = npf_dec();
    int t0 = npf_tenths(P.npf_focal, P.npf_ap10), t10 = npf_tenths_dec(P.npf_focal, P.npf_ap10, dec);
    int r500 = P.npf_focal > 0 ? 5000 / P.npf_focal : 0;
    int y = 242;
    info_card(24, y, SCR_W - 48, 118, "TEMPO MASSIMO CON LE STELLE PUNTIFORMI (NPF)");
    fmt_tenths(a, sizeof(a), MIN(t10, 9999));
    snprintf(buf, sizeof(buf), "%s s", a);
    txt(FONT_LARGE, t10 > 300 ? C_WARN : C_ACCENT, 44, y + 26, buf);
    if (dec > 0)
    {
        /* quanto si guadagna puntando lontano dall'equatore celeste */
        int k100 = t0 > 0 ? t10 * 100 / t0 : 100;
        fmt_tenths(b, sizeof(b), t0);
        snprintf(buf, sizeof(buf), "x%d,%02d rispetto all'equatore (%s s)", k100 / 100, k100 % 100, b);
        txt(FONT_SMALL, C_DIM, 44 + txt_w(FONT_LARGE, a) + 40, y + 40, buf);
    }
    /* confronto: barra NPF e barra regola del 500 sulla stessa scala */
    int mx = MAX(MAX(t10, r500), 1);
    int bw = 260;
    fill_r(C_PANEL2, 44, y + 74, bw, 8, 4);
    fill_r(C_ACCENT, 44, y + 74, MAX(8, bw * MIN(t10, mx) / mx), 8, 4);
    fill_r(C_PANEL2, 44, y + 94, bw, 8, 4);
    fill_r(C_LINE, 44, y + 94, MAX(8, bw * MIN(r500, mx) / mx), 8, 4);
    txt(FONT_SMALL, C_DIM, 44 + bw + 10, y + 72, "NPF");
    fmt_tenths(b, sizeof(b), r500);
    snprintf(buf, sizeof(buf), "regola del 500: %s s", b);
    txt(FONT_SMALL, C_DIM, 44 + bw + 10, y + 92, buf);
    int x2 = 470;
    snprintf(buf, sizeof(buf), "ISO %d   3900 K", P.npf_ap10 <= 20 ? 3200 : 6400);
    txt(FONT_MED, C_TEXT, x2, y + 26, buf);
    txt(FONT_SMALL, C_DIM, x2, y + 54, "RAW, stabilizzatore spento");
    if (t10 > 300) txt(FONT_SMALL, C_WARN, x2, y + 72, "oltre 30 s: imposto 30 s");
    info_card(24, y + 126, SCR_W - 48, 70, "OBIETTIVO");
    txt(FONT_MED, C_TEXT, 44, y + 146, lens_info.name[0] ? lens_info.name : "Manuale: imposta focale e diaframma");
    txt(FONT_SMALL, C_DIM, 44, y + 174, "t = (35 x N + 30 x p) / (f x cos d)    N diaframma, p pixel 6,54 um, d declinazione");
}

static const struct form npf_form = { 4, npf_labels, npf_value, npf_change, npf_activate, npf_extra,
    "^/v riga   </> cambia   SET applica   MENU indietro", 58, 44, 0, 0, 0 };

/* ------------------------------------------------------------------ intervallometro */

struct preset
{
    const char *name;
    int count, bulb, pause, tag;   /* bulb: 0 = tempo della fotocamera, -1 = lascia l'attuale */
    const char *note1, *note2;
};

static const struct preset presets[] = {
    { "Via Lattea",   20,   0, 2, 1, "Treppiede fisso: tempo dalla regola NPF,",        "diaframma tutto aperto, ISO 3200-6400." },
    { "Deep sky",     15, 120, 3, 1, "Pose da 2 minuti in posa B:",                     "serve una montatura che insegue le stelle." },
    { "Star trails", 120,   0, 0, 1, "Pose una dopo l'altra, senza pause:",             "poi unisci i file per le scie delle stelle." },
    { "Timelapse",   300,   0, 5, 0, "Uno scatto ogni 5 s col tempo",                   "impostato sulla fotocamera." },
    { "Giorno-notte",600,   0, 5, 0, "Live View acceso, ghiera su M: Iride attiva",     "Auto exposure e Post Deflicker di Magic Lantern." },
    { "Dark frame",   20,  -1, 2, 2, "Tappo sull'obiettivo, stessi tempo e ISO",        "delle foto: tolgono il rumore del sensore." },
    { "Flat",         20,   0, 1, 3, "Panno bianco o schermo uniforme davanti,",        "stesso fuoco e diaframma, modo Av." },
    { "Bias",         30,   0, 1, 4, "Tappo sull'obiettivo, stesso ISO delle foto,",    "tempo 1/4000 impostato da Iride." },
};
#define NUM_PRESETS ((int)COUNT(presets))
#define PRESET_HG 4
#define PRESET_BIAS 7

static void preset_select(int p)
{
    P.int_preset = COERCE(p, 0, NUM_PRESETS - 1);
    const struct preset *ps = &presets[P.int_preset];
    P.int_count = ps->count;
    P.int_pause = ps->pause;
    if (ps->bulb >= 0) P.int_bulb = ps->bulb;
    persist_touch();
}

static const char *const int_labels[] = { "Preset", "Scatti", "Posa", "Pausa", "Ritardo iniziale", "Otturatore",
    "Dithering", "Pausa per il dithering", "Schermo", "Etichetta file", "Registro della sessione", "Avvia" };
enum { IR_PRESET, IR_COUNT, IR_POSA, IR_PAUSE, IR_DELAY, IR_SHUTTER, IR_DITHER, IR_DITHER_S, IR_SCREEN, IR_TAG, IR_SESSION, IR_START, IR_N };
static const int dither_steps[] = { 0, 1, 2, 3, 5, 10 };
static const int dither_s_steps[] = { 5, 10, 15, 20, 30, 60 };

/* posa di uno scatto in secondi (-1 se dipende dalla fotocamera e non si sa) */
static float int_expo()
{
    return P.int_bulb > 0 ? (float)P.int_bulb : P.int_preset == PRESET_BIAS ? 0.00025f :
        lens_info.raw_shutter > 0 ? raw2shutterf(lens_info.raw_shutter) : -1.0f;
}

/* durata della sequenza in secondi, comprese le pause per il dithering */
static float int_duration(float expo)
{
    int n = P.int_count, dn = opt_dither_n > 0 ? (n - 1) / opt_dither_n : 0;
    float pause = (float)(MAX(n - 1, 0) * P.int_pause);
    if (dn > 0 && opt_dither_s > P.int_pause) pause += (float)(dn * (opt_dither_s - P.int_pause));
    return (float)n * expo + pause + (float)delay_steps[COERCE(P.int_delay_idx, 0, (int)COUNT(delay_steps) - 1)];
}

static void int_value(int row, char *buf, int len, int *kind)
{
    const struct preset *ps = &presets[COERCE(P.int_preset, 0, NUM_PRESETS - 1)];
    switch (row)
    {
        case IR_PRESET: snprintf(buf, len, "< %s >", ps->name); break;
        case IR_COUNT: snprintf(buf, len, "%d", P.int_count); break;
        case IR_POSA:
            if (P.int_bulb > 0) snprintf(buf, len, "%d s (posa B)", P.int_bulb);
            else if (P.int_preset == PRESET_BIAS) snprintf(buf, len, "1/4000");
            else snprintf(buf, len, "come la fotocamera");
            break;
        case IR_PAUSE: snprintf(buf, len, "%d s", P.int_pause); break;
        case IR_DELAY: snprintf(buf, len, "%d s", delay_steps[COERCE(P.int_delay_idx, 0, (int)COUNT(delay_steps) - 1)]); break;
        case IR_SHUTTER: snprintf(buf, len, "< %s >", opt_silent ? "silenzioso" : "meccanico"); break;
        case IR_DITHER:
            if (opt_dither_n <= 0) snprintf(buf, len, "< no >");
            else if (opt_dither_n == 1) snprintf(buf, len, "< dopo ogni scatto >");
            else snprintf(buf, len, "< ogni %d scatti >", opt_dither_n);
            break;
        case IR_DITHER_S:
            if (opt_dither_n <= 0) { snprintf(buf, len, "-"); *kind = VK_INFO; }
            else snprintf(buf, len, "< %d s >", opt_dither_s);
            break;
        case IR_SCREEN: *kind = P.int_screen_off ? VK_OFF : VK_ON; break;
        case IR_TAG: snprintf(buf, len, "%s", tag_names[ps->tag]); *kind = VK_INFO; break;
        case IR_SESSION: *kind = opt_session ? VK_ON : VK_OFF; break;
        case IR_START: *kind = VK_ACTION; snprintf(buf, len, "SET  >"); break;
    }
}

static void int_change(int row, int dir, int big)
{
    switch (row)
    {
        case IR_PRESET: preset_select(MOD(P.int_preset + dir, NUM_PRESETS)); break;
        case IR_COUNT: P.int_count = COERCE(P.int_count + dir * (big ? 10 : 1), 1, 9999); break;
        case IR_POSA: P.int_bulb = step_in(bulb_steps, COUNT(bulb_steps), P.int_bulb, dir); break;
        case IR_PAUSE: P.int_pause = COERCE(P.int_pause + dir * (big ? 5 : 1), 0, 3600); break;
        case IR_DELAY: P.int_delay_idx = COERCE(P.int_delay_idx + dir, 0, (int)COUNT(delay_steps) - 1); break;
        case IR_SHUTTER: opt_silent = !opt_silent; break;
        case IR_DITHER: opt_dither_n = step_in(dither_steps, COUNT(dither_steps), opt_dither_n, dir); break;
        case IR_DITHER_S: if (opt_dither_n > 0) opt_dither_s = step_in(dither_s_steps, COUNT(dither_s_steps), opt_dither_s, dir); break;
        case IR_SCREEN: P.int_screen_off = !P.int_screen_off; break;
        case IR_SESSION: opt_session = !opt_session; break;
    }
    persist_touch();
}

/* lo scatto silenzioso a piena risoluzione della 6D: da 0,3 s, ghiera su M, niente posa B */
static int silent_check(float expo)
{
    if (!ml_available(MLF_SILENT)) { say(C_WARN, "Silenzioso: attiva il modulo silent nel menu Modules di ML"); return 0; }
    if (P.int_bulb > 0) { say(C_WARN, "Silenzioso: niente posa B, usa il tempo della fotocamera"); return 0; }
    if (P.int_preset == PRESET_BIAS || P.int_preset == PRESET_HG) { say(C_WARN, "Silenzioso: non con bias e giorno-notte"); return 0; }
    if (shooting_mode != SHOOTMODE_M) { say(C_WARN, "Silenzioso: metti la ghiera su M"); return 0; }
    if (expo >= 0.0f && (expo < 0.29f || expo > 30.5f)) { say(C_WARN, "Silenzioso: tempi da 0,3 a 30 s"); return 0; }
    return 1;
}

static int interval_check()
{
    if (P.int_preset == PRESET_BIAS && shooting_mode != SHOOTMODE_M) { say(C_WARN, "Bias: metti la ghiera su M"); return 0; }
    if (P.int_bulb > 0 && shooting_mode != SHOOTMODE_M && shooting_mode != SHOOTMODE_BULB)
    { say(C_WARN, "Posa B: metti la ghiera su M o B"); return 0; }
    if (P.int_preset == PRESET_HG)
    {
        if (shooting_mode != SHOOTMODE_M || !lv) { say(C_WARN, "Giorno-notte: ghiera su M e Live View acceso"); return 0; }
        if (!ml_available(MLF_AUTOEXPO)) { say(C_WARN, "Attiva il modulo autoexpo nel menu Modules di ML"); return 0; }
    }
    if (opt_silent && !silent_check(int_expo())) return 0;
    int b = cam_battery();
    if (b >= 0 && b <= 5) { say(C_WARN, "Batteria al 5%: cambiala prima di iniziare"); return 0; }
    if (cam_shots_left(opt_silent) == 0) { say(C_WARN, "La scheda e' piena"); return 0; }
    return 1;
}

static void int_activate(int row)
{
    switch (row)
    {
        case IR_PRESET: preset_select(MOD(P.int_preset + 1, NUM_PRESETS)); return;
        case IR_SHUTTER: case IR_DITHER: case IR_DITHER_S: case IR_SCREEN: case IR_SESSION: int_change(row, 1, 0); return;
    }
    if (!interval_check()) return;
    job_request = JOB_SEQ;
    gui_stop_menu();
}

/* riga "etichetta  valore" della scheda di destra */
static void kv_line(int x, int y, int w, const char *k, const char *v, int c)
{
    txt(FONT_SMALL, C_DIM, x, y + 5, k);
    txt_r(FONT_MED, c, x + w, y, v);
}

static void int_extra()
{
    const struct preset *ps = &presets[COERCE(P.int_preset, 0, NUM_PRESETS - 1)];
    const int x = 456, w = SCR_W - 24 - x, ix = x + 16, iw = w - 32;
    char buf[96], v[32];
    info_card(x, 52, w, 384, "PRESET");
    txt(FONT_MED_LARGE, C_ACCENT, ix, 72, ps->name);
    /* la sequenza: un rettangolo per scatto, lo spazio e' la pausa (piu' lungo dove c'e' il dithering) */
    int n = MIN(P.int_count, 12), fw = 12, gap = 2 + MIN(P.int_pause, 10);
    int fx = ix;
    for (int i = 0; i < n && fx + fw <= x + w - 16; i++)
    {
        fill(i == 0 ? C_ACCENT : C_DIM, fx, 108, fw, 16);
        fx += fw + gap + (opt_dither_n > 0 && (i + 1) % opt_dither_n == 0 ? 6 : 0);
    }
    if (P.int_count > n) txt(FONT_SMALL, C_DIM, fx + 2, 110, "...");

    /* durata e ora di fine */
    float expo = int_expo();
    txt(FONT_SMALL, C_DIM, ix, 136, "DURATA");
    float tot = expo >= 0.0f ? int_duration(expo) : -1.0f;
    if (tot >= 0.0f)
    {
        fmt_duration(buf, sizeof(buf), (int)(tot * 10.0f));
        txt(FONT_LARGE, C_TEXT, ix, 150, buf);
        struct tm now;
        LoadCalendarFromRTC(&now);
        int end = MOD(now.tm_hour * 60 + now.tm_min + (int)(tot / 60.0f + 0.5f), 1440);
        snprintf(buf, sizeof(buf), "finisce verso le %02d:%02d", end / 60, end % 60);
        txt(FONT_SMALL, C_DIM, ix, 186, buf);
    }
    else txt(FONT_MED, C_TEXT, ix, 154, "dipende dal tempo");

    /* in campo: batteria, scheda, temperatura */
    fill(C_LINE, ix, 210, iw, 1);
    txt(FONT_SMALL, C_DIM, ix, 216, "IN CAMPO");
    int b = cam_battery(), left = cam_battery_left_s();
    if (b < 0) snprintf(v, sizeof(v), "-");
    else if (left > 0) snprintf(v, sizeof(v), "%d%s  ~%dh%02d", b, "%", left / 3600, left % 3600 / 60);
    else snprintf(v, sizeof(v), "%d%%", b);
    kv_line(ix, 234, iw, "BATTERIA", v, b >= 0 && b <= 15 ? C_WARN : C_TEXT);
    int sl = cam_shots_left(opt_silent);
    if (sl < 0) snprintf(v, sizeof(v), "-");
    else snprintf(v, sizeof(v), "%d scatti", sl);
    kv_line(ix, 260, iw, "SCHEDA", v, sl >= 0 && sl < P.int_count ? C_WARN : C_TEXT);
    snprintf(v, sizeof(v), "%d gradi", cam_temp_c());
    kv_line(ix, 286, iw, "TEMPERATURA", v, C_TEXT);

    /* i limiti: quello che finisce prima */
    int y = 318;
    if (sl >= 0 && sl < P.int_count)
    {
        snprintf(buf, sizeof(buf), "La scheda basta per %d scatti su %d.", sl, P.int_count);
        y = txt_wrap(FONT_SMALL, C_WARN, ix, y, iw, buf, 14) + 4;
    }
    if (left > 0 && tot > 0.0f && left < (int)tot && expo >= 0.0f)
    {
        int per = MAX(1, (int)(expo + 0.5f) + P.int_pause);
        snprintf(buf, sizeof(buf), "La batteria basta per circa %d scatti: Iride si ferma da sola al 4%%.", MIN(left / per, P.int_count));
        y = txt_wrap(FONT_SMALL, C_WARN, ix, y, iw, buf, 14) + 4;
    }
    if (opt_silent)
    {
        y = txt_wrap(FONT_SMALL, C_DIM, ix, y, iw, "Silenzioso: DNG senza otturatore, da 0,3 a 30 s; i dati di scatto vanno nel registro.", 14) + 4;
    }
    if (y < 400)
    {
        snprintf(buf, sizeof(buf), "%s %s", ps->note1, ps->note2);
        txt_wrap(FONT_SMALL, C_DIM, ix, y, iw, buf, 14);
    }
}

static const struct form int_form = { IR_N, int_labels, int_value, int_change, int_activate, int_extra,
    "^/v riga   </> cambia   SET avvia   MENU indietro", 52, 48, 0, 420, 0 };

/* ------------------------------------------------------------------ fuoco sulle stelle (HFR) */

#define HFR_HIST 96
static int hfr_hist[HFR_HIST];
static int hfr_count = 0, hfr_best = 0, hfr_last = -1, hfr_peak = 0;
static int star_x = -1, star_y = -1, star_r = 0;
static int focus_step = 2;

static int luma(const uint8_t *buf, int pitch, int x, int y) { return buf[y * pitch + x * 2 + 1]; }

/* area di ricerca, in frazioni dello schermo: e' esattamente il riquadro disegnato, sopra il
   pannello in basso (prima la ricerca arrivava fin sotto il pannello, dove la stella non si vede) */
#define SR_X0 144
#define SR_X1 576
#define SR_Y0 86
#define SR_Y1 328

/* fondo cielo e soglia di rumore dall'anello tra r e ro intorno a (bx, by):
   mediana e MAD dell'anello (come StarMeter di Iride) */
static int ring_stats(const uint8_t *b, int W, int H, int Pt, int bx, int by, int r, int ro, int *bg_out, int *thr_out)
{
    static int hist[256], dev[256];
    memset(hist, 0, sizeof(hist));
    int cnt = 0;
    for (int y = MAX(by - ro, 0); y <= MIN(by + ro, H - 1); y++)
        for (int x = MAX(bx - ro, 0); x <= MIN(bx + ro, W - 1); x++)
        {
            int dx = x - bx, dy = y - by, d2 = dx * dx + dy * dy;
            if (d2 > r * r && d2 <= ro * ro) { hist[luma(b, Pt, x, y)]++; cnt++; }
        }
    if (cnt < 8) return 0;
    int acc = 0, bg = 0;
    for (bg = 0; bg < 255; bg++) { acc += hist[bg]; if (acc * 2 >= cnt) break; }
    memset(dev, 0, sizeof(dev));
    for (int i = 0; i < 256; i++) if (hist[i]) dev[ABS(i - bg)] += hist[i];
    int mad = 0;
    for (acc = 0, mad = 0; mad < 255; mad++) { acc += dev[mad]; if (acc * 2 >= cnt) break; }
    *bg_out = bg;
    *thr_out = bg + MAX(4, (mad * 3 * 3) / 2);
    return 1;
}

/* stella piu' luminosa nel riquadro, centroide pesato, HFR = raggio medio pesato sulla luce.
 * L'apertura si allarga se la stella e' molto sfocata: prima oltre ~12 px di HFR il valore
 * smetteva di crescere (e poteva perfino calare), ingannando chi gira la ghiera. */
static int hfr_measure(int *hfr100, int *peak_out, int *cx_out, int *cy_out, int *r_out)
{
    struct vram_info *v = get_yuv422_vram();
    if (!v || !v->vram) return 0;
    const uint8_t *b = v->vram;
    int W = v->width, H = v->height, Pt = v->pitch;
    if (W < 64 || H < 64 || Pt < W * 2) return 0;

    int sx0 = MAX(1, W * SR_X0 / SCR_W), sx1 = MIN(W - 2, W * SR_X1 / SCR_W);
    int sy0 = MAX(1, H * SR_Y0 / SCR_H), sy1 = MIN(H - 2, H * SR_Y1 / SCR_H);

    /* candidato: somma 3x3 senza il pixel centrale, cosi' un pixel caldo isolato non vince */
    int best = -1, bx = 0, by = 0;
    for (int y = sy0; y < sy1; y += 2)
        for (int x = sx0; x < sx1; x += 2)
        {
            int s = luma(b, Pt, x - 1, y - 1) + luma(b, Pt, x, y - 1) + luma(b, Pt, x + 1, y - 1)
                  + luma(b, Pt, x - 1, y)                             + luma(b, Pt, x + 1, y)
                  + luma(b, Pt, x - 1, y + 1) + luma(b, Pt, x, y + 1) + luma(b, Pt, x + 1, y + 1);
            if (s > best) { best = s; bx = x; by = y; }
        }
    if (best < 0) return 0;

    int r = MAX(8, W * 35 / 1000), rmax = MAX(r, W * 125 / 1000);
    float cx = (float)bx, cy = (float)by;
    int h = 0, peak = 0, npx = 0;
    for (int pass = 0; pass < 4; pass++)
    {
        int bg, thr, ro = r + MAX(5, r / 5);
        if (!ring_stats(b, W, H, Pt, (int)cx, (int)cy, r, ro, &bg, &thr)) return 0;
        float rr = (float)(r * r);
        for (int it = 0; it < 3; it++)
        {
            float sum = 0.0f, sxw = 0.0f, syw = 0.0f;
            peak = 0;
            int y0 = MAX(0, (int)cy - r), y1 = MIN(H - 1, (int)cy + r);
            int x0 = MAX(0, (int)cx - r), x1 = MIN(W - 1, (int)cx + r);
            for (int y = y0; y <= y1; y++)
                for (int x = x0; x <= x1; x++)
                {
                    float dx = (float)x - cx, dy = (float)y - cy;
                    if (dx * dx + dy * dy > rr) continue;
                    int val = luma(b, Pt, x, y);
                    if (val > peak) peak = val;
                    if (val <= thr) continue;
                    int f = val - bg;
                    sum += (float)f; sxw += (float)(f * x); syw += (float)(f * y);
                }
            if (sum <= 0.0f) return 0;
            cx = sxw / sum;
            cy = syw / sum;
        }
        if (peak - bg < 25) return 0;

        float sum = 0.0f, sd = 0.0f;
        npx = 0;
        int y0 = MAX(0, (int)cy - r), y1 = MIN(H - 1, (int)cy + r);
        int x0 = MAX(0, (int)cx - r), x1 = MIN(W - 1, (int)cx + r);
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++)
            {
                float dx = (float)x - cx, dy = (float)y - cy, d2 = dx * dx + dy * dy;
                if (d2 > rr) continue;
                int val = luma(b, Pt, x, y);
                if (val <= thr) continue;
                int f = val - bg;
                sum += (float)f; sd += (float)f * sqrtf(d2);
                npx++;
            }
        if (sum <= 0.0f || npx < 4) return 0;   /* uno o due pixel caldi non sono una stella */
        h = (int)(sd / sum * 100.0f);
        /* la luce arriva fino al bordo dell'apertura: si allarga e si rimisura */
        int want = MIN(rmax, h * 3 / 100 + 4);
        if (want <= r + 2) break;
        r = want;
    }

    *hfr100 = h;
    *peak_out = peak;
    *cx_out = (int)cx;
    *cy_out = (int)cy;
    *r_out = r;
    return 1;
}

static void stars_reset() { hfr_count = 0; hfr_best = 0; hfr_last = -1; hfr_peak = 0; star_x = -1; }

/* ---- fuoco automatico sulle stelle: curva a V dell'HFR
 * Si gira il fuoco a passi uguali misurando l'HFR (media di 3 misure dopo ogni passo).
 * Se peggiora subito si torna indietro e si prova dall'altra parte; quando l'HFR risale per due
 * passi il minimo e' passato. Una parabola sui punti intorno al minimo dice dove sta il fuoco;
 * ci si torna arrivando sempre dallo stesso verso della scansione, cosi' il gioco degli
 * ingranaggi dell'obiettivo non sposta il risultato. */
enum { AF_IDLE, AF_RUN, AF_DONE, AF_FAIL };
#define AF_MAXP 96
#define AF_MAXSTEPS 40
#define AF_BACKLASH 8                              /* ritorno per arrivare sul fuoco dallo stesso verso */
#define AF_WIN 6                                   /* passi intorno al migliore usati per la curva */
#define AF_RISE 112                                /* "peggiora" = HFR oltre il 12% del migliore */
static volatile int af_state = AF_IDLE, af_abort = 0;
static int af_pos, af_dir, af_n, af_phase, af_rise, af_lost, af_reversed, af_target, af_h0, af_hend, af_min;
static short af_px[AF_MAXP], af_ph[AF_MAXP];
static char af_msg[64];

static int af_measure()
{
    int sum = 0, n = 0;
    msleep(450);                                   /* il Live View mostra la stella dopo lo spostamento */
    for (int k = 0; k < 3; k++)
    {
        int h, p, cx, cy, r;
        if (hfr_measure(&h, &p, &cx, &cy, &r))
        {
            sum += h; n++;
            hfr_last = h; hfr_peak = p;
            if (hfr_best == 0 || h < hfr_best) hfr_best = h;
            star_x = LV2BM_X(cx); star_y = LV2BM_Y(cy); star_r = MAX(6, LV2BM_DX(r));
        }
        msleep(120);
    }
    return n ? sum / n : -1;
}

static int af_move(int steps)
{
    if (!steps) return 1;
    int dir = steps > 0 ? 1 : -1;
    if (!lens_focus(ABS(steps), dir * focus_step, 1, 0)) return 0;
    af_pos += steps;
    return 1;
}

static void af_fail(const char *why)
{
    snprintf(af_msg, sizeof(af_msg), "%s", why);
    af_state = AF_FAIL;
}

/* centro della curva a V. Una stella sfocata cresce come HFR^2 = a (x - x0)^2 + c, quindi si adatta
 * una parabola ai quadrati dell'HFR dei punti vicini al migliore: e' esatta per questa forma,
 * non solo vicino al minimo, e regge meglio il rumore della misura dove la curva e' piatta. */
static int af_vertex()
{
    int bi = af_min, lo = 1000, hi = -1000;
    float sx = 0, sx2 = 0, sx3 = 0, sx4 = 0, sy = 0, sxy = 0, sx2y = 0;
    int n = 0;
    for (int i = 0; i < af_n; i++)
    {
        int d = af_px[i] - af_px[bi];
        if (d < -AF_WIN || d > AF_WIN || af_ph[i] <= 0) continue;
        float x = (float)d, h = (float)af_ph[i] / 100.0f, y = h * h;
        sx += x; sx2 += x * x; sx3 += x * x * x; sx4 += x * x * x * x;
        sy += y; sxy += x * y; sx2y += x * x * y;
        lo = MIN(lo, d); hi = MAX(hi, d);
        n++;
    }
    if (n < 3) return af_px[bi];
    /* sistema normale per y = a x^2 + b x + c (regola di Cramer) */
    float N = (float)n;
    float D = sx4 * (sx2 * N - sx * sx) - sx3 * (sx3 * N - sx * sx2) + sx2 * (sx3 * sx - sx2 * sx2);
    if (fabsf(D) < 1e-6f) return af_px[bi];
    float Da = sx2y * (sx2 * N - sx * sx) - sx3 * (sxy * N - sx * sy) + sx2 * (sxy * sx - sx2 * sy);
    float Db = sx4 * (sxy * N - sy * sx) - sx2y * (sx3 * N - sx * sx2) + sx2 * (sx3 * sy - sxy * sx2);
    float a = Da / D, b = Db / D;
    if (a <= 0.0f) return af_px[bi];
    float v = -b / (2.0f * a);
    v = COERCE(v, (float)lo, (float)hi);
    return af_px[bi] + (int)(v + (v >= 0.0f ? 0.5f : -0.5f));
}

static void af_point(int h)
{
    if (af_n < AF_MAXP) { af_px[af_n] = (short)af_pos; af_ph[af_n] = (short)MIN(h, 32000); af_n++; }
    if (h > 0 && (af_min < 0 || h < af_ph[af_min])) { af_min = af_n - 1; af_rise = 0; }
    else if (h > 0 && h * 100 > af_ph[af_min] * AF_RISE) af_rise++;
}

/* un passo della macchina a stati: un movimento e una misura */
static void af_step()
{
    if (af_state != AF_RUN) return;
    if (af_phase == 0)
    {
        /* indietro e di nuovo avanti: il gioco degli ingranaggi e' gia' preso nel verso della scansione */
        if (!af_move(-af_dir * AF_BACKLASH) || !af_move(af_dir * AF_BACKLASH)) { af_fail("Fuoco automatico: il motore non risponde (AF attivo?)"); return; }
        af_h0 = af_measure();
        if (af_h0 < 0) { af_fail("Fuoco automatico: nessuna stella nel riquadro"); return; }
        af_point(af_h0);
        af_phase = 1;
        return;
    }
    if (af_phase == 1)
    {
        if (!af_move(af_dir)) { af_fail("Fuoco automatico: il motore non risponde (AF attivo?)"); return; }
        int h = af_measure();
        if (h < 0) { if (++af_lost >= 3) af_fail("Fuoco automatico: stella persa"); return; }
        af_lost = 0;
        af_point(h);
        if (ABS(af_pos) > AF_MAXSTEPS) { af_fail("Fuoco automatico: fuoco non trovato in 40 passi"); return; }
        if (af_rise < 2) return;
        if (!af_reversed && af_ph[af_min] * 100 > af_ph[0] * 95)
        {
            /* non migliora (il primo punto resta il migliore, a meno del rumore): si torna indietro e si ricomincia la curva nell'altro verso.
             * Il primo passo indietro recupera il gioco e non si misura; i punti vecchi si buttano,
             * cosi' tutti quelli della curva sono presi muovendo il fuoco nello stesso verso. */
            af_reversed = 1;
            af_dir = -af_dir;
            af_rise = 0; af_n = 0; af_min = -1;
            if (!af_move(af_dir)) { af_fail("Fuoco automatico: il motore non risponde"); return; }
            return;
        }
        af_target = af_vertex();
        /* indietro oltre il minimo, poi di nuovo avanti: si arriva sempre dallo stesso verso */
        int back = af_target - af_pos - af_dir * AF_BACKLASH;
        if (!af_move(back) || !af_move(af_dir * AF_BACKLASH)) { af_fail("Fuoco automatico: il motore non risponde"); return; }
        af_phase = 2;
        return;
    }
    if (af_phase == 2)
    {
        af_hend = af_measure();
        snprintf(af_msg, sizeof(af_msg), "Fuoco automatico: HFR %d.%02d -> %d.%02d",
                 af_h0 / 100, af_h0 % 100, MAX(af_hend, 0) / 100, MAX(af_hend, 0) % 100);
        af_state = AF_DONE;
    }
}

static void af_start()
{
    af_pos = 0; af_dir = 1; af_n = 0; af_phase = 0; af_rise = 0; af_lost = 0; af_reversed = 0;
    af_min = -1; af_h0 = af_hend = -1; af_abort = 0; af_msg[0] = 0;
    stars_reset();
    af_state = AF_RUN;
}

static void af_task(void *unused)
{
    while (af_state == AF_RUN)
    {
        if (af_abort || ml_shutdown_requested || !app_running || screen != SCR_STARS || !lv)
        {
            af_state = AF_FAIL;
            snprintf(af_msg, sizeof(af_msg), "Fuoco automatico fermato");
            break;
        }
        af_step();
        dirty = 1;
    }
    dirty = 1;
}

static void stars_enter() { stars_reset(); if (af_state != AF_RUN) af_state = AF_IDLE; }

static void stars_leave()
{
    if (lv && lv_dispsize != 1) set_lv_zoom(1);
}

/* misura ogni 250 ms: ritorna 1 se c'e' da ridisegnare */
static int stars_tick()
{
    if (!lv) { hfr_last = -1; star_x = -1; return 1; }
    if (af_state == AF_RUN) return 1;              /* misura il fuoco automatico */
    int h, p, cx, cy, r;
    if (hfr_measure(&h, &p, &cx, &cy, &r))
    {
        hfr_last = h; hfr_peak = p;
        hfr_hist[hfr_count % HFR_HIST] = h;
        hfr_count++;
        if (hfr_best == 0 || h < hfr_best) hfr_best = h;
        star_x = LV2BM_X(cx); star_y = LV2BM_Y(cy); star_r = MAX(6, LV2BM_DX(r));
    }
    else { hfr_last = -1; star_x = -1; }
    return 1;
}

static void draw_stars()
{
    /* centro trasparente: si vede il Live View */
    fill(COLOR_EMPTY, 0, 46, SCR_W, 398);
    draw_header("Fuoco stelle");
    char buf[64], a[12];
    fill(C_BG, 0, 46, SCR_W, 32);
    snprintf(buf, sizeof(buf), "zoom x%d     passo %d", lv_dispsize == 10 ? 10 : lv_dispsize == 5 ? 5 : 1, focus_step);
    txt(FONT_MED, C_DIM, 16, 50, buf);

    /* cornice dell'area di ricerca (la stessa usata dalla misura) */
    ring_r(C_LINE, SR_X0, SR_Y0, SR_X1 - SR_X0, SR_Y1 - SR_Y0, R_CARD, 1);
    if (star_x >= 0 && star_y > 46 && star_y < 336)
    {
        draw_circle(star_x, star_y, star_r, C_ACCENT);
        draw_circle(star_x, star_y, star_r + 1, C_ACCENT);
        draw_line(star_x - star_r - 10, star_y, star_x - star_r - 3, star_y, C_ACCENT);
        draw_line(star_x + star_r + 3, star_y, star_x + star_r + 10, star_y, C_ACCENT);
    }

    fill(C_BG, 0, 336, SCR_W, 108);
    if (!lv)
    {
        txt(FONT_MED, C_WARN, 16, 346, "Premi START/STOP: Iride accende il Live View e torna qui.");
        txt(FONT_MED, C_DIM, 16, 376, "Poi punta una stella luminosa al centro.");
    }
    else if (hfr_last < 0)
    {
        txt(FONT_MED, C_WARN, 16, 346, "Nessuna stella riconosciuta");
        txt(FONT_MED, C_DIM, 16, 376, "Centra una stella luminosa, zoom x10 con SET.");
    }
    else
    {
        snprintf(a, sizeof(a), "%d.%02d", hfr_last / 100, hfr_last % 100);
        snprintf(buf, sizeof(buf), "HFR %s", a);
        int good = hfr_best > 0 && hfr_last * 100 <= hfr_best * 105;
        txt(FONT_LARGE, good ? C_OK : C_TEXT, 16, 344, buf);
        /* tendenza rispetto alla media delle 3 misure precedenti */
        if (hfr_count >= 4)
        {
            int prev = 0;
            for (int k = 2; k <= 4; k++) prev += hfr_hist[(hfr_count - k) % HFR_HIST];
            prev /= 3;
            const char *tr = hfr_last * 100 < prev * 97 ? "meglio" : hfr_last * 100 > prev * 103 ? "peggio" : 0;
            if (tr) txt(FONT_MED, hfr_last < prev ? C_OK : C_WARN, 16 + txt_w(FONT_LARGE, buf) + 14, 352, tr);
        }
        if (hfr_peak >= 250) snprintf(buf, sizeof(buf), "migliore %d.%02d   stella satura: abbassa ISO", hfr_best / 100, hfr_best % 100);
        else snprintf(buf, sizeof(buf), "migliore %d.%02d   picco %d", hfr_best / 100, hfr_best % 100, hfr_peak);
        txt(FONT_MED, hfr_peak >= 250 ? C_WARN : C_DIM, 16, 392, buf);
    }
    int gx = 380, gy = 344, gw = 320, gh = 88;
    ring_r(C_LINE, gx - 2, gy - 2, gw + 4, gh + 4, R_ROW, 1);
    if (af_state == AF_RUN || ((af_state == AF_DONE || af_state == AF_FAIL) && af_n > 1))
    {
        /* la curva a V: posizione del fuoco in orizzontale, HFR in verticale */
        int lo = 0, hi = 0, mx = 1;
        for (int i = 0; i < af_n; i++) { lo = MIN(lo, af_px[i]); hi = MAX(hi, af_px[i]); mx = MAX(mx, af_ph[i]); }
        int span = MAX(hi - lo, 1);
        for (int i = 0; i < af_n; i++)
        {
            int x = gx + 6 + (af_px[i] - lo) * (gw - 12) / span, y = gy + gh - 4 - af_ph[i] * (gh - 10) / mx;
            disc(x, y, i == af_min ? 4 : 2, i == af_min ? C_OK : C_ACCENT);
        }
        if (af_state == AF_DONE)
        {
            int x = gx + 6 + (af_target - lo) * (gw - 12) / span;
            fill(C_OK, x, gy, 1, gh);
        }
    }
    else if (hfr_count > 0)
    {
        int n = MIN(hfr_count, HFR_HIST), maxv = 1;
        for (int i = 0; i < n; i++) maxv = MAX(maxv, hfr_hist[(hfr_count - n + i) % HFR_HIST]);
        int bw = gw / HFR_HIST;
        for (int i = 0; i < n; i++)
        {
            int val = hfr_hist[(hfr_count - n + i) % HFR_HIST];
            int bh = MAX(2, val * gh / maxv);
            fill((hfr_best > 0 && val * 100 <= hfr_best * 105) ? C_OK : C_ACCENT, gx + i * bw, gy + gh - bh, MAX(bw - 1, 1), bh);
        }
    }
    if (af_state == AF_RUN)
    {
        snprintf(buf, sizeof(buf), "Fuoco automatico: passo %+d", af_pos);
        fill(C_BG, 0, 386, 370, 34);
        txt(FONT_MED, C_ACCENT, 16, 392, buf);
        anim_keep();
    }
    else if (af_state == AF_DONE || af_state == AF_FAIL)
    {
        fill(C_BG, 0, 386, 370, 34);
        txt_fit(af_state == AF_DONE ? C_OK : C_WARN, 16, 392, 354, af_msg);
    }
    draw_footer(af_state == AF_RUN ? "Q ferma il fuoco automatico" :
                lv ? "SET zoom  </> fuoco  ^/v passo  Q auto  INFO aiuto" : "START/STOP accende il Live View   MENU indietro");
}

static void stars_key(int key, int dx, int dy)
{
    if (af_state == AF_RUN)
    {
        /* durante il fuoco automatico ogni tasto lo ferma */
        af_abort = 1;
        return;
    }
    if (af_state == AF_DONE || af_state == AF_FAIL) { say(af_state == AF_DONE ? C_OK : C_WARN, af_msg); af_state = AF_IDLE; }
    if (key == MODULE_KEY_Q)
    {
        if (!lv) { say(C_WARN, "Serve il Live View"); return; }
        if (is_manual_focus()) { say(C_WARN, "Fuoco automatico: obiettivo su AF, non MF"); return; }
        if (hfr_last < 0) { say(C_WARN, "Centra prima una stella nel riquadro"); return; }
        af_start();
        task_create("iride_af", 0x1d, 0x2000, af_task, (void *)0);
        return;
    }
    if (dx)
    {
        if (!lv) { say(C_WARN, "Serve il Live View"); return; }
        if (is_manual_focus()) { say(C_WARN, "Obiettivo in MF: gira la ghiera a mano"); return; }
        if (!lens_focus(1, dx * focus_step, 1, 0)) say(C_WARN, "Il motore di fuoco non risponde (AF attivo?)");
    }
    else if (dy) focus_step = COERCE(focus_step - dy, 1, 3);
    else if (key == MODULE_KEY_PRESS_SET)
    {
        if (!lv) { say(C_WARN, "Serve il Live View"); return; }
        set_lv_zoom(lv_dispsize == 1 ? 5 : lv_dispsize == 5 ? 10 : 1);
        stars_reset();
    }
}

/* ------------------------------------------------------------------ astronomia di base */

#define DEG2RAD 0.017453293f
#define RAD2DEG 57.29578f

/* tempo come giorni interi + frazione dall'epoca J2000.0 (1/1/2000 12:00 UTC):
 * la frazione separata mantiene la precisione dei float */
struct jtime { int d; float f; };

static int days_from_civil(int y, int m, int d)
{
    y -= m <= 2;
    int era = (y >= 0 ? y : y - 399) / 400;
    int yoe = y - era * 400;
    int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;   /* giorni dal 1/1/1970 */
}

static int weekday(int days1970) { return MOD(days1970 + 4, 7); }   /* 0 = domenica */

static int days_in_month(int y, int m)
{
    static const char dm[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (m == 2) return 28 + ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0);
    return dm[COERCE(m, 1, 12) - 1];
}

static int last_sunday(int y, int m)
{
    int dim = days_in_month(y, m);
    return dim - weekday(days_from_civil(y, m, dim));
}

/* n-esima domenica del mese (n = 1, 2, ...) */
static int nth_sunday(int y, int m, int n)
{
    int w = weekday(days_from_civil(y, m, 1));
    return 1 + (7 - w) % 7 + (n - 1) * 7;
}

/* ora legale europea: dall'ultima domenica di marzo all'ultima di ottobre, alle 01:00 UTC */
static int eu_dst(int y, int m, int d, int utc_hour)
{
    if (m < 3 || m > 10) return 0;
    if (m > 3 && m < 10) return 1;
    int ls = last_sunday(y, m);
    if (m == 3) return d > ls || (d == ls && utc_hour >= 1);
    return d < ls || (d == ls && utc_hour < 1);
}

/* ---- fusi orari. tz_mode: 0 = Europa centrale (Italia) con l'ora legale, 1..4 = UTC+0..+3 fisso
 * (versioni fino alla 1.1), 10..17 = zone con l'ora legale, 100..204 = fisso a quarti d'ora da UTC-12 a UTC+14.
 * Regole dal database IANA dei fusi (tzdata 2026); le ore sono quelle dell'orologio. */
enum { DST_NONE, DST_EU, DST_US, DST_AU, DST_NZ };
struct tzdef { short code, std_min; char rule; const char *name; };
static const struct tzdef tzdefs[] = {
    {  0,   60, DST_EU, "Europa centrale" },
    { 10,    0, DST_EU, "Europa occidentale" },
    { 11,  120, DST_EU, "Europa orientale" },
    { 12, -300, DST_US, "USA est" },
    { 13, -360, DST_US, "USA centrale" },
    { 14, -420, DST_US, "USA montagne" },
    { 15, -480, DST_US, "USA Pacifico" },
    { 16,  600, DST_AU, "Australia sud-est" },
    { 17,  720, DST_NZ, "Nuova Zelanda" },
};
#define TZ_FIXED(off_min) (100 + ((off_min) + 720) / 15)
#define TZ_FIXED_MIN 100
#define TZ_FIXED_MAX 204

static const struct tzdef *tz_def(int code)
{
    for (int i = 0; i < (int)COUNT(tzdefs); i++) if (tzdefs[i].code == code) return &tzdefs[i];
    return 0;
}

static int tz_valid(int code)
{
    return tz_def(code) || (code >= 1 && code <= 4) || (code >= TZ_FIXED_MIN && code <= TZ_FIXED_MAX);
}

/* scarto fisso o d'inverno in minuti */
static int tz_std_min(int code)
{
    const struct tzdef *t = tz_def(code);
    if (t) return t->std_min;
    if (code >= 1 && code <= 4) return (code - 1) * 60;
    if (code >= TZ_FIXED_MIN && code <= TZ_FIXED_MAX) return (code - 100) * 15 - 720;
    return 60;
}

static int tz_dst(int rule, int y, int m, int d, int h, int std_min)
{
    switch (rule)
    {
        case DST_EU: return eu_dst(y, m, d, h - std_min / 60);
        case DST_US:    /* dalla seconda domenica di marzo alla prima di novembre, alle 2 */
            if (m < 3 || m > 11) return 0;
            if (m > 3 && m < 11) return 1;
            if (m == 3) { int s = nth_sunday(y, 3, 2); return d > s || (d == s && h >= 2); }
            { int s = nth_sunday(y, 11, 1); return d < s || (d == s && h < 2); }
        case DST_AU:    /* dalla prima domenica di ottobre alla prima di aprile */
            if (m > 4 && m < 10) return 0;
            if (m > 10 || m < 4) return 1;
            if (m == 10) { int s = nth_sunday(y, 10, 1); return d > s || (d == s && h >= 2); }
            { int s = nth_sunday(y, 4, 1); return d < s || (d == s && h < 3); }
        case DST_NZ:    /* dall'ultima domenica di settembre alla prima di aprile */
            if (m > 4 && m < 9) return 0;
            if (m > 9 || m < 4) return 1;
            if (m == 9) { int s = last_sunday(y, 9); return d > s || (d == s && h >= 2); }
            { int s = nth_sunday(y, 4, 1); return d < s || (d == s && h < 3); }
    }
    return 0;
}

static int utc_offset_min(int y, int m, int d, int local_hour)
{
    int code = tz_valid(P.tz_mode) ? P.tz_mode : 0;
    int std = tz_std_min(code);
    const struct tzdef *t = tz_def(code);
    return std + (t && tz_dst(t->rule, y, m, d, local_hour, std) ? 60 : 0);
}

/* "UTC+5:30", "UTC-3", "UTC" */
static void fmt_utc(char *buf, int len, int off)
{
    if (!off) { snprintf(buf, len, "UTC"); return; }
    int a = ABS(off);
    if (a % 60) snprintf(buf, len, "UTC%c%d:%02d", off < 0 ? '-' : '+', a / 60, a % 60);
    else snprintf(buf, len, "UTC%c%d", off < 0 ? '-' : '+', a / 60);
}

/* nome del fuso per le opzioni */
static void tz_name(int code, char *buf, int len)
{
    const struct tzdef *t = tz_def(code);
    if (t) { snprintf(buf, len, "%s, ora legale auto", t->name); return; }
    char u[16];
    fmt_utc(u, sizeof(u), tz_std_min(code));
    snprintf(buf, len, "%s fisso", u);
}

/* fuso successivo o precedente: prima le zone con l'ora legale, poi gli scarti fissi */
static int tz_next(int code, int dir)
{
    if (code >= 1 && code <= 4) code = TZ_FIXED((code - 1) * 60);     /* vecchi valori: stesso scarto */
    int nz = (int)COUNT(tzdefs), n = nz + (TZ_FIXED_MAX - TZ_FIXED_MIN + 1), i = 0;
    for (int k = 0; k < nz; k++) if (tzdefs[k].code == code) i = k;
    if (code >= TZ_FIXED_MIN && code <= TZ_FIXED_MAX) i = nz + code - TZ_FIXED_MIN;
    i = MOD(i + dir, n);
    return i < nz ? tzdefs[i].code : TZ_FIXED_MIN + i - nz;
}

/* ora locale (giorni dal 1970 + minuti) -> jtime */
static struct jtime jt_from_local(int days1970, int minutes, int offset_min)
{
    int um = minutes - offset_min;
    int dd = days1970 + (um >= 0 ? um / 1440 : -((-um + 1439) / 1440));
    um = MOD(um, 1440);
    struct jtime j;
    /* J2000.0 = 10957 giorni dopo il 1/1/1970, a mezzogiorno */
    j.d = dd - 10957;
    j.f = (float)um / 1440.0f - 0.5f;
    if (j.f < 0.0f) { j.f += 1.0f; j.d -= 1; }
    return j;
}

static struct jtime jt_now(int *days1970, int *minutes, int *offset)
{
    struct tm now;
    LoadCalendarFromRTC(&now);
    int y = now.tm_year + 1900, m = now.tm_mon + 1, d = now.tm_mday;
    *days1970 = days_from_civil(y, m, d);
    *minutes = now.tm_hour * 60 + now.tm_min;
    *offset = utc_offset_min(y, m, d, now.tm_hour);
    return jt_from_local(*days1970, *minutes, *offset);
}

static float wrap360(float a) { a = fmodf(a, 360.0f); return a < 0.0f ? a + 360.0f : a; }

/* moltiplica un coefficiente per (d + f) senza perdere precisione, modulo 360 */
static float lin360(float base, float rate, struct jtime j)
{
    return wrap360(base + fmodf(rate * (float)j.d, 360.0f) + rate * j.f);
}

/* tempo siderale locale in gradi */
static float lst_deg(struct jtime j, float lon)
{
    float g = 280.46061837f + fmodf(0.98564736629f * (float)j.d, 360.0f) + 360.98564736629f * j.f;
    return wrap360(g + lon);
}

static void ecl_to_eq(float lam, float bet, float eps, float *ra, float *dec)
{
    float sl = sinf(lam * DEG2RAD), cl = cosf(lam * DEG2RAD);
    float sb = sinf(bet * DEG2RAD), cb = cosf(bet * DEG2RAD);
    float se = sinf(eps * DEG2RAD), ce = cosf(eps * DEG2RAD);
    *ra = wrap360(atan2f(sl * ce * cb - sb * se, cl * cb) * RAD2DEG);
    *dec = asinf(sb * ce + cb * se * sl) * RAD2DEG;
}

/* Sole, precisione ~0,01 gradi (Astronomical Almanac, formula breve) */
static void sun_pos(struct jtime j, float *ra, float *dec, float *lam_out)
{
    float L = lin360(280.460f, 0.9856474f, j);
    float g = lin360(357.528f, 0.9856003f, j) * DEG2RAD;
    float lam = L + 1.915f * sinf(g) + 0.020f * sinf(2.0f * g);
    float eps = 23.439f - 0.0000004f * (float)j.d;
    ecl_to_eq(lam, 0.0f, eps, ra, dec);
    if (lam_out) *lam_out = wrap360(lam);
}

/* Luna, termini principali (errore ~0,3 gradi) */
static void moon_pos(struct jtime j, float *ra, float *dec, float *lam_out)
{
    float Lp = lin360(218.316f, 13.176396f, j);
    float Mp = lin360(134.963f, 13.064993f, j) * DEG2RAD;
    float F  = lin360(93.272f, 13.229350f, j) * DEG2RAD;
    float D  = lin360(297.850f, 12.190749f, j) * DEG2RAD;
    float Ms = lin360(357.529f, 0.98560028f, j) * DEG2RAD;
    float lam = Lp + 6.289f * sinf(Mp) - 1.274f * sinf(Mp - 2.0f * D) + 0.658f * sinf(2.0f * D)
              - 0.186f * sinf(Ms) - 0.059f * sinf(2.0f * Mp - 2.0f * D) - 0.057f * sinf(Mp - 2.0f * D + Ms)
              + 0.053f * sinf(Mp + 2.0f * D) + 0.046f * sinf(2.0f * D - Ms) + 0.041f * sinf(Mp - Ms)
              - 0.035f * sinf(D) - 0.031f * sinf(Mp + Ms) + 0.214f * sinf(2.0f * Mp);
    float bet = 5.128f * sinf(F) + 0.281f * sinf(Mp + F) + 0.278f * sinf(Mp - F) + 0.173f * sinf(2.0f * D - F);
    float eps = 23.439f - 0.0000004f * (float)j.d;
    ecl_to_eq(lam, bet, eps, ra, dec);
    if (lam_out) *lam_out = wrap360(lam);
}

static float altitude(float ra, float dec, float lst, float lat)
{
    float h = (lst - ra) * DEG2RAD;
    float s = sinf(lat * DEG2RAD) * sinf(dec * DEG2RAD) + cosf(lat * DEG2RAD) * cosf(dec * DEG2RAD) * cosf(h);
    return asinf(COERCE(s, -1.0f, 1.0f)) * RAD2DEG;
}

/* frazione illuminata della Luna (millesimi) e fase crescente/calante */
static int moon_illum(struct jtime j, int *waxing)
{
    float r, d, ls, lm;
    sun_pos(j, &r, &d, &ls);
    moon_pos(j, &r, &d, &lm);
    float e = wrap360(lm - ls);
    if (waxing) *waxing = e < 180.0f;
    return (int)((1.0f - cosf(e * DEG2RAD)) * 500.0f);
}

/* ------------------------------------------------------------------ allineamento polare */

/* Polare (FK5, precessione calcolata con astropy per 2026-2036) */
static void polaris(struct jtime j, float *ra_deg, float *dec_deg)
{
    float dy = ((float)j.d + j.f - 9496.5f) / 365.25f;   /* anni dal 1/1/2026 */
    *ra_deg = (3.09745f + 0.025356f * dy + 0.0001604f * dy * dy) * 15.0f;
    *dec_deg = 89.37159f + 0.00377f * dy;
}

/* citta' pronte: coordinate da GeoNames, fuso dal database IANA (con l'ora legale dove c'e') */
struct city { const char *name; short lat100, lon100; unsigned char tz; };
static const struct city cities[] = {
    { "Aosta", 4574, 732, 0 },
    { "Torino", 4507, 769, 0 },
    { "Genova", 4440, 894, 0 },
    { "Milano", 4546, 919, 0 },
    { "Trento", 4607, 1112, 0 },
    { "Bolzano", 4649, 1134, 0 },
    { "Venezia", 4544, 1233, 0 },
    { "Padova", 4541, 1189, 0 },
    { "Rovigo", 4507, 1179, 0 },
    { "Ferrara", 4484, 1162, 0 },
    { "Verona", 4544, 1099, 0 },
    { "Trieste", 4565, 1378, 0 },
    { "Bologna", 4449, 1134, 0 },
    { "Firenze", 4378, 1125, 0 },
    { "Ancona", 4361, 1351, 0 },
    { "Perugia", 4311, 1239, 0 },
    { "Roma", 4189, 1251, 0 },
    { "L'Aquila", 4235, 1340, 0 },
    { "Campobasso", 4156, 1467, 0 },
    { "Napoli", 4085, 1427, 0 },
    { "Bari", 4112, 1687, 0 },
    { "Potenza", 4064, 1581, 0 },
    { "Catanzaro", 3888, 1660, 0 },
    { "Palermo", 3812, 1336, 0 },
    { "Cagliari", 3923, 912, 0 },
    { "Reykjavik", 6414, -2190, 148 },
    { "Lisbona", 3873, -915, 10 },
    { "Londra", 5151, -13, 10 },
    { "Madrid", 4042, -370, 0 },
    { "Tenerife", 2847, -1625, 10 },
    { "Parigi", 4885, 235, 0 },
    { "Berlino", 5252, 1341, 0 },
    { "Vienna", 4821, 1637, 0 },
    { "Atene", 3798, 2373, 11 },
    { "Helsinki", 6017, 2494, 11 },
    { "Istanbul", 4101, 2895, 160 },
    { "Mosca", 5575, 3762, 160 },
    { "Citta' del Capo", -3393, 1842, 156 },
    { "Nairobi", -128, 3682, 160 },
    { "Dubai", 2508, 5531, 164 },
    { "Nuova Delhi", 2862, 7721, 170 },
    { "Kathmandu", 2770, 8532, 171 },
    { "Bangkok", 1375, 10050, 176 },
    { "Pechino", 3991, 11640, 180 },
    { "Tokyo", 3569, 13969, 184 },
    { "Perth", -3195, 11586, 180 },
    { "Sydney", -3387, 15121, 16 },
    { "Auckland", -3685, 17476, 17 },
    { "Honolulu", 2131, -15786, 108 },
    { "Los Angeles", 3405, -11824, 15 },
    { "Phoenix", 3345, -11207, 120 },
    { "Denver", 3974, -10498, 14 },
    { "Citta' del Messico", 1943, -9913, 124 },
    { "Chicago", 4185, -8765, 13 },
    { "New York", 4071, -7401, 12 },
    { "Buenos Aires", -3461, -5838, 136 },
    { "San Paolo", -2355, -4664, 136 },
};
#define NUM_CITIES ((int)COUNT(cities))

/* la citta' che corrisponde al luogo impostato, -1 se il luogo e' personalizzato */
static int city_find()
{
    for (int i = 0; i < NUM_CITIES; i++)
        if (cities[i].lat100 == P.lat100 && cities[i].lon100 == P.lon100 && cities[i].tz == P.tz_mode) return i;
    return -1;
}

/* la piu' vicina (in gradi, con la longitudine accorciata verso i poli) */
static int city_nearest()
{
    int best = 0;
    float bd = 1e9f, cl = cosf((float)P.lat100 * 0.00017453293f);
    for (int i = 0; i < NUM_CITIES; i++)
    {
        float dla = (float)(cities[i].lat100 - P.lat100);
        float dlo = (float)(cities[i].lon100 - P.lon100);
        if (dlo > 18000.0f) dlo -= 36000.0f;
        if (dlo < -18000.0f) dlo += 36000.0f;
        float d = dla * dla + dlo * dlo * cl * cl;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

static void city_set(int i)
{
    i = MOD(i, NUM_CITIES);
    P.lat100 = cities[i].lat100; P.lon100 = cities[i].lon100; P.tz_mode = cities[i].tz;
    persist_touch();
}

/* "45.07 N  11.79 E" */
static void fmt_latlon(char *buf, int len)
{
    snprintf(buf, len, "%d.%02d %c  %d.%02d %c", ABS(P.lat100) / 100, ABS(P.lat100) % 100, P.lat100 < 0 ? 'S' : 'N',
             ABS(P.lon100) / 100, ABS(P.lon100) % 100, P.lon100 < 0 ? 'O' : 'E');
}

static float lat_deg() { return (float)P.lat100 / 100.0f; }
static float lon_deg() { return (float)P.lon100 / 100.0f; }

/* angolo orario della Polare in ore (0..24) */
static float polar_ha_hours(struct jtime j)
{
    float ra, dec;
    polaris(j, &ra, &dec);
    return wrap360(lst_deg(j, lon_deg()) - ra) / 15.0f;
}

/* posizione della Polare sull'orologio, vista a occhio nudo */
static void polar_clock(int *h, int *m)
{
    int d70, mins, off;
    float ha = polar_ha_hours(jt_now(&d70, &mins, &off));
    float clock = fmodf(12.0f - ha / 2.0f + 24.0f, 12.0f);
    *h = (int)clock; *m = (int)((clock - (float)*h) * 60.0f);
    if (*h == 0) *h = 12;
}

/* stelle guida intorno al polo (J2000; la precessione non si vede alla scala della mappa) */
struct guide_star { float ra_h, dec; };
static const struct guide_star uma[7] = {        /* Grande Carro: Dubhe, Merak, Phecda, Megrez, Alioth, Mizar, Alkaid */
    { 11.0621f, 61.751f }, { 11.0307f, 56.382f }, { 11.8972f, 53.695f }, { 12.2571f, 57.033f },
    { 12.9005f, 55.960f }, { 13.3988f, 54.925f }, { 13.7923f, 49.313f } };
static const struct guide_star cas[5] = {        /* Cassiopea: Caph, Schedar, Navi, Ruchbah, Segin */
    { 0.1530f, 59.150f }, { 0.6751f, 56.537f }, { 0.9451f, 60.717f }, { 1.4303f, 60.235f }, { 1.9066f, 63.670f } };
static const signed char uma_lines[7][2] = { {0, 1}, {1, 2}, {2, 3}, {3, 0}, {3, 4}, {4, 5}, {5, 6} };

static int polar_t0 = 0, polar_ping = 0;          /* entrata (lancetta che arriva) e impulso ogni secondo */
static struct glider polar_rot = { -1, 0, 0, 0 }; /* 0 = occhio nudo, 180000 = cannocchiale (gradi x1000) */
static struct glider polar_seg = { -1, 0, 0, 0 };

static void polar_enter() { polar_t0 = get_ms_clock(); polar_rot.vis = -1; polar_seg.vis = -1; }
static int polar_tick() { polar_ping = get_ms_clock(); return 1; }   /* la Polare si muove: ridisegna */

/* posizione sul quadrante: angolo orario (gradi) e distanza in pixel; vista a occhio nudo
   guardando a nord: in alto lo zenit, le stelle girano in senso antiorario intorno al polo */
static void sky_xy(int cx, int cy, float ha_deg, float r, int *x, int *y)
{
    float a = ha_deg * DEG2RAD;
    *x = cx - (int)(r * sinf(a));
    *y = cy - (int)(r * cosf(a));
}

/* mappa del cielo a nord per orientarsi: Grande Carro, Cassiopea, Polare e orizzonte */
static void draw_north_map(int mx, int my, int mr, float lst, float lat)
{
    const float k = (float)mr / 46.0f;                /* 46 gradi dal polo fino al bordo */
    disc(mx, my, mr, C_PANEL);
    /* terreno: sotto l'orizzonte, a "lat" gradi dal polo verso il basso */
    int hy = my + (int)(lat * k);
    for (int yy = MAX(hy, my - mr); yy <= my + mr; yy++)
    {
        int half = (int)sqrtf((float)MAX(mr * mr - (yy - my) * (yy - my), 0));
        fill(night ? COLOR_BLACK : COLOR_GRAY(4), mx - half, yy, 2 * half + 1, 1);
    }
    if (hy < my + mr)
    {
        int half = (int)sqrtf((float)MAX(mr * mr - (hy - my) * (hy - my), 0));
        fill(C_LINE, mx - half, hy, 2 * half + 1, 1);
        txt_c(FONT_SMALL, C_DIM, mx, hy + 3, "N");
    }
    draw_circle(mx, my, mr, C_LINE);
    int px[7], py[7];
    for (int i = 0; i < 7; i++)
        sky_xy(mx, my, lst - uma[i].ra_h * 15.0f, (90.0f - uma[i].dec) * k, &px[i], &py[i]);
    for (int i = 0; i < 7; i++)
        draw_line(px[uma_lines[i][0]], py[uma_lines[i][0]], px[uma_lines[i][1]], py[uma_lines[i][1]], tc(COLOR_GRAY(40)));
    for (int i = 0; i < 7; i++) disc(px[i], py[i], i == 3 ? 1 : 2, tc(COLOR_WHITE));
    int qx[5], qy[5];
    for (int i = 0; i < 5; i++)
        sky_xy(mx, my, lst - cas[i].ra_h * 15.0f, (90.0f - cas[i].dec) * k, &qx[i], &qy[i]);
    for (int i = 0; i < 4; i++) draw_line(qx[i], qy[i], qx[i + 1], qy[i + 1], tc(COLOR_GRAY(40)));
    for (int i = 0; i < 5; i++) disc(qx[i], qy[i], 2, tc(COLOR_LIGHT_BLUE));
    /* polo e Polare (a meno di un grado: praticamente al centro) */
    fill(C_DIM, mx - 5, my, 11, 1);
    fill(C_DIM, mx, my - 5, 1, 11);
    disc(mx + 1, my - 1, 2, C_ACCENT);
}

/* interruttore a due voci con il fondo che scivola */
static void draw_segments(int x, int y, int w, int h, const char *a, const char *b, int pos)
{
    int half = w / 2;
    fill_r(C_PANEL, x, y, w, h, R_ROW);
    int sx = x + half * glide(&polar_seg, pos * 1000, 220) / 1000;
    fill_r(C_SEL, sx, y, half, h, R_ROW);
    fill_rrc(C_ACCENT, sx, y + h - 3, half, 3, 3, RC_BL | RC_BR, -10000, 10000);
    txt_c(FONT_MED, pos == 0 ? C_TEXT : C_DIM, x + half / 2, y + (h - 23) / 2, a);
    txt_c(FONT_MED, pos == 1 ? C_TEXT : C_DIM, x + half + half / 2, y + (h - 23) / 2, b);
}

static void draw_polar()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Allineamento polare");
    int d70, mins, off;
    struct jtime j = jt_now(&d70, &mins, &off);
    float ha = polar_ha_hours(j);
    float ra, dec;
    polaris(j, &ra, &dec);
    int inverted = P.polar_view;
    int now = get_ms_clock();

    /* ---- reticolo del cannocchiale polare */
    const int cx = 196, cy = 246, R = 150, rp = 100;
    disc(cx, cy, R, night ? COLOR_BLACK : COLOR_GRAY(10));
    disc(cx, cy, R - 18, C_BG);
    draw_circle(cx, cy, R, C_LINE);
    draw_circle(cx, cy, R - 18, C_LINE);
    for (int t = 0; t < 48; t++)                       /* tacche ogni 15 minuti dell'orologio */
    {
        float a = (float)t * 7.5f * DEG2RAD, sa = sinf(a), ca = cosf(a);
        int len = t % 4 == 0 ? 18 : t % 2 == 0 ? 9 : 5;
        int c = t % 4 == 0 ? C_DIM : C_LINE;
        int r0 = R - 18, r1 = r0 + len;
        draw_line(cx + (int)((float)r0 * sa), cy - (int)((float)r0 * ca), cx + (int)((float)r1 * sa), cy - (int)((float)r1 * ca), c);
        if (t % 12 == 0)                               /* 12, 3, 6, 9 piu' marcati */
            draw_line(cx + (int)((float)r0 * sa) + 1, cy - (int)((float)r0 * ca), cx + (int)((float)r1 * sa) + 1, cy - (int)((float)r1 * ca), C_TEXT);
    }
    for (int h = 0; h < 12; h++)
    {
        float a = (float)h * 30.0f * DEG2RAD;
        char n[8];
        snprintf(n, sizeof(n), "%d", h == 0 ? 12 : h);
        txt_c(FONT_SMALL, h % 3 == 0 ? C_TEXT : C_DIM, cx + (int)((float)(R + 13) * sinf(a)), cy - (int)((float)(R + 13) * cosf(a)) - 6, n);
    }
    /* croce con lo spazio al centro, cerchio della Polare e cerchio interno */
    int cross = tc(COLOR_GRAY(24));
    fill(cross, cx - R + 24, cy, R - 34, 1); fill(cross, cx + 10, cy, R - 34, 1);
    fill(cross, cx, cy - R + 24, 1, R - 34); fill(cross, cx, cy + 10, 1, R - 34);
    draw_circle(cx, cy, rp, tc(COLOR_GRAY(34)));
    draw_circle(cx, cy, rp + 1, tc(COLOR_GRAY(20)));
    draw_circle(cx, cy, rp / 2, tc(COLOR_GRAY(16)));
    draw_circle(cx, cy, 4, C_DIM);
    char buf[64];
    snprintf(buf, sizeof(buf), "%d'", (int)((90.0f - dec) * 60.0f + 0.5f));
    txt(FONT_SMALL, C_DIM, cx + rp * 71 / 100 + 6, cy + rp * 71 / 100 - 2, buf);

    /* lancetta: all'entrata parte dalle 12; cambiando vista ruota di mezzo giro */
    float target = ha * 15.0f;
    if (target > 180.0f) target -= 360.0f;            /* la lancetta prende la via piu' corta */
    int ee = phase(now - polar_t0, 0, anim_ms(750), EZ_BACK);
    if (ee < 1000) anim_keep();
    float shown = target * (float)ee / 1000.0f + (float)glide(&polar_rot, inverted ? 180000 : 0, 420) / 1000.0f;
    int px, py;
    sky_xy(cx, cy, shown, (float)rp, &px, &py);
    draw_line(cx, cy, px, py, C_ACCENT);
    draw_line(cx + 1, cy, px + 1, py, C_ACCENT);
    /* impulso: un anello che si allarga e si spegne dopo ogni aggiornamento */
    int ep = anim_e(polar_ping, 650, EZ_OUT);
    if (ep < 1000 && ee >= 1000)
    {
        int c = fade_c(C_ACCENT, 1000 - ep);
        draw_circle(px, py, lerp(11, 26, ep), c);
        draw_circle(px, py, lerp(11, 26, ep) + 1, c);
    }
    disc(px, py, 10, C_ACCENT);
    disc(px, py, 5, C_BG);
    disc(px, py, 2, C_TEXT);

    /* ---- pannello a destra */
    float clock = 12.0f - ha / 2.0f + (inverted ? 6.0f : 0.0f);
    clock = fmodf(clock + 24.0f, 12.0f);
    int ch = (int)clock, cm = (int)((clock - (float)ch) * 60.0f);
    const int x2 = 388;
    int e1 = phase(now - polar_t0, 0, anim_ms(300), EZ_OUT), e2 = phase(now - polar_t0, anim_ms(80), anim_ms(300), EZ_OUT);
    int e3 = phase(now - polar_t0, anim_ms(160), anim_ms(300), EZ_OUT), e4 = phase(now - polar_t0, anim_ms(240), anim_ms(300), EZ_OUT);
    txt_in(FONT_SMALL, C_DIM, x2, 56, "POSIZIONE DELLA POLARE", e1, 20, 0);
    snprintf(buf, sizeof(buf), "ore %d:%02d", ch == 0 ? 12 : ch, cm);
    txt_in(FONT_LARGE, C_ACCENT, x2, 72, buf, e1, 20, 0);
    float to_transit = (24.0f - ha) * 0.99727f;          /* ore siderali -> ore solari */
    int tt = (int)(to_transit * 60.0f + 0.5f);
    char at[8];
    fmt_hm(at, sizeof(at), mins + tt);
    snprintf(buf, sizeof(buf), "Culmina tra %dh %02dm, alle %s", tt / 60, tt % 60, at);
    txt_in(FONT_MED, C_TEXT, x2, 112, buf, e2, 20, 0);
    int hah = (int)ha, ham = (int)((ha - (float)hah) * 60.0f);
    snprintf(buf, sizeof(buf), "Angolo orario %02dh %02dm", hah, ham);
    txt_in(FONT_MED, C_DIM, x2, 138, buf, e2, 20, 0);

    /* mappa per trovare la Polare (sempre a occhio nudo) */
    const int mx = 456, my = 248, mr = 72;
    if (e3 > 0) draw_north_map(mx, my, mr, lst_deg(j, lon_deg()), lat_deg());
    txt_in(FONT_SMALL, C_DIM, 540, 182, "CIELO A NORD", e3, 16, 0);
    if (e3 >= 1000)
    {
        disc(546, 207, 3, tc(COLOR_WHITE));
        disc(546, 227, 3, tc(COLOR_LIGHT_BLUE));
        disc(546, 247, 3, C_ACCENT);
    }
    txt_in(FONT_SMALL, C_TEXT, 560, 200, "Grande Carro", e3, 16, 0);
    txt_in(FONT_SMALL, C_TEXT, 560, 220, "Cassiopea", e3, 16, 0);
    txt_in(FONT_SMALL, C_TEXT, 560, 240, "Polare", e3, 16, 0);
    txt_in(FONT_SMALL, C_DIM, 540, 268, "Tra le due, quasi", e3, 16, 0);
    txt_in(FONT_SMALL, C_DIM, 540, 284, "a meta': la Polare.", e3, 16, 0);

    draw_segments(x2, 336, 312, 34, "Occhio nudo", "Cannocchiale", inverted);
    txt_in(FONT_SMALL, C_DIM, x2, 378, inverted ? "Immagine capovolta, come nel cannocchiale" : "Guardando a nord, senza ottiche", e4, 0, 6);
    char ll[32], u[16];
    int ci = city_find();
    fmt_latlon(ll, sizeof(ll));
    fmt_utc(u, sizeof(u), off);
    snprintf(buf, sizeof(buf), "%s%s%s   %s", ci >= 0 ? cities[ci].name : "", ci >= 0 ? "  " : "", ll, u);
    txt_in(FONT_SMALL, C_DIM, x2, 398, buf, e4, 0, 6);
    draw_footer("SET o </> cambia vista   INFO aiuto   MENU indietro");
}

static void polar_key(int key, int dx, int dy)
{
    if (key == MODULE_KEY_PRESS_SET || dx) { P.polar_view = dx ? (dx > 0) : !P.polar_view; persist_touch(); }
}

/* ------------------------------------------------------------------ calendario del cielo */

#define SKY_NIGHTS 7
#define SKY_SAMPLES 97                    /* dalle 16:00 alle 08:00, ogni 10 minuti */
#define SKY_T0 960                        /* minuti locali del primo campione */
enum { SK_DAY, SK_CIVIL, SK_NAUT, SK_ASTRO, SK_NIGHT };   /* Sole: giorno, crepuscoli, notte */
#define SK_SUN  0x07
#define SK_MOON 0x08                      /* Luna sopra l'orizzonte */
#define SK_GC   0x10                      /* centro della Via Lattea a piu' di 10 gradi */
struct night
{
    int day1970;
    int sunset, dusk, dawn, sunrise;      /* minuti locali, -1 se assente */
    int moonrise, moonset;                /* tra le 12 e le 12 del giorno dopo, -1 se assente */
    int dark_min, moonless_min;
    int moon_illum, moon_waxing;
    int gc_start, gc_end, gc_max_alt;     /* finestra del centro della Via Lattea, buio e senza Luna */
    int nelm_dark, nelm_moon, nelm_moon_max;   /* magnitudine limite x10: senza Luna, con la Luna (peggiore e migliore); -1 se mai */
    int score;                            /* qualita' della notte per la scelta della migliore */
    unsigned char s[SKY_SAMPLES];         /* stato del cielo ogni 10 minuti */
    unsigned char q[SKY_SAMPLES];         /* brillanza allo zenit col buio o con la Luna (SQM x10 - 100), 0 = non calcolata */
};
static struct night nights[SKY_NIGHTS];
static int mw_night, mw_back_sky, sky_keep;   /* "Dove puntare" aperta dal calendario (sky2.inc) */
static int sky_ready = 0, sky_progress = 0, sky_scroll = 0;
static int sky_sel = 0, sky_t0 = 0, sky_ready_t0 = 0, sky_best = 0;
static struct glider sky_moon_g = { -1, 0, 0, 0 };

/* centro galattico (Sgr A*), equinozio 2026.5 */
#define GC_RA  (17.789203f * 15.0f)
#define GC_DEC (-29.016482f)

/* ---- quanto la Luna schiarisce il cielo
 * Brillanza allo zenit col modello di Krisciunas & Schaefer (PASP 103, 1991):
 * luce lunare diffusa (Rayleigh + Mie) secondo fase e altezza della Luna, sommata
 * al cielo buio del luogo (opzione in mag/arcsec^2, come un SQM). Poi la
 * magnitudine limite a occhio nudo: NELM = 7.93 - 5 log10(10^(4.316 - SQM/5) + 1). */
#define SKY_EXT 0.20f                     /* estinzione in V (mag per massa d'aria), cielo limpido di pianura */

struct skymodel { float b0, istar, tz; };

/* cielo del luogo, entro valori sensati anche se la configurazione e' rovinata */
static int sky_sqm0() { return COERCE(opt_sqm10, 160, 230); }

static void sky_model(struct skymodel *m, int illum)
{
    float alpha = acosf(COERCE(2.0f * illum / 1000.0f - 1.0f, -1.0f, 1.0f)) * RAD2DEG;   /* angolo di fase, 0 = piena */
    float a2 = alpha * alpha;
    m->b0 = 34.08f * expf(20.7233f - 0.92104f * (sky_sqm0() / 10.0f));                     /* nanoLambert */
    m->istar = powf(10.0f, -0.4f * (3.84f + 0.026f * alpha + 4e-9f * a2 * a2));
    m->tz = 1.0f - powf(10.0f, -0.4f * SKY_EXT);                                          /* massa d'aria 1 allo zenit */
}

/* brillanza allo zenit in mag/arcsec^2 con la Luna all'altezza data (gradi) */
static float sky_sqm(const struct skymodel *m, float moon_alt)
{
    float b = m->b0;
    if (moon_alt > 0.0f)
    {
        float rho = 90.0f - moon_alt;                     /* distanza Luna-zenit = distanza zenitale della Luna */
        float sz = sinf(rho * DEG2RAD), cr = cosf(rho * DEG2RAD);
        float xm = 1.0f / sqrtf(1.0f - 0.96f * sz * sz);
        float fm = rho >= 10.0f ? powf(10.0f, 6.15f - rho / 40.0f) : 6.2e7f / (MAX(rho, 1.0f) * MAX(rho, 1.0f));
        float f = 229086.8f * (1.06f + cr * cr) + fm;     /* 10^5.36 (1.06 + cos^2) + Mie */
        b += f * m->istar * powf(10.0f, -0.4f * SKY_EXT * xm) * m->tz;
    }
    return (20.7233f - logf(b / 34.08f)) / 0.92104f;
}

static int sky_nelm10(float sqm)
{
    return (int)((7.93f - 5.0f * log10f(powf(10.0f, 4.316f - sqm / 5.0f) + 1.0f)) * 10.0f + 0.5f);
}

/* fuso orario di adesso in minuti (ora legale compresa) */
static int tz_now_min()
{
    int d70, mins, off;
    jt_now(&d70, &mins, &off);
    return off;
}

/* adesso: altezza e fase della Luna e stelle visibili allo zenit (per il registro della sessione) */
static float sky_now_nelm(float *moon_alt, int *illum)
{
    int d70, mins, off, wax;
    struct jtime j = jt_now(&d70, &mins, &off);
    float lst = lst_deg(j, lon_deg()), ra, dec;
    moon_pos(j, &ra, &dec, 0);
    float a = altitude(ra, dec, lst, lat_deg());
    a -= 0.95f * cosf(a * DEG2RAD);                       /* parallasse lunare */
    int ill = moon_illum(j, &wax);
    struct skymodel sm;
    sky_model(&sm, ill);
    if (moon_alt) *moon_alt = a;
    if (illum) *illum = ill;
    return (float)sky_nelm10(sky_sqm(&sm, a)) / 10.0f;
}

static void sky_compute_night(int n, int day1970)
{
    struct night *nt = &nights[n];
    memset(nt, 0, sizeof(*nt));
    nt->day1970 = day1970;
    nt->sunset = nt->dusk = nt->dawn = nt->sunrise = nt->gc_start = nt->gc_end = -1;
    nt->moonrise = nt->moonset = -1;
    float lat = lat_deg(), lon = lon_deg();
    int y = 0, m = 0, d = 0;
    {
        /* data civile per l'ora legale */
        int z = day1970 + 719468, era = (z >= 0 ? z : z - 146096) / 146097;
        int doe = z - era * 146097, yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        int doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
        d = doy - (153 * mp + 2) / 5 + 1; m = mp + (mp < 10 ? 3 : -9); y = yoe + era * 400 + (m <= 2);
    }
    /* fase della Luna a mezzanotte: serve per tutta la notte */
    {
        struct jtime jm = jt_from_local(day1970, 1440, utc_offset_min(y, m, d, 0));
        nt->moon_illum = moon_illum(jm, &nt->moon_waxing);
    }
    struct skymodel sm;
    sky_model(&sm, nt->moon_illum);
    nt->nelm_dark = nt->nelm_moon = nt->nelm_moon_max = -1;
    float prev_sun = 0.0f, prev_moon = 0.0f;
    for (int k = 0; k <= 144; k++)
    {
        int mins = 720 + k * 10;                 /* dalle 12:00 alle 12:00 del giorno dopo */
        int off = utc_offset_min(y, m, d, (mins / 60) % 24);
        struct jtime j = jt_from_local(day1970, mins, off);
        float lst = lst_deg(j, lon), ra, dec;
        sun_pos(j, &ra, &dec, 0);
        float sun = altitude(ra, dec, lst, lat);
        moon_pos(j, &ra, &dec, 0);
        float mgeo = altitude(ra, dec, lst, lat);
        float moon = mgeo - 0.95f * cosf(mgeo * DEG2RAD);   /* parallasse lunare */
        float gc = altitude(GC_RA, GC_DEC, lst, lat);
        int lm = mins % 1440;
        int moon_up = moon >= -0.833f;                      /* bordo superiore, con la rifrazione */
        if (k > 0)
        {
            /* istante del passaggio interpolato tra i due campioni (10 min) */
            #define CROSS(prev, cur, th) MOD(mins - 10 + (int)(10.0f * ((prev) - (th)) / ((prev) - (cur)) + 0.5f), 1440)
            if (prev_sun >= -0.833f && sun < -0.833f && nt->sunset < 0) nt->sunset = CROSS(prev_sun, sun, -0.833f);
            if (prev_sun >= -18.0f && sun < -18.0f && nt->dusk < 0) nt->dusk = CROSS(prev_sun, sun, -18.0f);
            if (prev_sun < -18.0f && sun >= -18.0f) nt->dawn = CROSS(prev_sun, sun, -18.0f);
            if (prev_sun < -0.833f && sun >= -0.833f) nt->sunrise = CROSS(prev_sun, sun, -0.833f);
            if (prev_moon < -0.833f && moon >= -0.833f && nt->moonrise < 0) nt->moonrise = CROSS(prev_moon, moon, -0.833f);
            if (prev_moon >= -0.833f && moon < -0.833f && nt->moonset < 0) nt->moonset = CROSS(prev_moon, moon, -0.833f);
            #undef CROSS
        }
        prev_sun = sun;
        prev_moon = moon;
        int sqm10 = 0;
        if (sun < -18.0f || moon > 0.0f)
        {
            float q = sky_sqm(&sm, moon);
            sqm10 = COERCE((int)(q * 10.0f + 0.5f), 101, 255 + 100);
            if (k < 144 && sun < -18.0f)
            {
                int ne = sky_nelm10(q);
                if (moon > 0.0f) { nt->nelm_moon = nt->nelm_moon < 0 ? ne : MIN(nt->nelm_moon, ne); nt->nelm_moon_max = MAX(nt->nelm_moon_max, ne); }
                else nt->nelm_dark = MAX(nt->nelm_dark, ne);
                nt->score += MAX(0, ne - 40);                /* decimi di magnitudine sopra la 4: ~21 per 10 minuti di cielo buio */
            }
        }
        if (k >= 24 && k <= 120)
        {
            int cls = sun > -0.833f ? SK_DAY : sun > -6.0f ? SK_CIVIL : sun > -12.0f ? SK_NAUT : sun > -18.0f ? SK_ASTRO : SK_NIGHT;
            nt->s[k - 24] = cls | (moon_up ? SK_MOON : 0) | (gc > 10.0f ? SK_GC : 0);
            nt->q[k - 24] = sqm10 ? sqm10 - 100 : 0;
        }
        if (k < 144 && sun < -18.0f)
        {
            nt->dark_min += 10;
            if (!moon_up)
            {
                nt->moonless_min += 10;
                if (gc > 10.0f)
                {
                    if (nt->gc_start < 0) nt->gc_start = lm;
                    nt->gc_end = (lm + 10) % 1440;
                    nt->gc_max_alt = MAX(nt->gc_max_alt, (int)gc);
                }
            }
        }
    }
}

static int gc_window(const struct night *n) { return n->gc_start < 0 ? 0 : MOD(n->gc_end - n->gc_start, 1440); }

/* ---- quando torna il centro della Via Lattea, se nelle 7 notti non c'e' */
static int gcn_state = 0;                 /* 0 non serve, 1 cerco, 2 trovato, 3 da qui non sale */
static int gcn_day, gcn_from, gcn_start, gcn_end;

static void civil_from_days(int d70, int *y, int *m, int *d)
{
    int z = d70 + 719468, era = (z >= 0 ? z : z - 146096) / 146097;
    int doe = z - era * 146097, yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
    *d = doy - (153 * mp + 2) / 5 + 1; *m = mp + (mp < 10 ? 3 : -9); *y = yoe + era * 400 + (*m <= 2);
}

/* centro galattico sopra 10 gradi col buio astronomico, nella notte che inizia il giorno d70:
   prima finestra, con il passo dato in minuti (la Luna qui non conta: e' la stagione) */
static int gc_dark_window(int d70, int step, int *start, int *end)
{
    int y, m, d;
    civil_from_days(d70, &y, &m, &d);
    float lat = lat_deg(), lon = lon_deg();
    int first = -1, last = -1;
    for (int mins = 960; mins <= 1920; mins += step)        /* dalle 16 alle 8: il buio sta qui */
    {
        struct jtime j = jt_from_local(d70, mins, utc_offset_min(y, m, d, (mins / 60) % 24));
        float lst = lst_deg(j, lon), ra, dec;
        int ok = 0;
        sun_pos(j, &ra, &dec, 0);
        if (altitude(ra, dec, lst, lat) < -18.0f) ok = altitude(GC_RA, GC_DEC, lst, lat) > 10.0f;
        if (ok) { if (first < 0) first = mins; last = mins; }
        else if (first >= 0) break;
    }
    if (first < 0) return 0;
    if (start) *start = first % 1440;
    if (end) *end = (last + step) % 1440;
    return 1;
}

/* un pezzo di ricerca per giro: salti di 3 giorni, poi il primo giorno esatto.
   "Torna" = prima notte con almeno mezz'ora utile (la prima in assoluto dura pochi minuti) */
#define GC_MIN_WINDOW 30
static int gc_good_night(int d70, int *start, int *end)
{
    int a, b;
    if (!gc_dark_window(d70, 10, &a, &b) || MOD(b - a, 1440) < GC_MIN_WINDOW) return 0;
    if (start) *start = a;
    if (end) *end = b;
    return 1;
}

static void gc_scan_step()
{
    for (int i = 0; i < 2; i++)                    /* sulla 6D ogni notte provata costa ~15 ms: pochi per giro */
    {
        if (gcn_day > gcn_from + 370) { gcn_state = 3; return; }
        if (gc_good_night(gcn_day, 0, 0))
        {
            int d = gcn_day;
            for (int b = 0; b < 2 && d - 1 >= gcn_from && gc_good_night(d - 1, 0, 0); b++) d--;
            gc_good_night(d, &gcn_start, &gcn_end);
            gcn_day = d;
            gcn_state = 2;
            return;
        }
        gcn_day += 3;
    }
}

/* calcolo a pezzi: una notte per giro, cosi' l'interfaccia resta fluida */
static int sky_tick()
{
    if (sky_ready)
    {
        if (gcn_state != 1) return 0;
        gc_scan_step();
        return 1;
    }
    int d70, mins, off;
    jt_now(&d70, &mins, &off);
    if (mins < 720) d70 -= 1;   /* prima di mezzogiorno vale ancora la notte appena passata */
    sky_compute_night(sky_progress, d70 + sky_progress);
    sky_progress++;
    if (sky_progress >= SKY_NIGHTS)
    {
        /* notte migliore: piu' buio senza Luna, con un premio se c'e' il centro della Via Lattea */
        int best = 0, score = -1;
        for (int i = 0; i < SKY_NIGHTS; i++)
        {
            /* cielo buio pesato con le stelle visibili; il centro della Via Lattea vale il doppio;
               a pari merito vince la notte con meno Luna */
            int sc = nights[i].score + 4 * gc_window(&nights[i]) + nights[i].moonless_min / 10;
            if (sc > score) { score = sc; best = i; }
        }
        sky_best = best;
        /* il centro della Via Lattea manca in tutte le 7 notti: si cerca quando torna */
        int any = 0;
        for (int i = 0; i < SKY_NIGHTS; i++)
            for (int k = 0; k < SKY_SAMPLES; k++)
                if ((nights[i].s[k] & SK_GC) && (nights[i].s[k] & SK_SUN) == SK_NIGHT) any = 1;
        gcn_state = any ? 0 : 1;
        gcn_from = gcn_day = nights[SKY_NIGHTS - 1].day1970 + 1;
        sky_ready = 1;
        sky_ready_t0 = sky_t0 = get_ms_clock();
    }
    return 1;
}

static void sky_enter()
{
    /* di ritorno da "Dove puntare": il calendario c'e' gia', si resta sulla notte scelta */
    if (sky_keep && sky_ready) { sky_keep = 0; sky_t0 = get_ms_clock(); return; }
    sky_keep = 0;
    sky_ready = 0; sky_progress = 0; sky_scroll = 0; sky_sel = 0; sky_moon_g.vis = -1; gcn_state = 0;
}

static void sky_key(int key, int dx, int dy)
{
    if (!sky_ready) return;
    if (key == MODULE_KEY_PRESS_SET) { mw_night = sky_sel; mw_back_sky = 1; go(SCR_MWPLAN, 1); return; }
    if (!dy) return;
    int s = COERCE(sky_sel + dy, 0, SKY_NIGHTS - 1);
    if (s != sky_sel) { sky_sel = s; sky_t0 = get_ms_clock(); }
}

static const char *moon_phase_name(int illum, int waxing)
{
    if (illum < 30) return "Luna nuova";
    if (illum > 970) return "Luna piena";
    if (illum >= 470 && illum <= 530) return waxing ? "Primo quarto" : "Ultimo quarto";
    if (illum < 500) return waxing ? "Falce crescente" : "Falce calante";
    return waxing ? "Gibbosa crescente" : "Gibbosa calante";
}

static void draw_moon_icon(int cx, int cy, int r, int illum, int waxing)
{
    /* su sfondo nero il bordo sfuma; nelle righe (sfondo che cambia) resta netto */
    draw_moon(cx, cy, r, illum, waxing, r >= 12 ? 0 : -1);
}

/* colore del cielo per lo stato del Sole */
static int sky_color(int cls)
{
    static const unsigned char c[5] = { COLOR_LIGHT_BLUE, COLOR_BLUE, COLOR_GRAY(24), COLOR_GRAY(12), COLOR_BLACK };
    if (night) return cls <= SK_CIVIL ? COLOR_DARK_RED : COLOR_BLACK;
    return c[COERCE(cls, 0, 4)];
}

static void fmt_mag(char *buf, int len, int m10) { snprintf(buf, len, "%d,%d", m10 / 10, m10 % 10); }

/* sorgere e tramonto della Luna dentro la striscia (16 - 8), in ordine di tempo:
   frase intera (long) o breve, da mettere tra parentesi */
static void moon_text(const struct night *t, char *buf, int len, int brief)
{
    char a[8], b[8];
    int pr = t->moonrise >= 0 ? MOD(t->moonrise - SKY_T0, 1440) : 9999;
    int ps = t->moonset >= 0 ? MOD(t->moonset - SKY_T0, 1440) : 9999;
    int in_r = pr <= 960, in_s = ps <= 960;
    int any = 0;
    for (int i = 0; i < SKY_SAMPLES; i++) any |= t->s[i] & SK_MOON;
    if (in_r) fmt_hm(a, sizeof(a), t->moonrise);
    if (in_s) fmt_hm(b, sizeof(b), t->moonset);
    if (in_r && in_s)
    {
        if (pr < ps) snprintf(buf, len, brief ? "sorge %s, tramonta %s" : "Luna: sorge %s, tramonta %s", a, b);
        else snprintf(buf, len, brief ? "tramonta %s, risorge %s" : "Luna: tramonta %s, risorge %s", b, a);
    }
    else if (in_r) snprintf(buf, len, brief ? "sorge alle %s" : "Luna: sorge alle %s, poi resta fino al mattino", a);
    else if (in_s) snprintf(buf, len, brief ? "fino alle %s" : "Luna: in cielo fino alle %s", b);
    else snprintf(buf, len, any ? (brief ? "tutta la notte" : "Luna: in cielo tutta la notte") : (brief ? "assente" : "Luna: sotto l'orizzonte tutta la notte"));
}

/* fin dove si vedono le stelle a occhio nudo: col cielo buio e con la Luna */
static void stars_line(const struct night *t, char *buf, int len)
{
    char a[8], b[8], tm[40];
    if (t->nelm_dark < 0 && t->nelm_moon < 0) { moon_text(t, buf, len, 0); return; }     /* niente buio astronomico */
    if (t->nelm_moon < 0)
    {
        fmt_mag(a, sizeof(a), t->nelm_dark);
        snprintf(buf, len, "Senza Luna col buio: stelle fino a mag %s", a);
        return;
    }
    if (t->nelm_dark < 0)
    {
        fmt_mag(a, sizeof(a), t->nelm_moon); fmt_mag(b, sizeof(b), t->nelm_moon_max);
        if (t->nelm_moon_max - t->nelm_moon >= 3) snprintf(buf, len, "Luna tutta la notte: stelle fino a mag %s - %s", a, b);
        else snprintf(buf, len, "Luna tutta la notte: stelle fino a mag %s", a);
        return;
    }
    fmt_mag(a, sizeof(a), t->nelm_dark); fmt_mag(b, sizeof(b), t->nelm_moon);
    moon_text(t, tm, sizeof(tm), 1);
    snprintf(buf, len, "Stelle fino a mag %s; con la Luna %s (%s)", a, b, tm);
    if (txt_w(FONT_MED, buf) > SCR_W - 24 - 124) snprintf(buf, len, "Stelle fino a mag %s; con la Luna %s", a, b);
}

/* la riga del centro della Via Lattea quando nella notte scelta non c'e' */
static void gc_missing_line(int gc_dark, char *buf, int len)
{
    char a[8], b[8];
    int y, m, d;
    if (gc_dark) { snprintf(buf, len, "Centro Via Lattea: col buio c'e' la Luna"); return; }
    switch (gcn_state)
    {
        case 1: snprintf(buf, len, "Centro Via Lattea: cerco quando torna..."); return;
        case 3: snprintf(buf, len, "Centro Via Lattea: da qui mai alto col buio"); return;
        case 2:
            civil_from_days(gcn_day, &y, &m, &d);
            fmt_hm(a, sizeof(a), gcn_start); fmt_hm(b, sizeof(b), gcn_end);
            snprintf(buf, len, "Centro Via Lattea: torna dal %d/%d, ore %s-%s %s", d, m, a, b,
                gcn_start < 720 ? "prima dell'alba" : "dopo il tramonto");
            if (txt_w(FONT_MED, buf) > SCR_W - 24 - 124) snprintf(buf, len, "Centro Via Lattea: torna dal %d/%d, ore %s-%s", d, m, a, b);
            return;
    }
    snprintf(buf, len, "Centro Via Lattea: non visibile col buio");
}

static void fmt_day(char *buf, int len, int day1970)
{
    static const char *const wd[] = { "dom", "lun", "mar", "mer", "gio", "ven", "sab" };
    int z = day1970 + 719468, era = (z >= 0 ? z : z - 146096) / 146097;
    int doe = z - era * 146097, yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
    int dd = doy - (153 * mp + 2) / 5 + 1, mm = mp + (mp < 10 ? 3 : -9);
    snprintf(buf, len, "%s %d/%d", wd[weekday(day1970)], dd, mm);
}

/* striscia della notte dalle 16 alle 8: cielo (giorno -> crepuscoli -> notte), Luna in alto,
   Via Lattea in basso. reveal: fin dove disegnare (entrata da sinistra) */
/* quanto la Luna schiarisce il cielo in un campione: decimi di magnitudine (0 se non c'e' o non e' buio) */
static int sky_lift(const struct night *n, int i)
{
    if ((n->s[i] & SK_SUN) != SK_NIGHT || !n->q[i]) return 0;
    return MAX(0, sky_sqm0() - (n->q[i] + 100));
}

/* un colore per campione, "piu' scuro = meglio": giorno e crepuscoli come prima,
   buio vero nero, buio con la Luna grigio tanto piu' chiaro quanto piu' la Luna illumina */
static int sky_px(const struct night *n, int i)
{
    int cls = n->s[i] & SK_SUN, d = sky_lift(n, i);
    if (cls != SK_NIGHT) return sky_color(cls);
    if (d <= 2) return COLOR_BLACK;
    if (night) return COLOR_DARK_RED;
    return COLOR_GRAY(MIN(50, 12 + d * 3 / 2));
}

/* la striscia della notte: colore del cielo, stelle (meno con la Luna), centro della Via Lattea in verde.
   big: puntini piu' fitti e scritte dentro le zone (sera, buio, Luna, alba) */
#define SKYBAR_W 640
#define SKYBAR_H 40
static unsigned char skybar_px[SKYBAR_W * SKYBAR_H];

static void draw_sky_bar(const struct night *n, int x0, int y, int w, int h, int reveal, int big)
{
    int base = COERCE((sky_sqm0() - 180) / 10, 1, 4);      /* stelle col cielo del luogo, senza Luna */
    /* la barra si compone in memoria e va sullo schermo una riga alla volta: prima erano un
       riempimento per campione e uno per stellina, centinaia per riga del Calendario */
    w = MIN(w, SKYBAR_W); h = MIN(h, SKYBAR_H);
    int vis = COERCE(reveal - x0, 0, w);
    if (vis > 0)
    {
        unsigned char *pb = skybar_px;
        for (int i = 0; i < SKY_SAMPLES - 1; i++)
        {
            int xa = w * i / (SKY_SAMPLES - 1), xb = w * (i + 1) / (SKY_SAMPLES - 1);
            if (xa >= vis) break;
            xb = MIN(xb, vis);
            int v = n->s[i];
            unsigned char c0 = (unsigned char)sky_px(n, i);
            for (int r = 0; r < h; r++) memset(pb + r * SKYBAR_W + xa, c0, xb - xa);
            if ((v & SK_SUN) == SK_NIGHT)
            {
                int dots = COERCE(base - sky_lift(n, i) / 5, 0, 4);   /* la Luna spegne le stelle */
                if (!big) dots = (i % 3 == 0 && dots >= 2);
                for (int k = 0; k < dots; k++)
                {
                    unsigned hsh = (unsigned)(i * 2654435761u + k * 40503u);
                    int band = big ? (h - 4) / 3 : h - 4;            /* nella grande lascia libero il centro per le scritte */
                    int yy = (hsh >> 8) % MAX(1, band);
                    if (big && (hsh & 1)) yy += h - 4 - band;
                    int c = night ? ((hsh >> 4) & 1 ? COLOR_RED : COLOR_DARK_RED) : ((hsh >> 4) & 3 ? COLOR_GRAY(70) : COLOR_WHITE);
                    int dx = xa + (int)((hsh >> 16) % MAX(1, xb - xa)), dy = 2 + yy;
                    if (dx < vis && dy < h) pb[dy * SKYBAR_W + dx] = (unsigned char)c;
                }
            }
            if (!big && (v & SK_GC) && (v & SK_SUN) == SK_NIGHT && !(v & SK_MOON))
                for (int r = MAX(0, h - 3); r < h; r++) memset(pb + r * SKYBAR_W + xa, (unsigned char)C_OK, xb - xa);
        }
        /* estremita' arrotondate: i pixel fuori dagli angoli restano trasparenti */
        int rr = MIN(h / 2, big ? 6 : 4);
        for (int i = 0; i < rr; i++)
        {
            int ins = rc_inset(rr, i);
            for (int k = 0; k < ins; k++)
            {
                pb[i * SKYBAR_W + k] = pb[(h - 1 - i) * SKYBAR_W + k] = 0;
                if (w - 1 - k < vis) pb[i * SKYBAR_W + w - 1 - k] = pb[(h - 1 - i) * SKYBAR_W + w - 1 - k] = 0;
            }
        }
        for (int r = 0; r < h; r++) put_row(x0, y + r, pb + r * SKYBAR_W, vis);
    }
    ring_r(C_LINE, x0 - 1, y - 1, w + 2, h + 2, MIN((h + 2) / 2, big ? 7 : 5), 1);
    if (!big || reveal < x0 + w) return;

    /* scritte dentro le zone, solo se c'e' spazio */
    int ty = y + (h - 12) / 2;
    for (int a = 0; a < SKY_SAMPLES - 1; )
    {
        int cls = n->s[a] & SK_SUN;
        int kind = cls == SK_NIGHT ? (sky_lift(n, a) <= 2 ? 2 : 3) : cls == SK_DAY ? 0 : 1;
        int b = a + 1;
        while (b < SKY_SAMPLES - 1)
        {
            int c2 = n->s[b] & SK_SUN;
            int k2 = c2 == SK_NIGHT ? (sky_lift(n, b) <= 2 ? 2 : 3) : c2 == SK_DAY ? 0 : 1;
            if (k2 != kind) break;
            b++;
        }
        int xa = x0 + w * a / (SKY_SAMPLES - 1), xb = x0 + w * b / (SKY_SAMPLES - 1), ww = xb - xa, cx = (xa + xb) / 2;
        char lab[24];
        lab[0] = 0;
        if (kind == 1) snprintf(lab, sizeof(lab), "%s", a < SKY_SAMPLES / 2 ? "sera" : "alba");
        if (kind == 2) snprintf(lab, sizeof(lab), "%s", ww >= 110 ? "cielo buio" : "buio");
        if (kind == 3)
        {
            /* Luna: la sua fase disegnata e la scritta */
            int iw = 18;
            snprintf(lab, sizeof(lab), ww >= 120 ? "Luna %d%%" : "Luna", (n->moon_illum + 5) / 10);
            int tw = txt_w(FONT_SMALL, lab);
            if (ww >= iw + tw + 12)
            {
                int x = cx - (iw + tw) / 2;
                draw_moon(x + 7, y + h / 2, 7, n->moon_illum, n->moon_waxing, -1);
                txt(FONT_SMALL, C_TEXT, x + iw, ty, lab);
            }
            else if (ww >= 18) draw_moon(cx, y + h / 2, 7, n->moon_illum, n->moon_waxing, -1);
            a = b;
            continue;
        }
        if (lab[0] && ww >= txt_w(FONT_SMALL, lab) + 10)
            txt_c(FONT_SMALL, kind == 2 ? C_DIM : C_TEXT, cx, ty, lab);
        a = b;
    }
}

static void draw_sky()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Calendario del cielo");
    int now = get_ms_clock();
    if (!sky_ready)
    {
        draw_moon_icon(360, 154, 40, sky_progress * 1000 / SKY_NIGHTS, 1);
        txt_c(FONT_MED, C_DIM, 360, 214, "Calcolo di Sole, Luna e Via Lattea...");
        draw_progress(160, 252, 400, 6, sky_progress * 1000 / SKY_NIGHTS);
        draw_footer("MENU indietro");
        anim_keep();
        return;
    }
    char buf[96], a[8], b[8];
    const struct night *t = &nights[sky_sel];
    int e0 = phase(now - sky_t0, 0, anim_ms(260), EZ_OUT), e1 = phase(now - sky_t0, anim_ms(60), anim_ms(260), EZ_OUT);
    int e2 = phase(now - sky_t0, anim_ms(120), anim_ms(260), EZ_OUT), e3 = phase(now - sky_t0, anim_ms(180), anim_ms(260), EZ_OUT);
    if (e3 < 1000) anim_keep();

    /* ---- la notte scelta */
    draw_moon_icon(66, 106, 44, glide(&sky_moon_g, t->moon_illum, 420), t->moon_waxing);
    static const char *const when[3] = { "STANOTTE", "DOMANI", "DOPODOMANI" };
    if (sky_sel < 3) snprintf(buf, sizeof(buf), "%s", when[sky_sel]); else snprintf(buf, sizeof(buf), "TRA %d NOTTI", sky_sel);
    txt_in(FONT_SMALL, C_ACCENT, 124, 50, buf, e0, 0, 6);
    if (sky_sel == sky_best)
    {
        star(560, 57, 3, C_ACCENT);
        txt_in(FONT_SMALL, C_ACCENT, 572, 50, "NOTTE MIGLIORE", e0, 0, 6);
    }
    char day[16];
    fmt_day(day, sizeof(day), t->day1970);
    snprintf(buf, sizeof(buf), "%s   %s %d%%", day, moon_phase_name(t->moon_illum, t->moon_waxing), (t->moon_illum + 5) / 10);
    txt_in(FONT_MED_LARGE, C_TEXT, 124, 60, buf, e0, 0, 8);
    if (t->dusk >= 0 && t->dawn >= 0)
    {
        fmt_hm(a, sizeof(a), t->dusk); fmt_hm(b, sizeof(b), t->dawn);
        snprintf(buf, sizeof(buf), "Buio %s - %s   senza Luna %dh%02d", a, b, t->moonless_min / 60, t->moonless_min % 60);
    }
    else snprintf(buf, sizeof(buf), "Il cielo non diventa del tutto buio");
    txt_in(FONT_MED, C_TEXT, 124, 89, buf, e1, 0, 8);
    int gc_dark = 0;
    for (int i = 0; i < SKY_SAMPLES; i++)
        if ((t->s[i] & SK_GC) && (t->s[i] & SK_SUN) == SK_NIGHT) gc_dark = 1;
    stars_line(t, buf, sizeof(buf));
    txt_in(FONT_MED, C_DIM, 124, 111, buf, e2, 0, 8);
    if (t->gc_start >= 0)
    {
        fmt_hm(a, sizeof(a), t->gc_start); fmt_hm(b, sizeof(b), t->gc_end);
        snprintf(buf, sizeof(buf), "Centro Via Lattea %s - %s, alto fino a %d gradi", a, b, t->gc_max_alt);
        txt_in(FONT_MED, C_OK, 124, 133, buf, e3, 0, 8);
    }
    else
    {
        gc_missing_line(gc_dark, buf, sizeof(buf));
        txt_in(FONT_MED, C_DIM, 124, 133, buf, e3, 0, 8);
    }

    /* ---- linea del tempo della notte scelta */
    const int lx = 104, lw = 592, sy = 164, sh = 34, gy = sy + sh + 4;
    int rev = lx + lw * phase(now - sky_t0, 0, anim_ms(520), EZ_IO) / 1000;
    if (rev < lx + lw) anim_keep();
    txt(FONT_SMALL, C_DIM, 24, sy + (sh - 12) / 2, "Cielo");
    draw_sky_bar(t, lx, sy, lw, sh, rev, 1);
    /* centro della Via Lattea: verde quando si fotografa (buio e senza Luna), grigio se e' alto ma c'e' luce */
    int gca = -1;
    for (int i = 0; i < SKY_SAMPLES - 1; i++)
    {
        int xa = lx + lw * i / (SKY_SAMPLES - 1), xb = lx + lw * (i + 1) / (SKY_SAMPLES - 1);
        if (xa >= rev) break;
        xb = MIN(xb, rev);
        int v = t->s[i];
        if (!(v & SK_GC)) continue;
        int ok = (v & SK_SUN) == SK_NIGHT && !(v & SK_MOON);
        if (ok) fill(C_OK, xa, gy, xb - xa, 6);                      /* da fotografare: barra piena */
        else fill(night ? COLOR_DARK_RED : COLOR_GRAY(30), xa, gy + 2, xb - xa, 2);   /* alto ma c'e' luce: solo una linea */
        if (gca < 0) gca = i;
    }
    txt(FONT_SMALL, gca >= 0 ? C_DIM : C_LINE, 24, gy - 3, "Via Lattea");
    for (int h = 0; h <= 16; h++)                          /* ore: 16 ... 8 */
    {
        int x = lx + lw * h / 16;
        fill(C_LINE, x, gy + 9, 1, h % 2 ? 2 : 4);
        if (h % 2 == 0)
        {
            snprintf(buf, sizeof(buf), "%d", (16 + h) % 24);
            txt_c(FONT_SMALL, C_DIM, x, gy + 14, buf);
        }
    }
    if (sky_sel == 0)
    {
        /* adesso */
        int d70, mins, off;
        jt_now(&d70, &mins, &off);
        int since = mins >= 720 ? mins - SKY_T0 : mins + 1440 - SKY_T0;
        if (since >= 0 && since <= 960)
        {
            int x = lx + lw * since / 960;
            fill(C_ACCENT, x - 1, sy - 4, 3, gy + 8 - (sy - 4));
            fill_tri(x - 4, sy - 8, x + 4, sy - 8, x, sy - 3, C_ACCENT);
        }
    }

    /* ---- le 7 notti */
    const int ty = 246, y0 = 262, rh = 26, mx = 214, mw = 280;
    txt(FONT_SMALL, C_DIM, 32, ty, "NOTTE");
    txt(FONT_SMALL, C_DIM, 140, ty, "LUNA");
    txt(FONT_SMALL, C_DIM, mx, ty, "16");
    txt_c(FONT_SMALL, C_DIM, mx + mw / 2, ty, "0");
    txt_r(FONT_SMALL, C_DIM, mx + mw, ty, "8");
    txt(FONT_SMALL, C_DIM, 512, ty, "SENZA LUNA");
    txt(FONT_SMALL, C_DIM, 610, ty, "VIA LATTEA");
    for (int i = 0; i < SKY_NIGHTS; i++) if (i % 2 == 0) fill_r(C_PANEL, 24, y0 + i * rh, SCR_W - 48, rh - 2, R_CHIP);
    hl_bar(24, SCR_W - 48, y0 + sky_sel * rh, rh - 2);
    for (int i = 0; i < SKY_NIGHTS; i++)
    {
        const struct night *n = &nights[i];
        int ry = y0 + i * rh;
        int er = phase(now - sky_ready_t0, anim_ms(i * 45), anim_ms(300), EZ_OUT);
        if (er < 1000) anim_keep();
        if (er <= 0) continue;
        int mine = i == sky_sel;
        fmt_day(buf, sizeof(buf), n->day1970);
        txt_in(FONT_MED, mine ? C_ACCENT : C_TEXT, 34, ry, buf, er, 24, 0);
        draw_moon_icon(146, ry + 11, 8, n->moon_illum, n->moon_waxing);
        snprintf(buf, sizeof(buf), "%d%%", (n->moon_illum + 5) / 10);
        txt_in(FONT_MED, mine ? C_TEXT : C_DIM, 160, ry, buf, er, 24, 0);
        draw_sky_bar(n, mx, ry + 5, mw, 12, mx + mw * er / 1000, 0);
        snprintf(buf, sizeof(buf), "%dh%02d", n->moonless_min / 60, n->moonless_min % 60);
        txt_in(FONT_MED, C_TEXT, 512, ry, buf, er, 24, 0);
        if (i == sky_best && er >= 1000) star(512 + txt_w(FONT_MED, buf) + 10, ry + 11, 3, C_ACCENT);
        if (n->gc_start >= 0)
        {
            /* senza zeri iniziali, cosi' sta nella colonna */
            snprintf(buf, sizeof(buf), "%d:%02d-%d:%02d", n->gc_start / 60, n->gc_start % 60, n->gc_end / 60, n->gc_end % 60);
            txt_in(FONT_MED, C_OK, 590, ry, buf, er, COERCE(SCR_W - 6 - 590 - txt_w(FONT_MED, buf), 0, 24), 0);   /* entra senza uscire dal bordo */
        }
        else txt_in(FONT_MED, mine ? C_TEXT : C_DIM, 610, ry, "-", er, 24, 0);
    }
    draw_footer("^/v scegli la notte   SET dove puntare   MENU indietro");
}

/* ------------------------------------------------------------------ menu Astro */

static const struct menu_row astro_rows[] = {
    { "Via Lattea (regola NPF)", "Tempo massimo, ISO e bianco dall'obiettivo", SCR_NPF },
    { "Intervallometro", "Via Lattea, Deep sky, Star trails, Giorno-notte, Dark...", SCR_INTERVAL },
    { "Fuoco sulle stelle", "Misura HFR in Live View, zoom e passi di fuoco", SCR_STARS },
    { "Allineamento polare", "Dove mettere la Polare nel cannocchiale", SCR_POLAR },
    { "Calendario del cielo", "Luna, buio e Via Lattea nelle prossime 7 notti", SCR_SKY },
    { "Dove puntare la Via Lattea", "Direzione, altezza e arco del centro, ora per ora", SCR_MWPLAN },
    { "Luna da fotografare", "Fase, posizione e tempo di scatto giusto", SCR_MOON },
    { "Etichetta file", "LGHT, DARK, FLAT, BIAS al posto di IMG_", -1 },
};
#define NUM_ASTRO_ROWS ((int)COUNT(astro_rows))
#define ASTRO_TAG_ROW (NUM_ASTRO_ROWS - 1)
