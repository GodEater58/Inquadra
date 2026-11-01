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
    int t10 = npf_tenths(P.npf_focal, P.npf_ap10);
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

static const char *const npf_labels[] = { "Focale", "Diaframma", "Applica (ghiera su M)" };

static void npf_value(int row, char *buf, int len, int *kind)
{
    char a[12];
    if (row == 0) snprintf(buf, len, "< %d mm >", P.npf_focal);
    else if (row == 1) { fmt_tenths(a, sizeof(a), P.npf_ap10); snprintf(buf, len, "< f/%s >", a); }
    else *kind = VK_ACTION;
}

static void npf_change(int row, int dir, int big)
{
    if (row == 0) P.npf_focal = COERCE(P.npf_focal + dir * (big ? 5 : 1), 8, 800);
    else if (row == 1) P.npf_ap10 = step_in(ap_steps, COUNT(ap_steps), P.npf_ap10, dir);
    persist_touch();
}

static void npf_activate(int row) { npf_apply(); }

static void npf_extra()
{
    char buf[96], a[12], b[12];
    int t10 = npf_tenths(P.npf_focal, P.npf_ap10);
    int r500 = P.npf_focal > 0 ? 5000 / P.npf_focal : 0;
    int y = 196;
    fill(C_PANEL, 24, y, SCR_W - 48, 120);
    txt(FONT_MED, C_DIM, 44, y + 10, "Tempo massimo (regola NPF)");
    fmt_tenths(a, sizeof(a), t10);
    snprintf(buf, sizeof(buf), "%s s", a);
    txt(FONT_LARGE, C_ACCENT, 44, y + 44, buf);
    int x2 = 380;
    fmt_tenths(b, sizeof(b), r500);
    snprintf(buf, sizeof(buf), "Regola del 500: %s s", b);
    txt(FONT_MED, C_DIM, x2, y + 14, buf);
    snprintf(buf, sizeof(buf), "ISO %d   bianco 3900 K", P.npf_ap10 <= 20 ? 3200 : 6400);
    txt(FONT_MED, C_TEXT, x2, y + 48, buf);
    txt(FONT_MED, C_DIM, x2, y + 82, "RAW, stabilizzatore spento");
    txt(FONT_MED, C_DIM, 30, 330, lens_info.name[0] ? lens_info.name : "Obiettivo manuale: imposta focale e diaframma");
    txt(FONT_MED, t10 > 300 ? C_WARN : C_DIM, 30, 362,
        t10 > 300 ? "Oltre 30 s: imposto 30 s, per di piu' usa Deep sky (posa B)" : "t = (35 x N + 30 x p) / f, con p = 6,54 um");
}

static const struct form npf_form = { 3, npf_labels, npf_value, npf_change, npf_activate, npf_extra,
    "^/v riga   </> cambia   SET applica   MENU indietro", 58, 44 };

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

static const char *const int_labels[] = { "Preset", "Scatti", "Posa", "Pausa", "Ritardo iniziale", "Schermo", "Etichetta file", "Avvia" };

static void int_value(int row, char *buf, int len, int *kind)
{
    const struct preset *ps = &presets[COERCE(P.int_preset, 0, NUM_PRESETS - 1)];
    switch (row)
    {
        case 0: snprintf(buf, len, "< %s >", ps->name); break;
        case 1: snprintf(buf, len, "%d", P.int_count); break;
        case 2:
            if (P.int_bulb > 0) snprintf(buf, len, "%d s (posa B)", P.int_bulb);
            else if (P.int_preset == PRESET_BIAS) snprintf(buf, len, "1/4000");
            else snprintf(buf, len, "come la fotocamera");
            break;
        case 3: snprintf(buf, len, "%d s", P.int_pause); break;
        case 4: snprintf(buf, len, "%d s", delay_steps[COERCE(P.int_delay_idx, 0, (int)COUNT(delay_steps) - 1)]); break;
        case 5: *kind = P.int_screen_off ? VK_OFF : VK_ON; break;
        case 6: snprintf(buf, len, "%s", tag_names[ps->tag]); *kind = VK_INFO; break;
        case 7: *kind = VK_ACTION; snprintf(buf, len, "SET  >"); break;
    }
}

