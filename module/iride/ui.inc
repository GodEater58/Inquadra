/* ui.inc - tema, testo, doppio buffer, animazioni, elementi grafici */

#ifndef NO_BG_ERASE
#define NO_BG_ERASE 0xFF
#endif

/* ------------------------------------------------------------------ tema */

static int C_BG, C_PANEL, C_PANEL2, C_SEL, C_LINE, C_TEXT, C_DIM, C_ACCENT, C_OK, C_WARN;
static int night = 0;

static int astro_screen(int s)
{
    return s == SCR_ASTRO || s == SCR_NPF || s == SCR_INTERVAL || s == SCR_STARS ||
           s == SCR_POLAR || s == SCR_SKY;
}

static void theme_apply()
{
    night = (opt_theme == 1) || (opt_theme == 2 && astro_screen(screen));
    if (!night)
    {
        C_BG = COLOR_BLACK;       C_PANEL = COLOR_GRAY(10); C_PANEL2 = COLOR_GRAY(16);
        C_SEL = COLOR_GRAY(26);   C_LINE = COLOR_GRAY(34);  C_TEXT = COLOR_WHITE;
        C_DIM = COLOR_GRAY(58);   C_ACCENT = COLOR_ORANGE;  C_OK = COLOR_GREEN1;
        C_WARN = COLOR_RED;
    }
    else
    {
        /* modalita' notte: solo rossi, per non perdere l'adattamento al buio */
        C_BG = COLOR_BLACK;       C_PANEL = COLOR_BLACK;    C_PANEL2 = COLOR_BLACK;
        C_SEL = COLOR_DARK_RED;   C_LINE = COLOR_DARK_RED;  C_TEXT = COLOR_RED;
        C_DIM = COLOR_DARK_RED;   C_ACCENT = COLOR_RED;     C_OK = COLOR_RED;
        C_WARN = COLOR_RED;
    }
}

/* qualsiasi colore (rullini, icone) adattato al tema */
static int tc(int c)
{
    if (!night || c == COLOR_BLACK || c == COLOR_EMPTY) return c;
    if (c >= 38 && c <= 79) return (c - 38) > 22 ? COLOR_RED : COLOR_DARK_RED;
    if (c == COLOR_DARK_RED || c == COLOR_BLUE || c == COLOR_GREEN2) return COLOR_DARK_RED;
    return COLOR_RED;
}

/* ------------------------------------------------------------------ testo (sempre senza sfondo) */

static void txt(int font, int fg, int x, int y, const char *s)
{
    bmp_printf(FONT(font, fg, NO_BG_ERASE), x, y, "%s", s);
}

static int txt_w(int font, const char *s)
{
    return bmp_string_width(FONT(font, COLOR_WHITE, NO_BG_ERASE), s);
}

static void txt_c(int font, int fg, int cx, int y, const char *s) { txt(font, fg, cx - txt_w(font, s) / 2, y, s); }
static void txt_r(int font, int fg, int xr, int y, const char *s) { txt(font, fg, xr - txt_w(font, s), y, s); }

static void fmt_tenths(char *buf, int len, int v10)
{
    if (v10 < 0) v10 = 0;
    snprintf(buf, len, "%d.%d", v10 / 10, v10 % 10);
}

static void fmt_hm(char *buf, int len, int minutes)
{
    minutes = MOD(minutes, 1440);
    snprintf(buf, len, "%02d:%02d", minutes / 60, minutes % 60);
}

/* ------------------------------------------------------------------ forme */

static void fill(int c, int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCR_W) w = SCR_W - x;
    if (y + h > SCR_H) h = SCR_H - y;
    if (w > 0 && h > 0) bmp_fill(c, x, y, w, h);
}

static void frame_rect(int c, int x, int y, int w, int h, int t)
{
    for (int i = 0; i < t; i++) bmp_draw_rect(c, x + i, y + i, w - 2 * i, h - 2 * i);
}

/* ------------------------------------------------------------------ animazioni a tempo (mai bloccanti) */

static int anim_until = 0;
static void anim_keep() { anim_until = get_ms_clock() + 40; }
static int anim_active() { return opt_anim != 2 && get_ms_clock() < anim_until; }

static int anim_ms(int full)
{
    if (opt_anim == 2) return 0;
    return opt_anim == 1 ? full * 6 / 10 : full;
}

/* ease-out cubica, risultato in millesimi */
static int ease(int t, int dur)
{
    if (dur <= 0 || t >= dur) return 1000;
    if (t <= 0) return 0;
    int q = 1000 - t * 1000 / dur;
    return 1000 - q * q / 1000 * q / 1000;
}

/* altre curve per le animazioni dei rullini (millesimi; 0 prima, 1000 dopo) */
enum { EZ_OUT, EZ_IO, EZ_IN, EZ_BACK };
static int ease_k(int t, int dur, int kind)
{
    /* solo interi: con -ffast-math x*x*x in float diventerebbe __powisf2, che ML non esporta */
    if (dur <= 0 || t >= dur) return 1000;
    if (t <= 0) return 0;
    if (kind == EZ_OUT) return ease(t, dur);
    int x = t * 1000 / dur;
    if (kind == EZ_IN) return x * x / 1000 * x / 1000;
    if (kind == EZ_IO)
    {
        if (x < 500) return 4 * (x * x / 1000 * x / 1000);
        int q = 2000 - 2 * x;
        return 1000 - q * q / 1000 * q / 1000 / 2;
    }
    /* uscita con piccolo rimbalzo oltre l'arrivo: 1 + 2.25 q^3 + 1.25 q^2, q = x - 1 */
    int q = x - 1000;
    int q2 = q * q / 1000, q3 = q2 * q / 1000;
    return 1000 + q3 * 225 / 100 + q2 * 125 / 100;
}

/* fase [a, a + d) di una timeline: millesimi con la curva scelta */
static int phase(int tv, int a, int d, int kind) { return ease_k(tv - a, d, kind); }
static int lerp(int a, int b, int e) { return a + (b - a) * e / 1000; }

/* avanzamento (millesimi) di un'animazione partita a t0; tiene vivo il ridisegno finche' dura */
static int anim_e(int t0, int full, int kind)
{
    int dur = anim_ms(full), t = get_ms_clock() - t0;
    if (!dur || t >= dur) return 1000;
    anim_keep();
    return ease_k(t, dur, kind);
}

/* colore che si accende: la palette non ha trasparenze, quindi una rampa di grigi
   fino al colore finale (e in millesimi, 1000 = colore pieno) */
static int fade_c(int c, int e)
{
    if (e >= 1000) return c;
    if (night) return e < 550 ? COLOR_DARK_RED : c;
    int gray = c >= 38 && c <= 79;
    if (!gray && c != COLOR_WHITE)
        return e >= 650 ? c : COLOR_GRAY(8 + 52 * e / 650);
    int top = gray ? (c - 38) * 100 / 41 : 100;
    return COLOR_GRAY(MAX(8, top * MIN(e, 1000) / 1000));
}

/* testo che entra: scivola di (dx, dy) pixel verso il suo posto e si accende */
static void txt_in(int font, int c, int x, int y, const char *s, int e, int dx, int dy)
{
    if (e <= 0) return;
    txt(font, fade_c(c, e), x + dx * (1000 - e) / 1000, y + dy * (1000 - e) / 1000, s);
}

static void txt_c_in(int font, int c, int cx, int y, const char *s, int e, int dx, int dy)
{
    txt_in(font, c, cx - txt_w(font, s) / 2, y, s, e, dx, dy);
}

/* valore appena cambiato (</> o SET): entra scorrendo dal lato della freccia */
static int val_t0 = -100000, val_dir = 0;
static void val_kick(int dir) { val_t0 = get_ms_clock(); val_dir = dir; }

/* valore animato x1000 che insegue il suo obiettivo (carosello, sottolineatura) */
struct glider { int vis, from, to, t0; };
static int glide(struct glider *g, int target, int full)
{
    int now = get_ms_clock(), dur = anim_ms(full);
    if (g->vis < 0 || dur == 0) { g->vis = g->from = g->to = target; return target; }
    if (target != g->to) { g->from = g->vis; g->to = target; g->t0 = now; }
    int e = ease(now - g->t0, dur);
    g->vis = lerp(g->from, g->to, e);
    if (e < 1000) anim_keep();
    return g->vis;
}

/* barra di selezione elastica: il bordo davanti arriva subito, quello dietro la segue */
static int hl_to = -1, hl_lead, hl_tail, hl_fl, hl_ft, hl_t0;

static void hl_reset() { hl_to = -1; }

/* restituisce l'altezza della barra (riga h piu' lo stiramento) e in *top la sua cima;
   in *lead la posizione del bordo che guida (per la tacca colorata) */
static int hl_span(int target, int h, int *top, int *lead)
{
    int now = get_ms_clock(), d1 = anim_ms(110), d2 = anim_ms(230);
    if (hl_to < 0 || !d1) { hl_to = hl_lead = hl_tail = hl_fl = hl_ft = target; hl_t0 = now - 100000; }
    else if (target != hl_to) { hl_fl = hl_lead; hl_ft = hl_tail; hl_to = target; hl_t0 = now; }
    int t = now - hl_t0;
    hl_lead = lerp(hl_fl, hl_to, ease(t, d1));
    hl_tail = lerp(hl_ft, hl_to, ease(t, d2));
    if (t < d2) anim_keep();
    *top = MIN(hl_lead, hl_tail);
    if (lead) *lead = hl_lead;
    return MAX(hl_lead, hl_tail) - *top + h;
}

/* barra completa: fondo elastico e tacca colorata che guida */
static void hl_bar(int x, int w, int target, int h)
{
    int top, lead;
    int hh = hl_span(target, h, &top, &lead);
    fill(C_SEL, x, top, w, hh);
    fill(C_ACCENT, x, lead, 6, h);
}

/* ------------------------------------------------------------------ doppio buffer e transizioni */

static void frame_begin() { bmp_draw_to_idle(1); }

/* Transizioni tra schermate (solo l'area centrale; intestazione e piede cambiano subito):
 *  +1/-1  la nuova schermata entra da destra/sinistra, la vecchia scivola via a meta' velocita'
 *  +2/-2  un dialogo sale dal basso come un foglio / riscende scoprendo la schermata sotto
 *  +3/-3  zoom dal riquadro della Home (zoom_*) a tutto schermo / ritorno nel riquadro */
#define TR_Y0 47
#define TR_Y1 444
static int zoom_x = 0, zoom_y = TR_Y0, zoom_w = SCR_W, zoom_h = TR_Y1 - TR_Y0;

static void vram_fill(uint8_t *buf, int c, int x, int y, int w, int h)
{
    if (x < 0) { w += x; x = 0; }
    if (y < TR_Y0) { h += y - TR_Y0; y = TR_Y0; }
    if (x + w > SCR_W) w = SCR_W - x;
    if (y + h > TR_Y1) h = TR_Y1 - y;
    for (int yy = y; yy < y + h && w > 0; yy++) memset(buf + yy * BMPPITCH + x, c, w);
}