static void int_change(int row, int dir, int big)
{
    switch (row)
    {
        case 0: preset_select(MOD(P.int_preset + dir, NUM_PRESETS)); break;
        case 1: P.int_count = COERCE(P.int_count + dir * (big ? 10 : 1), 1, 9999); break;
        case 2: P.int_bulb = step_in(bulb_steps, COUNT(bulb_steps), P.int_bulb, dir); break;
        case 3: P.int_pause = COERCE(P.int_pause + dir * (big ? 5 : 1), 0, 3600); break;
        case 4: P.int_delay_idx = COERCE(P.int_delay_idx + dir, 0, (int)COUNT(delay_steps) - 1); break;
        case 5: P.int_screen_off = !P.int_screen_off; break;
    }
    persist_touch();
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
    return 1;
}

static void int_activate(int row)
{
    if (row == 0) { preset_select(MOD(P.int_preset + 1, NUM_PRESETS)); return; }
    if (row == 5) { P.int_screen_off = !P.int_screen_off; persist_touch(); return; }
    if (!interval_check()) return;
    job_request = JOB_SEQ;
    gui_stop_menu();
}

static void int_extra()
{
    const struct preset *ps = &presets[COERCE(P.int_preset, 0, NUM_PRESETS - 1)];
    txt(FONT_MED, C_DIM, 30, 380, ps->note1);
    txt(FONT_MED, C_DIM, 30, 406, ps->note2);
}

static const struct form int_form = { 8, int_labels, int_value, int_change, int_activate, int_extra,
    "^/v riga   </> cambia   SET avvia   MENU indietro", 52, 40 };

/* ------------------------------------------------------------------ fuoco sulle stelle (HFR) */

#define HFR_HIST 96
static int hfr_hist[HFR_HIST];
static int hfr_count = 0, hfr_best = 0, hfr_last = -1, hfr_peak = 0;
static int star_x = -1, star_y = -1, star_r = 0;
static int focus_step = 2;

static int luma(const uint8_t *buf, int pitch, int x, int y) { return buf[y * pitch + x * 2 + 1]; }

/* stella piu' luminosa, fondo cielo dalla mediana dell'anello, soglia di rumore dal MAD,
 * centroide pesato, HFR = raggio medio pesato sulla luce (come StarMeter di Iride) */
static int hfr_measure(int *hfr100, int *peak_out, int *cx_out, int *cy_out, int *r_out)
{
    struct vram_info *v = get_yuv422_vram();
    if (!v || !v->vram) return 0;
    const uint8_t *b = v->vram;
    int W = v->width, H = v->height, Pt = v->pitch;
    if (W < 64 || H < 64 || Pt < W * 2) return 0;

    int r = MAX(8, W * 35 / 1000), ring = 5, m = r + ring + 2;
    int sx0 = MAX(m, W * 2 / 10), sx1 = MIN(W - m, W * 8 / 10);
    int sy0 = MAX(m, H * 2 / 10), sy1 = MIN(H - m, H * 8 / 10);

    int best = -1, bx = 0, by = 0;
    for (int y = sy0; y < sy1; y += 2)
        for (int x = sx0; x < sx1; x += 2)
        {
            int s = luma(b, Pt, x - 1, y - 1) + luma(b, Pt, x, y - 1) + luma(b, Pt, x + 1, y - 1)
                  + luma(b, Pt, x - 1, y)     + luma(b, Pt, x, y)     + luma(b, Pt, x + 1, y)
                  + luma(b, Pt, x - 1, y + 1) + luma(b, Pt, x, y + 1) + luma(b, Pt, x + 1, y + 1);
            if (s > best) { best = s; bx = x; by = y; }
        }
    if (best < 0) return 0;

    static int hist[256], dev[256];
    memset(hist, 0, sizeof(hist));
    int cnt = 0, ro = r + ring;
    for (int y = by - ro; y <= by + ro; y++)
    {
        if (y < 0 || y >= H) continue;
        for (int x = bx - ro; x <= bx + ro; x++)
        {
            if (x < 0 || x >= W) continue;
            int dx = x - bx, dy = y - by, d2 = dx * dx + dy * dy;
            if (d2 > r * r && d2 <= ro * ro) { hist[luma(b, Pt, x, y)]++; cnt++; }
        }
    }
    if (cnt < 8) return 0;
    int acc = 0, bg = 0;
    for (bg = 0; bg < 255; bg++) { acc += hist[bg]; if (acc * 2 >= cnt) break; }
    memset(dev, 0, sizeof(dev));
    for (int i = 0; i < 256; i++) if (hist[i]) dev[ABS(i - bg)] += hist[i];
    int mad = 0;
    for (acc = 0, mad = 0; mad < 255; mad++) { acc += dev[mad]; if (acc * 2 >= cnt) break; }
    int thr = bg + MAX(4, (mad * 3 * 3) / 2);

    float cx = (float)bx, cy = (float)by, rr = (float)(r * r);
    int peak = 0;
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
    int npx = 0;
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
    if (sum <= 0.0f || npx < 2) return 0;   /* un pixel caldo isolato non e' una stella */

    *hfr100 = (int)(sd / sum * 100.0f);
    *peak_out = peak;
    *cx_out = (int)cx;
    *cy_out = (int)cy;
    *r_out = r;
    return 1;
}