/* spinta orizzontale con parallasse; restituisce i pixel gia' entrati */
static int tr_push(uint8_t *real, uint8_t *idle, int dir, int prev, int target)
{
    if (target <= prev) return prev;
    int delta = target / 2 - prev / 2;           /* la vecchia schermata si sposta di target/2 */
    for (int y = TR_Y0; y < TR_Y1; y++)
    {
        uint8_t *r = real + y * BMPPITCH, *i = idle + y * BMPPITCH;
        if (dir > 0)
        {
            if (delta) memmove(r, r + delta, SCR_W - target);
            memcpy(r + SCR_W - target, i, target);
        }
        else
        {
            if (delta) memmove(r + target, r + target - delta, SCR_W - target);
            memcpy(r, i + SCR_W - target, target);
        }
    }
    return target;
}

/* foglio verticale; restituisce lo spostamento gia' fatto */
static int tr_sheet(uint8_t *real, uint8_t *idle, int dir, int prev, int e)
{
    const int H = TR_Y1 - TR_Y0;
    if (dir > 0)
    {
        /* sale: la cima del foglio e' a ys, sotto c'e' la nuova schermata dall'alto */
        int ys = TR_Y0 + H * (1000 - MIN(e, 1000)) / 1000;
        for (int y = ys; y < TR_Y1; y++) memcpy(real + y * BMPPITCH, idle + (TR_Y0 + y - ys) * BMPPITCH, SCR_W);
        if (ys - 2 >= TR_Y0) vram_fill(real, C_ACCENT, 0, ys - 2, SCR_W, 2);
        return 0;
    }
    /* scende: la vecchia schermata trasla in basso, sopra si scopre la nuova */
    int off = H * MIN(e, 1000) / 1000;
    if (off <= prev) return prev;
    int d = off - prev;
    for (int y = TR_Y1 - 1; y >= TR_Y0 + off; y--) memcpy(real + y * BMPPITCH, real + (y - d) * BMPPITCH, SCR_W);
    for (int y = TR_Y0 + prev; y < TR_Y0 + off; y++) memcpy(real + y * BMPPITCH, idle + y * BMPPITCH, SCR_W);
    return off;
}

/* zoom: il rettangolo cresce dal riquadro (o ci rientra) con un bordo arancione e dentro
   c'e' la schermata intera rimpicciolita (proporzioni giuste, fondo come il riquadro).
   src: l'area centrale della schermata che si vede dentro (pitch = SCR_W se copia, NULL = ritaglio) */
static void tr_zoom(uint8_t *real, uint8_t *idle, const uint8_t *src, int pitch, int dir, int e)
{
    const int H = TR_Y1 - TR_Y0;
    int p = dir > 0 ? e : 1000 - e;              /* 0 = riquadro, 1000 = schermo intero */
    int x = lerp(zoom_x, 0, p), y = lerp(zoom_y, TR_Y0, p);
    int w = lerp(zoom_w, SCR_W, p), h = lerp(zoom_h, H, p);
    x = COERCE(x, 0, SCR_W - 1); y = COERCE(y, TR_Y0, TR_Y1 - 1);
    w = COERCE(w, 1, SCR_W - x); h = COERCE(h, 1, TR_Y1 - y);
    if (dir < 0)
        for (int yy = TR_Y0; yy < TR_Y1; yy++)
        {
            uint8_t *r = real + yy * BMPPITCH, *i = idle + yy * BMPPITCH;
            if (yy < y || yy >= y + h) memcpy(r, i, SCR_W);
            else { memcpy(r, i, x); memcpy(r + x + w, i + x + w, SCR_W - x - w); }
        }
    if (src)
    {
        /* stessa scala in orizzontale e in verticale: niente schermata schiacciata */
        int k = MIN(w * 1000 / SCR_W, h * 1000 / H);
        int tw = COERCE(SCR_W * k / 1000, 1, w), th = COERCE(H * k / 1000, 1, h);
        int ox = x + (w - tw) / 2, oy = y + (h - th) / 2;
        if (tw < w || th < h) vram_fill(real, C_SEL, x, y, w, h);
        static short xm[SCR_W];
        for (int i = 0; i < tw; i++) xm[i] = i * SCR_W / tw;
        for (int j = 0; j < th; j++)
        {
            const uint8_t *sr = src + (j * H / th) * pitch;
            uint8_t *dr = real + (oy + j) * BMPPITCH + ox;
            for (int i = 0; i < tw; i++) dr[i] = sr[xm[i]];
        }
    }
    else if (dir > 0)
        for (int yy = y; yy < y + h; yy++) memcpy(real + yy * BMPPITCH + x, idle + yy * BMPPITCH + x, w);
    const int t = 3;
    vram_fill(real, C_ACCENT, x, y, w, t);
    vram_fill(real, C_ACCENT, x, y + h - t, w, t);
    vram_fill(real, C_ACCENT, x, y, t, h);
    vram_fill(real, C_ACCENT, x + w - t, y, t, h);
}

static void frame_show(int dir)
{
    bmp_draw_to_idle(0);
    int kind = ABS(dir);
    int dur = anim_ms(kind == 3 ? 260 : kind == 2 ? 240 : 220);
    uint8_t *real = bmp_vram_real();
    uint8_t *idle = bmp_vram_idle();
    if (!dir || kind > 3 || !dur || !real || !idle || key_pending())
    {
        bmp_idle_copy(1, 0);
        return;
    }

    for (int y = 0; y < SCR_H; y++)
        if (y < TR_Y0 || y >= TR_Y1) memcpy(real + y * BMPPITCH, idle + y * BMPPITCH, SCR_W);

    /* zoom: dentro il rettangolo la schermata intera rimpicciolita. In avanti e' la nuova
       (gia' pronta nel buffer nascosto); indietro e' la vecchia, che va copiata da parte
       perche' sullo schermo viene sovrascritta. Senza memoria: ritaglio. */
    const uint8_t *zsrc = 0;
    int zpitch = BMPPITCH;
    uint8_t *zcopy = 0;
    if (kind == 3)
    {
        if (dir > 0) zsrc = idle + TR_Y0 * BMPPITCH;
        else if ((zcopy = tmp_malloc(SCR_W * (TR_Y1 - TR_Y0))) != 0)
        {
            for (int y = TR_Y0; y < TR_Y1; y++) memcpy(zcopy + (y - TR_Y0) * SCR_W, real + y * BMPPITCH, SCR_W);
            zsrc = zcopy; zpitch = SCR_W;
        }
    }

    int t0 = get_ms_clock(), prev = 0;
    for (;;)
    {
        if (key_pending()) break;           /* un tasto premuto conta piu' dell'animazione */
        int t = get_ms_clock() - t0;
        int e = ease(t, dur);
        if (kind == 1) prev = tr_push(real, idle, dir, prev, SCR_W * e / 1000);
        else if (kind == 2) prev = tr_sheet(real, idle, dir, prev, e);
        else tr_zoom(real, idle, zsrc, zpitch, dir, e);
        if (t >= dur) break;
        msleep(1);
    }
    if (zcopy) free(zcopy);
    bmp_idle_copy(1, 0);
}

/* ------------------------------------------------------------------ intestazione e piede */

static void draw_header(const char *section)
{
    fill(C_BG, 0, 0, SCR_W, 46);
    txt(FONT_LARGE, C_ACCENT, 16, 6, "IRIDE");
    int w = txt_w(FONT_LARGE, "IRIDE");
    txt(FONT_MED, C_DIM, 16 + w + 14, 12, section);

    char buf[48];
    int x = SCR_W - 16;
    if (P.roll_loaded)
    {
        snprintf(buf, sizeof(buf), "R%03d %d/%d%s", P.roll_num % 1000, P.roll_shots, P.roll_exp, P.roll_paused ? " II" : "");
        txt_r(FONT_MED, P.roll_paused ? C_DIM : C_TEXT, x, 12, buf);
        x -= txt_w(FONT_MED, buf) + 14;
    }
    struct tm now;
    LoadCalendarFromRTC(&now);
    snprintf(buf, sizeof(buf), "%02d:%02d", now.tm_hour, now.tm_min);
    txt_r(FONT_MED, C_DIM, x, 12, buf);
    x -= txt_w(FONT_MED, buf) + 12;
    if (P.snap_valid)
    {
        /* pallino: ci sono impostazioni da ripristinare con "Torna alla 6D" */
        fill_circle(x - 5, 23, 5, C_ACCENT);
    }
    fill(C_ACCENT, 16, 44, SCR_W - 32, 2);
}

/* piede: suggerimenti dei tasti, oppure un avviso che entra con la sua tacca e poi si spegne */
static void draw_footer(const char *hints)
{
    fill(C_BG, 0, 444, SCR_W, 36);
    fill(C_LINE, 16, 444, SCR_W - 32, 1);
    int now = get_ms_clock();
    if (msg[0] && now < msg_until)
    {
        int ein = anim_e(msg_t0, 260, EZ_BACK);
        int dout = anim_ms(220), left = msg_until - now;
        int eout = dout && left < dout ? left * 1000 / dout : 1000;
        if (eout < 1000) anim_keep();
        int bh = 22 * MIN(ein, 1000) / 1000 * eout / 1000;
        if (bh > 0) fill(msg_color == C_WARN ? C_WARN : C_ACCENT, 16, 463 - bh / 2, 6, bh);
        txt_in(FONT_MED, msg_color, 30, 450, msg, MIN(MIN(ein, 1000), eout), ein < 1000 ? 26 : 0, 0);
    }
    else if (hints && hints[0])
    {
        /* dopo un avviso i suggerimenti tornano piano */
        int e = msg_until && now >= msg_until ? anim_e(msg_until, 240, EZ_OUT) : 1000;
        txt_in(FONT_MED, C_DIM, 16, 450, hints, e, 0, 0);
    }
}

/* ------------------------------------------------------------------ elementi */

/* interruttore ON/OFF; e < 1000 mentre il pallino corre dall'altra parte */
static void draw_pill(int on, int xr, int y, int e)
{
    int w = 64, h = 26, x = xr - w;
    int p = on ? e : 1000 - e;                  /* 0 = pallino a sinistra (OFF), 1000 = a destra (ON) */
    p = COERCE(p, -60, 1060);
    int lit = p >= 500;
    fill(lit ? C_ACCENT : C_LINE, x, y, w, h);
    fill(lit ? C_TEXT : C_BG, x + 4 + 30 * p / 1000, y + 4, 26, h - 8);
    int el = (e - 500) * 2;                     /* la scritta compare a corsa quasi finita */
    if (el > 0) txt(FONT_SMALL, fade_c(on ? C_BG : C_DIM, on ? 1000 : el), on ? x + 8 : x + 34, y + 7, on ? "ON" : "OFF");
}

static void draw_progress(int x, int y, int w, int h, int permille)
{
    fill(C_LINE, x, y, w, h);
    fill(C_ACCENT, x, y, w * COERCE(permille, 0, 1000) / 1000, h);
}