static void stars_reset() { hfr_count = 0; hfr_best = 0; hfr_last = -1; hfr_peak = 0; star_x = -1; }

static void stars_enter() { stars_reset(); }

static void stars_leave()
{
    if (lv && lv_dispsize != 1) set_lv_zoom(1);
}

/* misura ogni 250 ms: ritorna 1 se c'e' da ridisegnare */
static int stars_tick()
{
    if (!lv) { hfr_last = -1; star_x = -1; return 1; }
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

    /* cornice dell'area di ricerca */
    bmp_draw_rect(C_LINE, 144, 96, 432, 228);
    if (star_x >= 0 && star_y > 96 && star_y < 324)
    {
        draw_circle(star_x, star_y, star_r, C_ACCENT);
        draw_circle(star_x, star_y, star_r + 1, C_ACCENT);
        draw_line(star_x - star_r - 10, star_y, star_x - star_r - 3, star_y, C_ACCENT);
        draw_line(star_x + star_r + 3, star_y, star_x + star_r + 10, star_y, C_ACCENT);
    }

    fill(C_BG, 0, 336, SCR_W, 108);
    if (!lv)
    {
        txt(FONT_MED, C_WARN, 16, 346, "Attiva il Live View (tasto Start/Stop) e punta");
        txt(FONT_MED, C_DIM, 16, 376, "una stella luminosa al centro dell'inquadratura.");
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
        snprintf(buf, sizeof(buf), "migliore %d.%02d   picco %d%s", hfr_best / 100, hfr_best % 100, hfr_peak,
            hfr_peak >= 250 ? " satura" : "");
        txt(FONT_MED, hfr_peak >= 250 ? C_WARN : C_DIM, 16, 392, buf);
    }
    int gx = 380, gy = 344, gw = 320, gh = 88;
    bmp_draw_rect(C_LINE, gx - 2, gy - 2, gw + 4, gh + 4);
    if (hfr_count > 0)
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
    draw_footer("SET zoom   </> fuoco -/+   ^/v passo   INFO azzera   MENU");
}