/* cerchio con la spunta che si disegna da sola (e: ms dall'inizio, completa da 560 in poi) */
static void draw_check(int cx, int cy, int r, int e)
{
    if (e <= 0) return;
    int rc = r * ease_k(e, 320, EZ_BACK) / 1000;
    if (rc < 2) return;
    fill_circle(cx, cy, rc, C_OK);
    int sp = ease_k(e - 220, 340, EZ_OUT);                 /* finita a 560 ms */
    if (sp <= 0 || rc < r * 8 / 10) return;
    int ax = cx - r * 45 / 100, ay = cy + r * 2 / 100;
    int bx = cx - r * 12 / 100, by = cy + r * 34 / 100;
    int qx = cx + r * 48 / 100, qy = cy - r * 30 / 100;
    int k1 = MIN(sp * 1000 / 350, 1000), k2 = MAX(0, (sp - 350) * 1000 / 650);
    for (int o = -2; o <= 1; o++)
    {
        draw_line(ax, ay + o, lerp(ax, bx, k1), lerp(ay, by, k1) + o, COLOR_BLACK);
        if (k2 > 0) draw_line(bx, by + o, lerp(bx, qx, k2), lerp(by, qy, k2) + o, COLOR_BLACK);
    }
}

/* ------------------------------------------------------------------ icone della Home */

/* numeri pseudo-casuali ripetibili (stelle sempre al loro posto) */
static unsigned icon_rng = 1;
static int irand(int n) { icon_rng = icon_rng * 1103515245u + 12345u; return (int)((icon_rng >> 16) % (unsigned)n); }

/* stella con croce di luce */
static void star(int x, int y, int r, int c)
{
    fill_circle(x, y, r, c);
    if (r >= 2)
    {
        fill(c, x - r - 3, y, 2 * r + 7, 1);
        fill(c, x, y - r - 3, 1, 2 * r + 7);
    }
}

/* Astro: Via Lattea, costellazione, falce di luna, montagne con treppiede. Area 190 x 100.
   t >= 0: animazione (ms) quando la casella viene scelta: passa una stella cadente */
static void icon_sky(int x, int y, int bg, int t)
{
    int w = 190, h = 100;
    /* Via Lattea: fascia diagonale di puntini, piu' fitta al centro */
    icon_rng = 7;
    for (int i = 0; i < 90; i++)
    {
        int t = irand(w);
        int spread = 14 - ABS(t - w / 2) / 10;
        int px = x + t, py = y + 70 - t * 55 / w + irand(2 * spread + 1) - spread;
        int c = irand(4) == 0 ? COLOR_WHITE : irand(2) ? COLOR_GRAY(62) : COLOR_GRAY(44);
        fill(tc(c), px, py, 1 + (irand(5) == 0), 1);
    }
    /* stelle di campo */
    for (int i = 0; i < 16; i++) fill(tc(COLOR_GRAY(80)), x + irand(w), y + irand(h - 34), 1, 1);
    /* costellazione */
    static const short cs[6][2] = { {18, 30}, {46, 44}, {74, 36}, {98, 52}, {122, 40}, {112, 18} };
    for (int i = 0; i < 5; i++)
        draw_line(x + cs[i][0], y + cs[i][1], x + cs[i + 1][0], y + cs[i + 1][1], tc(COLOR_GRAY(40)));
    int flare = t >= 0 && t < 420 ? (t < 210 ? t : 420 - t) / 70 : 0;   /* la stella gialla brilla */
    for (int i = 0; i < 6; i++)
        star(x + cs[i][0], y + cs[i][1], (i == 2 ? 3 + flare : i == 5 ? 2 : 1), tc(i == 2 ? COLOR_YELLOW : i == 4 ? COLOR_LIGHT_BLUE : COLOR_WHITE));
    if (t >= 120 && t < 900)
    {
        /* stella cadente: dall'alto a destra verso il basso a sinistra, scia che si allunga e svanisce */
        int e = ease_k(t - 120, 600, EZ_IN), tail = (t < 520 ? (t - 120) : 900 - t) * 34 / 400;
        int hx = x + 150 - 110 * MIN(e, 1000) / 1000, hy = y + 4 + 40 * MIN(e, 1000) / 1000;
        if (t < 720)
        {
            draw_line(hx, hy, hx + tail, hy - tail / 3, tc(COLOR_GRAY(70)));
            draw_line(hx, hy + 1, hx + tail / 2, hy + 1 - tail / 6, tc(COLOR_GRAY(52)));
            draw_line(hx + tail / 2, hy - tail / 6, hx + tail, hy - tail / 3, tc(COLOR_GRAY(40)));
            fill(tc(COLOR_WHITE), hx - 1, hy - 1, 3, 3);
        }
    }
    /* falce di luna */
    fill_circle(x + 166, y + 18, 12, tc(COLOR_ALMOST_WHITE));
    fill_circle(x + 171, y + 14, 11, bg);
    /* montagne: catena lontana in grigio e colline vicine in nero, treppiede sulla cima */
    static const short far_pk[3][3] = { {40, 46, 48}, {120, 54, 52}, {178, 40, 46} };   /* x, altezza, pendenza% */
    static const short near_pk[3][3] = { {10, 22, 40}, {84, 34, 44}, {160, 24, 38} };
    for (int layer = 0; layer < 2; layer++)
    {
        const short (*pk)[3] = layer ? near_pk : far_pk;
        int col = layer ? COLOR_BLACK : tc(COLOR_GRAY(14));
        for (int c = 0; c < w; c += 2)
        {
            int hh = 0;
            for (int k = 0; k < 3; k++)
            {
                int v = pk[k][1] - ABS(c - pk[k][0]) * pk[k][2] / 100;
                if (v > hh) hh = v;
            }
            if (hh > 0) fill(col, x + c, y + h - hh, 2, hh);
        }
    }
    /* neve sulla cima lontana piu' alta */
    for (int c = -6; c <= 6; c += 2) fill(tc(COLOR_GRAY(60)), x + 120 + c, y + h - 54 + ABS(c) * 52 / 100, 2, 2);
    int tx = x + 84, ty = y + h - 34;            /* cima della collina vicina */
    draw_line(tx, ty - 10, tx - 6, ty, tc(COLOR_GRAY(70)));
    draw_line(tx, ty - 10, tx + 6, ty, tc(COLOR_GRAY(70)));
    draw_line(tx, ty - 10, tx + 1, ty, tc(COLOR_GRAY(70)));
    fill(tc(COLOR_GRAY(70)), tx - 4, ty - 16, 9, 6);
    fill(C_ACCENT, tx + 3, ty - 18, 2, 2);
}

/* poligono convesso pieno (una riga orizzontale per y) */
static void fill_poly(const int *px, const int *py, int n, int c)
{
    int ymin = 10000, ymax = -10000;
    for (int k = 0; k < n; k++) { ymin = MIN(ymin, py[k]); ymax = MAX(ymax, py[k]); }
    for (int yy = ymin; yy <= ymax; yy++)
    {
        int xa = 10000, xb = -10000;
        for (int e = 0; e < n; e++)
        {
            int ax = px[e], ay = py[e], bx = px[(e + 1) % n], by = py[(e + 1) % n];
            if ((yy < ay && yy < by) || (yy > ay && yy > by)) continue;
            if (ay == by) { xa = MIN(xa, MIN(ax, bx)); xb = MAX(xb, MAX(ax, bx)); continue; }
            int xx = ax + (bx - ax) * (yy - ay) / (by - ay);
            xa = MIN(xa, xx); xb = MAX(xb, xx);
        }
        if (xb >= xa) fill(c, xa, yy, xb - xa + 1, 1);
    }
}

static void fill_tri(int x0, int y0, int x1, int y1, int x2, int y2, int c)
{
    int px[3] = { x0, x1, x2 }, py[3] = { y0, y1, y2 };
    fill_poly(px, py, 3, c);
}

/* diaframma a 6 lamelle: anello di raggio R, apertura esagonale r ruotata di rot (radianti).
   I lati dell'esagono proseguono fino all'anello: sono i bordi delle lamelle. */
static void draw_iris(int cx, int cy, int R, int r, float rot, int blade, int edge, int hole)
{
    fill_circle(cx, cy, R, blade);
    float fr = (float)MAX(r, 0), fR = (float)R;
    float t = -fr * 0.5f + sqrtf(MAX(fR * fR - 0.75f * fr * fr, 0.0f));   /* dal vertice all'anello */
    int px[6], py[6];
    float vx[6], vy[6];
    for (int i = 0; i < 6; i++)
    {
        float a = rot + (float)i * 1.0472f;
        vx[i] = fr * cosf(a); vy[i] = fr * sinf(a);
        px[i] = cx + (int)vx[i]; py[i] = cy + (int)vy[i];
    }
    if (r > 1) fill_poly(px, py, 6, hole);
    for (int i = 0; i < 6; i++)
    {
        /* lato i -> i+1: direzione perpendicolare al raggio medio, prolungata dal vertice i+1 */
        float m = rot + (float)i * 1.0472f + 0.5236f;
        float dx = -sinf(m), dy = cosf(m);
        int j = (i + 1) % 6;
        draw_line(px[j], py[j], cx + (int)(vx[j] + dx * t), cy + (int)(vy[j] + dy * t), edge);
    }
    draw_circle(cx, cy, R, edge);
}

/* Foto: reflex vista di fronte. Area ~150 x 100, (x, y) = angolo in alto a sinistra del corpo.
   t >= 0: il diaframma dell'obiettivo si chiude e si riapre, come in uno scatto */
static void icon_dslr(int x, int y, int bg, int t)
{
    int body = tc(COLOR_GRAY(24)), edge = tc(COLOR_GRAY(44)), dark = tc(COLOR_GRAY(12));
    int bw = 150, bh = 74, by = y + 22;
    /* pentaprisma a gradini e slitta flash */
    for (int i = 0; i < 12; i++) fill(body, x + 52 - i * 2, by - 2 - i, 46 + i * 4, 2);
    fill(edge, x + 52 - 22, by - 14, 46 + 44, 1);
    fill(tc(COLOR_GRAY(55)), x + 64, by - 20, 22, 5);
    fill(dark, x + 66, by - 18, 18, 2);
    /* corpo con angoli smussati */
    fill(body, x + 3, by, bw - 6, bh);
    fill(body, x, by + 3, bw, bh - 6);
    fill(edge, x + 3, by, bw - 6, 1);
    /* impugnatura con texture */
    fill(dark, x, by + 8, 28, bh - 14);
    for (int yy = by + 12; yy < by + bh - 8; yy += 4)
        for (int xx = x + 3 + ((yy / 4) & 1) * 2; xx < x + 26; xx += 4) fill(tc(COLOR_GRAY(20)), xx, yy, 1, 1);
    /* pulsante di scatto e ghiera dei modi */
    fill(C_ACCENT, x + 8, by - 5, 14, 5);
    fill(tc(COLOR_GRAY(60)), x + 9, by - 5, 12, 1);
    fill(edge, x + 116, by - 6, 26, 6);
    for (int xx = x + 118; xx < x + 140; xx += 3) fill(dark, xx, by - 5, 1, 4);
    /* luce AF */
    fill_circle(x + 38, by + 14, 3, tc(COLOR_GRAY(50)));
    /* obiettivo: barilotto, ghiera zigrinata, anello arancione, lente con riflessi */
    int cx = x + 80, cy = by + 36;
    fill_circle(cx, cy, 34, edge);
    fill_circle(cx, cy, 32, body);
    for (int i = 0; i < 28; i++)
    {
        float a = (float)i * 0.2244f;
        fill(dark, cx + (int)(29.0f * cosf(a)) - 1, cy + (int)(29.0f * sinf(a)) - 1, 2, 2);
    }
    draw_circle(cx, cy, 25, C_ACCENT);
    draw_circle(cx, cy, 24, C_ACCENT);
    fill_circle(cx, cy, 22, tc(COLOR_GRAY(8)));
    if (t >= 0 && t < 560)
    {
        int k = t < 240 ? ease_k(t, 240, EZ_IN) : 1000 - ease_k(t - 240, 320, EZ_BACK);
        draw_iris(cx, cy, 21, lerp(20, 3, k), (float)k * 0.0012f, tc(COLOR_GRAY(20)), tc(COLOR_GRAY(42)), COLOR_BLACK);
        if (t >= 200 && t < 300) fill(tc(COLOR_WHITE), x + 8, by - 5, 14, 5);    /* scatto */
    }
    else
    {
        fill_circle(cx, cy, 16, COLOR_BLACK);
        fill_circle(cx - 3, cy - 3, 9, tc(COLOR_BLUE));
        fill_circle(cx - 4, cy - 4, 6, COLOR_BLACK);
        fill_circle(cx - 8, cy - 9, 3, tc(COLOR_LIGHT_BLUE));
        fill(tc(COLOR_WHITE), cx - 9, cy - 10, 2, 2);
        fill(tc(COLOR_GRAY(50)), cx + 7, cy + 8, 3, 2);
    }
    (void)bg;
}