static void stars_key(int key, int dx, int dy)
{
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
    else if (key == MODULE_KEY_INFO) stars_reset();
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

static int last_sunday(int y, int m)
{
    int dim = (m == 3 || m == 10) ? 31 : 30;
    int d = days_from_civil(y, m, dim);
    return dim - weekday(d);
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

static int utc_offset_min(int y, int m, int d, int local_hour)
{
    if (P.tz_mode > 0) return (P.tz_mode - 1) * 60;
    return eu_dst(y, m, d, local_hour - 1) ? 120 : 60;
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

static float lat_deg() { return (float)P.lat100 / 100.0f; }
static float lon_deg() { return (float)P.lon100 / 100.0f; }

/* angolo orario della Polare in ore (0..24) */
static float polar_ha_hours(struct jtime j)
{
    float ra, dec;
    polaris(j, &ra, &dec);
    return wrap360(lst_deg(j, lon_deg()) - ra) / 15.0f;
}

static int polar_tick() { return 1; }   /* ridisegna ogni secondo: la Polare si muove */

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

    int cx = 200, cy = 236, R = 150, rp = R * 70 / 100;
    draw_circle(cx, cy, R, C_LINE);
    draw_circle(cx, cy, R - 1, C_LINE);
    draw_circle(cx, cy, rp, C_LINE);
    for (int h = 0; h < 12; h++)
    {
        float a = (float)h * 30.0f * DEG2RAD;
        int x1 = cx + (int)((float)(R - 12) * sinf(a)), y1 = cy - (int)((float)(R - 12) * cosf(a));
        int x2 = cx + (int)((float)R * sinf(a)), y2 = cy - (int)((float)R * cosf(a));
        draw_line(x1, y1, x2, y2, C_DIM);
        char n[8];
        snprintf(n, sizeof(n), "%d", h == 0 ? 12 : h);
        txt_c(FONT_SMALL, C_DIM, cx + (int)((float)(R + 14) * sinf(a)), cy - (int)((float)(R + 14) * cosf(a)) - 6, n);
    }
    draw_line(cx - 8, cy, cx + 8, cy, C_DIM);
    draw_line(cx, cy - 8, cx, cy + 8, C_DIM);

    /* vista a occhio nudo: dal 12 la Polare gira in senso antiorario di HA x 15 gradi */
    float a = ha * 15.0f * DEG2RAD;
    int sx = inverted ? 1 : -1;
    int px = cx + sx * (int)((float)rp * sinf(a));
    int py = cy + (inverted ? 1 : -1) * (int)((float)rp * cosf(a));
    draw_line(cx, cy, px, py, C_ACCENT);
    fill_circle(px, py, 8, C_ACCENT);
    fill_circle(px, py, 3, C_BG);

    float clock = 12.0f - ha / 2.0f + (inverted ? 6.0f : 0.0f);
    clock = fmodf(clock + 24.0f, 12.0f);
    int ch = (int)clock, cm = (int)((clock - (float)ch) * 60.0f);
    char buf[64];
    int x2 = 390;
    txt(FONT_MED, C_DIM, x2, 64, "Posizione della Polare");
    snprintf(buf, sizeof(buf), "ore %d:%02d", ch == 0 ? 12 : ch, cm);
    txt(FONT_LARGE, C_ACCENT, x2, 92, buf);
    int hah = (int)ha, ham = (int)((ha - (float)hah) * 60.0f);
    snprintf(buf, sizeof(buf), "Angolo orario %02dh %02dm", hah, ham);
    txt(FONT_MED, C_TEXT, x2, 140, buf);
    snprintf(buf, sizeof(buf), "Distanza dal polo %d'", (int)((90.0f - dec) * 60.0f + 0.5f));
    txt(FONT_MED, C_TEXT, x2, 170, buf);
    float to_transit = (24.0f - ha) * 0.99727f;
    snprintf(buf, sizeof(buf), "Culmina tra %dh %02dm", (int)to_transit, (int)((to_transit - (float)(int)to_transit) * 60.0f));
    txt(FONT_MED, C_DIM, x2, 200, buf);
    txt(FONT_MED, C_DIM, x2, 246, inverted ? "Vista: cannocchiale polare" : "Vista: a occhio nudo");
    txt(FONT_MED, C_DIM, x2, 272, inverted ? "(immagine capovolta)" : "(guardando a nord)");
    char la[16], lo[16];
    snprintf(la, sizeof(la), "%d.%02d", P.lat100 / 100, ABS(P.lat100 % 100));
    snprintf(lo, sizeof(lo), "%d.%02d", P.lon100 / 100, ABS(P.lon100 % 100));
    snprintf(buf, sizeof(buf), "Luogo %s N  %s E", la, lo);
    txt(FONT_MED, C_DIM, x2, 318, buf);
    snprintf(buf, sizeof(buf), "Fuso UTC+%d%s", off / 60, P.tz_mode ? "" : " (auto)");
    txt(FONT_MED, C_DIM, x2, 344, buf);
    draw_footer("INFO vista occhio/cannocchiale   MENU indietro   (luogo in Opzioni)");
}

static void polar_key(int key, int dx, int dy)
{
    if (key == MODULE_KEY_INFO || key == MODULE_KEY_PRESS_SET) { P.polar_view = !P.polar_view; persist_touch(); }
}

/* ------------------------------------------------------------------ calendario del cielo */

#define SKY_NIGHTS 7
struct night
{
    int day1970;
    int sunset, dusk, dawn, sunrise;      /* minuti locali, -1 se assente */
    int dark_min, moonless_min;
    int moon_illum, moon_waxing;
    int gc_start, gc_end, gc_max_alt;     /* finestra del centro della Via Lattea, buio e senza Luna */
};
static struct night nights[SKY_NIGHTS];
static int sky_ready = 0, sky_progress = 0, sky_scroll = 0;

/* centro galattico (Sgr A*), equinozio 2026.5 */
#define GC_RA  (17.789203f * 15.0f)
#define GC_DEC (-29.016482f)

static void sky_compute_night(int n, int day1970)
{
    struct night *nt = &nights[n];
    memset(nt, 0, sizeof(*nt));
    nt->day1970 = day1970;
    nt->sunset = nt->dusk = nt->dawn = nt->sunrise = nt->gc_start = nt->gc_end = -1;
    float lat = lat_deg(), lon = lon_deg();
    int y = 0, m = 0, d = 0;
    {
        /* data civile per l'ora legale */
        int z = day1970 + 719468, era = (z >= 0 ? z : z - 146096) / 146097;
        int doe = z - era * 146097, yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        int doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
        d = doy - (153 * mp + 2) / 5 + 1; m = mp + (mp < 10 ? 3 : -9); y = yoe + era * 400 + (m <= 2);
    }
    float prev_sun = 0.0f;
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
        if (k > 0)
        {
            /* istante del passaggio interpolato tra i due campioni (10 min) */
            #define CROSS(th) MOD(mins - 10 + (int)(10.0f * (prev_sun - (th)) / (prev_sun - sun) + 0.5f), 1440)
            if (prev_sun >= -0.833f && sun < -0.833f && nt->sunset < 0) nt->sunset = CROSS(-0.833f);
            if (prev_sun >= -18.0f && sun < -18.0f && nt->dusk < 0) nt->dusk = CROSS(-18.0f);
            if (prev_sun < -18.0f && sun >= -18.0f) nt->dawn = CROSS(-18.0f);
            if (prev_sun < -0.833f && sun >= -0.833f) nt->sunrise = CROSS(-0.833f);
            #undef CROSS
        }
        prev_sun = sun;
        if (k < 144 && sun < -18.0f)
        {
            nt->dark_min += 10;
            if (moon < 0.0f)
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
        if (k == 72) nt->moon_illum = moon_illum(j, &nt->moon_waxing);   /* mezzanotte */
    }
}

/* calcolo a pezzi: una notte per giro, cosi' l'interfaccia resta fluida */
static int sky_tick()
{
    if (sky_ready) return 0;
    int d70, mins, off;
    jt_now(&d70, &mins, &off);
    if (mins < 720) d70 -= 1;   /* prima di mezzogiorno vale ancora la notte appena passata */
    sky_compute_night(sky_progress, d70 + sky_progress);
    sky_progress++;
    if (sky_progress >= SKY_NIGHTS) sky_ready = 1;
    return 1;
}

static void sky_enter() { sky_ready = 0; sky_progress = 0; sky_scroll = 0; }

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
    fill_circle(cx, cy, r, tc(COLOR_GRAY(20)));
    /* parte illuminata: cerchio pieno meno un'ellisse d'ombra disegnata a linee */
    int lit = tc(COLOR_GRAY(85));
    for (int yy = -r; yy <= r; yy++)
    {
        int half = (int)sqrtf((float)(r * r - yy * yy));
        int term = half * (1000 - 2 * illum) / 1000;    /* bordo del terminatore */
        int x0 = waxing ? cx + term : cx - half;
        int x1 = waxing ? cx + half : cx - term;
        if (x1 > x0) draw_line(x0, cy + yy, x1, cy + yy, lit);
    }
}

static void draw_sky()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Calendario del cielo");
    if (!sky_ready)
    {
        txt_c(FONT_MED, C_DIM, 360, 200, "Calcolo di Sole, Luna e Via Lattea...");
        draw_progress(160, 240, 400, 10, sky_progress * 1000 / SKY_NIGHTS);
        draw_footer("MENU indietro");
        anim_keep();
        return;
    }
    char buf[96], a[8], b[8];
    struct night *t = &nights[0];
    draw_moon_icon(76, 112, 34, t->moon_illum, t->moon_waxing);
    snprintf(buf, sizeof(buf), "%s  %d%%", moon_phase_name(t->moon_illum, t->moon_waxing), (t->moon_illum + 5) / 10);
    txt(FONT_MED_LARGE, C_TEXT, 130, 70, buf);
    if (t->dusk >= 0 && t->dawn >= 0)
    {
        fmt_hm(a, sizeof(a), t->dusk); fmt_hm(b, sizeof(b), t->dawn);
        snprintf(buf, sizeof(buf), "Buio %s - %s   senza Luna %dh%02d", a, b, t->moonless_min / 60, t->moonless_min % 60);
    }
    else snprintf(buf, sizeof(buf), "Stanotte il cielo non diventa del tutto buio");
    txt(FONT_MED, C_DIM, 130, 106, buf);
    if (t->gc_start >= 0)
    {
        fmt_hm(a, sizeof(a), t->gc_start); fmt_hm(b, sizeof(b), t->gc_end);
        snprintf(buf, sizeof(buf), "Centro Via Lattea %s - %s, alto fino a %d gradi", a, b, t->gc_max_alt);
        txt(FONT_MED, C_OK, 130, 134, buf);
    }
    else txt(FONT_MED, C_DIM, 130, 134, "Centro Via Lattea: non visibile col buio senza Luna");

    /* tabella delle prossime notti */
    int y = 176;
    txt(FONT_SMALL, C_DIM, 30, y, "NOTTE");
    txt(FONT_SMALL, C_DIM, 150, y, "LUNA");
    txt(FONT_SMALL, C_DIM, 260, y, "BUIO SENZA LUNA");
    txt(FONT_SMALL, C_DIM, 470, y, "VIA LATTEA (CENTRO)");
    static const char *const wd[] = { "dom", "lun", "mar", "mer", "gio", "ven", "sab" };
    for (int i = 0; i < SKY_NIGHTS; i++)
    {
        struct night *n = &nights[i];
        int ry = y + 22 + i * 34;
        if (i % 2 == 0) fill(C_PANEL, 24, ry - 4, SCR_W - 48, 32);
        int z = n->day1970 + 719468, era = (z >= 0 ? z : z - 146096) / 146097;
        int doe = z - era * 146097, yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        int doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
        int dd = doy - (153 * mp + 2) / 5 + 1, mm = mp + (mp < 10 ? 3 : -9);
        snprintf(buf, sizeof(buf), "%s %d/%d", wd[weekday(n->day1970)], dd, mm);
        txt(FONT_MED, i == 0 ? C_ACCENT : C_TEXT, 30, ry, buf);
        draw_moon_icon(162, ry + 12, 10, n->moon_illum, n->moon_waxing);
        snprintf(buf, sizeof(buf), "%d%%", (n->moon_illum + 5) / 10);
        txt(FONT_MED, C_DIM, 180, ry, buf);
        int bar = n->moonless_min * 130 / 720;
        fill(C_LINE, 260, ry + 8, 130, 10);
        fill(C_ACCENT, 260, ry + 8, MIN(bar, 130), 10);
        snprintf(buf, sizeof(buf), "%dh%02d", n->moonless_min / 60, n->moonless_min % 60);
        txt(FONT_MED, C_TEXT, 398, ry, buf);
        if (n->gc_start >= 0)
        {
            fmt_hm(a, sizeof(a), n->gc_start); fmt_hm(b, sizeof(b), n->gc_end);
            snprintf(buf, sizeof(buf), "%s-%s", a, b);
            txt(FONT_MED, C_OK, 470, ry, buf);
        }
        else txt(FONT_MED, C_DIM, 470, ry, "-");
    }
    draw_footer("Calcolato in macchina per il tuo luogo   MENU indietro");
}

/* ------------------------------------------------------------------ menu Astro */

static const struct menu_row astro_rows[] = {
    { "Via Lattea (regola NPF)", "Tempo massimo, ISO e bianco dall'obiettivo", SCR_NPF },
    { "Intervallometro", "Via Lattea, Deep sky, Star trails, Giorno-notte, Dark...", SCR_INTERVAL },
    { "Fuoco sulle stelle", "Misura HFR in Live View, zoom e passi di fuoco", SCR_STARS },
    { "Allineamento polare", "Dove mettere la Polare nel cannocchiale", SCR_POLAR },
    { "Calendario del cielo", "Luna, buio e Via Lattea nelle prossime 7 notti", SCR_SKY },
    { "Etichetta file", "LGHT, DARK, FLAT, BIAS al posto di IMG_", -1 },
};
#define NUM_ASTRO_ROWS ((int)COUNT(astro_rows))