/* Torna alla 6D: freccia circolare spessa (antioraria) con una piccola reflex al centro.
   t >= 0: la freccia fa un giro completo */
static void icon_undo(int x, int y, int t)
{
    int c = C_ACCENT;
    float spin = t >= 0 ? (float)ease_k(t, 650, EZ_IO) * 0.0062832f : 0.0f;
    const float a0 = 1.05f + spin, a1 = 5.70f + spin;    /* da 60 a 327 gradi */
    for (int i = 0; i <= 40; i++)
    {
        float a = a0 + (a1 - a0) * (float)i / 40.0f;
        int px = x + (int)(17.0f * cosf(a)), py = y - (int)(17.0f * sinf(a));
        fill(c, px - 2, py - 2, 5, 5);
    }
    /* punta in partenza (in alto a destra), verso il senso di rotazione */
    float ca = cosf(a0), sa = sinf(a0);
    int ax = x + (int)(17.0f * ca), ay = y - (int)(17.0f * sa);
    int tx = (int)(-sa * 9.0f), ty = (int)(-ca * 9.0f);            /* tangente (verso antiorario) */
    int nx = (int)(ca * 8.0f), ny = (int)(-sa * 8.0f);             /* normale */
    fill_tri(ax + nx, ay + ny, ax - nx, ay - ny, ax + tx * 13 / 10, ay + ty * 13 / 10, c);
    /* reflex al centro */
    int g = tc(COLOR_GRAY(70));
    fill(g, x - 7, y - 3, 14, 9);
    fill(g, x - 3, y - 6, 6, 3);
    fill_circle(x, y + 1, 3, COLOR_BLACK);
}

/* Profili: tre cursori verticali da mixer, centro (x, y).
   t >= 0: i cursori corrono su e giu' e tornano al loro posto */
static void icon_faders(int x, int y, int t)
{
    static const signed char pos[3] = { 8, -10, 2 };
    for (int i = 0; i < 3; i++)
    {
        int fx = x - 15 + i * 15;
        fill(C_LINE, fx - 2, y - 22, 5, 44);
        fill(COLOR_BLACK, fx, y - 21, 1, 42);
        int ky = y + pos[i];
        if (t >= 0 && t < 700)
            ky = COERCE(ky + (int)(13.0f * sinf((float)t * 0.0135f + (float)i * 2.1f) * (float)(700 - t) / 700.0f), y - 17, y + 17);
        fill(C_ACCENT, fx - 6, ky - 4, 13, 9);
        fill(tc(COLOR_WHITE), fx - 5, ky, 11, 1);
        fill(tc(COLOR_GRAY(30)), fx - 6, ky + 4, 13, 1);
    }
}

/* Opzioni: due ingranaggi che si incastrano, centro del grande (x, y) */
static void gear(int x, int y, int r, int teeth, float rot, int c, int bg)
{
    for (int i = 0; i < teeth; i++)
    {
        float a = rot + (float)i * 6.2832f / (float)teeth;
        float ca = cosf(a), sa = sinf(a);
        for (int k = -1; k <= 1; k++)
        {
            int ox = (int)((float)k * 1.6f * -sa), oy = (int)((float)k * 1.6f * ca);
            draw_line(x + (int)((float)(r - 2) * ca) + ox, y + (int)((float)(r - 2) * sa) + oy,
                      x + (int)((float)(r + 4) * ca) + ox, y + (int)((float)(r + 4) * sa) + oy, c);
        }
    }
    fill_circle(x, y, r, c);
    fill_circle(x, y, r * 4 / 10, bg);
    draw_circle(x, y, r - 3, bg);
}

/* t >= 0: girano di due denti (il piccolo al contrario, piu' veloce) */
static void icon_gears(int x, int y, int bg, int t)
{
    float a = t >= 0 ? (float)ease_k(t, 700, EZ_IO) * 0.0012566f : 0.0f;
    gear(x - 3, y + 2, 15, 10, a, C_ACCENT, bg);
    gear(x + 18, y - 17, 8, 7, 0.25f - a * 10.0f / 7.0f, tc(COLOR_GRAY(70)), bg);
}

/* avvio: un diaframma si apre ruotando, "IRIDE" sale lettera per lettera,
   si allunga la linea arancione e compare la versione */
#define SPLASH_FULL 1000
static void splash()
{
    int dur = anim_ms(SPLASH_FULL);
    int t0 = get_ms_clock();
    const char *word = "IRIDE";
    int ww = txt_w(FONT_LARGE, word);
    int blade = night ? COLOR_BLACK : COLOR_GRAY(18), edge = night ? COLOR_DARK_RED : COLOR_GRAY(46);
    for (;;)
    {
        int t = get_ms_clock() - t0;
        int last = t >= dur;
        if (key_pending()) break;
        int tv = last || !dur ? SPLASH_FULL : t * SPLASH_FULL / dur;
        int e_ring = phase(tv, 0, 320, EZ_BACK);
        int e_open = phase(tv, 80, 560, EZ_IO);

        frame_begin();
        fill(C_BG, 0, 0, SCR_W, SCR_H);
        int R = lerp(12, 104, e_ring);
        if (R > 3)
        {
            draw_iris(360, 186, R, lerp(0, 70, e_open), (float)(1000 - e_open) * 0.00157f, blade, edge, C_BG);
            draw_circle(360, 186, R + 4, C_ACCENT);
            draw_circle(360, 186, R + 5, C_ACCENT);
        }
        /* lettere: salgono con un piccolo rimbalzo, una dopo l'altra */
        int x = 360 - ww / 2;
        for (int i = 0; word[i]; i++)
        {
            char ch[2] = { word[i], 0 };
            int ep = phase(tv, 360 + i * 70, 300, EZ_BACK);
            int ef = phase(tv, 360 + i * 70, 200, EZ_OUT);
            if (ef > 0) txt(FONT_LARGE, fade_c(C_TEXT, ef), x, 316 + (1000 - ep) * 22 / 1000, ch);
            x += txt_w(FONT_LARGE, ch);
        }
        int e_line = phase(tv, 640, 300, EZ_OUT);
        int lw = (ww + 60) * e_line / 1000;
        if (lw > 0) fill(C_ACCENT, 360 - lw / 2, 358, lw, 2);
        txt_c_in(FONT_MED, C_DIM, 360, 372, "Canon EOS 6D  -  " IRIDE_VERSION, phase(tv, 760, 240, EZ_OUT), 0, 8);
        frame_show(0);
        if (last) break;
        msleep(10);
    }
    for (int i = 0; i < 18 && !key_pending(); i++) msleep(10);
}

/* ------------------------------------------------------------------ moduli (form) e liste */

enum { VK_TEXT, VK_ON, VK_OFF, VK_ACTION, VK_INFO, VK_WARN };

struct form
{
    int n;
    const char *const *labels;
    void (*value)(int row, char *buf, int len, int *kind);
    void (*change)(int row, int dir, int big);
    void (*activate)(int row);
    void (*extra)(void);
    const char *hint;
    int y0, row_h;
};

/* righe etichetta + valore, con barra di selezione elastica; il valore appena cambiato
   entra scorrendo e gli interruttori fanno correre il pallino */
static void draw_form(const struct form *f, int s)
{
    int y0 = f->y0 ? f->y0 : 58;
    int rh = f->row_h ? f->row_h : 42;
    hl_bar(24, SCR_W - 48, y0 + s * rh, rh - 4);
    int ev = anim_e(val_t0, 200, EZ_OUT), ep = anim_e(val_t0, 260, EZ_BACK);
    for (int i = 0; i < f->n; i++)
    {
        int y = y0 + i * rh;
        char buf[64] = "";
        int kind = VK_TEXT;
        if (f->value) f->value(i, buf, sizeof(buf), &kind);
        int ty = y + (rh - 4 - 23) / 2;
        txt(FONT_MED, kind == VK_INFO ? C_DIM : C_TEXT, 44, ty, f->labels[i]);
        int mine = i == s;
        if (kind == VK_ON || kind == VK_OFF)
            draw_pill(kind == VK_ON, SCR_W - 44, y + (rh - 4 - 26) / 2, mine ? ep : 1000);
        else if (kind == VK_ACTION)
            txt_r(FONT_MED, C_ACCENT, SCR_W - 44, ty, buf[0] ? buf : "SET");
        else if (buf[0])
        {
            int c = kind == VK_WARN ? C_WARN : (mine ? C_ACCENT : C_TEXT);
            if (mine && ev < 1000 && val_dir)
                txt_in(FONT_MED, c, SCR_W - 44 - txt_w(FONT_MED, buf), ty, buf, ev, val_dir * 18, 0);
            else txt_r(FONT_MED, c, SCR_W - 44, ty, buf);
        }
    }
}

/* voci grandi con titolo e sottotitolo (menu Astro e Foto) */
struct menu_row { const char *title, *sub; int target; };

static void draw_menu_rows(const struct menu_row *rows, int n, int s, int rh)
{
    int y0 = 56;
    for (int i = 0; i < n; i++) fill(C_PANEL, 24, y0 + i * rh, SCR_W - 48, rh - 8);
    hl_bar(24, SCR_W - 48, y0 + s * rh, rh - 8);
    int big = rh >= 62;
    for (int i = 0; i < n; i++)
    {
        int y = y0 + i * rh;
        txt(big ? FONT_MED_LARGE : FONT_MED, C_TEXT, 46, y + 2, rows[i].title);
        txt(big ? FONT_MED : FONT_SMALL, C_DIM, 46, y + (big ? 30 : 30), rows[i].sub);
        if (rows[i].target >= 0)
            txt_r(FONT_MED, i == s ? C_ACCENT : C_LINE, SCR_W - 44, y + (rh - 8 - 23) / 2, ">");
    }
}
