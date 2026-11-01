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

/* barra di selezione che scivola tra le righe */
static int hl_cur = -1, hl_from = -1, hl_to = -1, hl_t0 = 0;

static void hl_reset() { hl_cur = -1; }

static int hl_y(int target)
{
    int now = get_ms_clock();
    int dur = anim_ms(110);
    if (hl_cur < 0 || dur == 0) { hl_cur = hl_from = hl_to = target; return target; }
    if (target != hl_to) { hl_from = hl_cur; hl_to = target; hl_t0 = now; }
    int t = now - hl_t0;
    if (t >= dur) { hl_cur = hl_to; return hl_cur; }
    hl_cur = hl_from + (hl_to - hl_from) * ease(t, dur) / 1000;
    anim_keep();
    return hl_cur;
}

/* ------------------------------------------------------------------ doppio buffer e transizioni */

static void frame_begin() { bmp_draw_to_idle(1); }

/* copia il fotogramma pronto nel buffer nascosto sullo schermo; con dir != 0 l'area
 * centrale scorre (la nuova schermata entra da destra se dir > 0, da sinistra se < 0) */
static void frame_show(int dir)
{
    bmp_draw_to_idle(0);
    int dur = anim_ms(170);
    uint8_t *real = bmp_vram_real();
    uint8_t *idle = bmp_vram_idle();
    if (!dir || !dur || !real || !idle || key_pending())
    {
        bmp_idle_copy(1, 0);
        return;
    }

    const int y0 = 47, y1 = 444;   /* intestazione e piede cambiano subito */
    for (int y = 0; y < SCR_H; y++)
        if (y < y0 || y >= y1) memcpy(real + y * BMPPITCH, idle + y * BMPPITCH, SCR_W);

    int t0 = get_ms_clock();
    int shown = 0;
    while (shown < SCR_W)
    {
        if (key_pending()) break;           /* un tasto premuto conta piu' dell'animazione */
        int t = get_ms_clock() - t0;
        int target = SCR_W * ease(t, dur) / 1000;
        if (t >= dur) target = SCR_W;
        int d = target - shown;
        if (d <= 0) { msleep(2); continue; }
        for (int y = y0; y < y1; y++)
        {
            uint8_t *r = real + y * BMPPITCH;
            uint8_t *i = idle + y * BMPPITCH;
            if (dir > 0)
            {
                memmove(r, r + d, SCR_W - d);
                memcpy(r + SCR_W - d, i + shown, d);
            }
            else
            {
                memmove(r + d, r, SCR_W - d);
                memcpy(r, i + SCR_W - target, d);
            }
        }
        shown = target;
        msleep(1);
    }
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

static void draw_footer(const char *hints)
{
    fill(C_BG, 0, 444, SCR_W, 36);
    fill(C_LINE, 16, 444, SCR_W - 32, 1);
    if (msg[0] && get_ms_clock() < msg_until)
    {
        fill(msg_color == C_WARN ? C_WARN : C_ACCENT, 16, 452, 6, 22);
        txt(FONT_MED, msg_color, 30, 450, msg);
    }
    else txt(FONT_MED, C_DIM, 16, 450, hints);
}

/* ------------------------------------------------------------------ elementi */

static void draw_pill(int on, int xr, int y)
{
    int w = 64, h = 26, x = xr - w;
    fill(on ? C_ACCENT : C_LINE, x, y, w, h);
    fill(on ? C_TEXT : C_BG, x + (on ? w - 30 : 4), y + 4, 26, h - 8);
    txt(FONT_SMALL, on ? C_BG : C_DIM, on ? x + 8 : x + 34, y + 7, on ? "ON" : "OFF");
}

static void draw_progress(int x, int y, int w, int h, int permille)
{
    fill(C_LINE, x, y, w, h);
    fill(C_ACCENT, x, y, w * COERCE(permille, 0, 1000) / 1000, h);
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

/* Astro: Via Lattea, costellazione, falce di luna, montagne con treppiede. Area 190 x 100. */
static void icon_sky(int x, int y, int bg)
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
    for (int i = 0; i < 6; i++)
        star(x + cs[i][0], y + cs[i][1], i == 2 ? 3 : i == 5 ? 2 : 1, tc(i == 2 ? COLOR_YELLOW : i == 4 ? COLOR_LIGHT_BLUE : COLOR_WHITE));
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

/* triangolo pieno (righe orizzontali) */
static void fill_tri(int x0, int y0, int x1, int y1, int x2, int y2, int c)
{
    int ymin = MIN(y0, MIN(y1, y2)), ymax = MAX(y0, MAX(y1, y2));
    for (int yy = ymin; yy <= ymax; yy++)
    {
        int xa = 10000, xb = -10000;
        int px[3] = { x0, x1, x2 }, py[3] = { y0, y1, y2 };
        for (int e = 0; e < 3; e++)
        {
            int ax = px[e], ay = py[e], bx = px[(e + 1) % 3], by = py[(e + 1) % 3];
            if ((yy < ay && yy < by) || (yy > ay && yy > by)) continue;
            int xx = ay == by ? ax : ax + (bx - ax) * (yy - ay) / (by - ay);
            if (ay == by) { xa = MIN(xa, MIN(ax, bx)); xb = MAX(xb, MAX(ax, bx)); }
            else { xa = MIN(xa, xx); xb = MAX(xb, xx); }
        }
        if (xb >= xa) fill(c, xa, yy, xb - xa + 1, 1);
    }
}

/* Foto: reflex vista di fronte. Area ~150 x 100, (x, y) = angolo in alto a sinistra del corpo */
static void icon_dslr(int x, int y, int bg)
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
    fill_circle(cx, cy, 16, COLOR_BLACK);
    fill_circle(cx - 3, cy - 3, 9, tc(COLOR_BLUE));
    fill_circle(cx - 4, cy - 4, 6, COLOR_BLACK);
    fill_circle(cx - 8, cy - 9, 3, tc(COLOR_LIGHT_BLUE));
    fill(tc(COLOR_WHITE), cx - 9, cy - 10, 2, 2);
    fill(tc(COLOR_GRAY(50)), cx + 7, cy + 8, 3, 2);
    (void)bg;
}

/* Torna alla 6D: freccia circolare spessa (antioraria) con una piccola reflex al centro */
static void icon_undo(int x, int y)
{
    int c = C_ACCENT;
    const float a0 = 1.05f, a1 = 5.70f;                 /* da 60 a 327 gradi */
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

/* Profili: tre cursori verticali da mixer, centro (x, y) */
static void icon_faders(int x, int y)
{
    static const signed char pos[3] = { 8, -10, 2 };
    for (int i = 0; i < 3; i++)
    {
        int fx = x - 15 + i * 15;
        fill(C_LINE, fx - 2, y - 22, 5, 44);
        fill(COLOR_BLACK, fx, y - 21, 1, 42);
        int ky = y + pos[i];
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

static void icon_gears(int x, int y, int bg)
{
    gear(x - 3, y + 2, 15, 10, 0.0f, C_ACCENT, bg);
    gear(x + 18, y - 17, 8, 7, 0.25f, tc(COLOR_GRAY(70)), bg);
}

/* "IRIDE" che si apre come un diaframma */
static void splash()
{
    int dur = anim_ms(520);
    int t0 = get_ms_clock();
    for (;;)
    {
        int t = get_ms_clock() - t0;
        if (t > dur || key_pending()) break;
        int e = ease(t, dur);
        frame_begin();
        fill(C_BG, 0, 0, SCR_W, SCR_H);
        for (int i = 0; i < 6; i++)
        {
            int r = (20 + i * 16) * e / 1000;
            if (r > 2) draw_circle(360, 200, r, i % 2 ? C_LINE : C_ACCENT);
        }
        const char *word = "IRIDE";
        char part[8];
        int n = COERCE(e * 6 / 1000, 0, 5);
        memcpy(part, word, n);
        part[n] = 0;
        txt_c(FONT_LARGE, C_TEXT, 360, 320, part);
        if (e > 800) txt_c(FONT_MED, C_DIM, 360, 362, "Canon EOS 6D");
        frame_show(0);
        msleep(10);
    }
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

/* righe etichetta + valore, con barra di selezione animata */
static void draw_form(const struct form *f, int s)
{
    int y0 = f->y0 ? f->y0 : 58;
    int rh = f->row_h ? f->row_h : 42;
    int yh = hl_y(y0 + s * rh);
    fill(C_SEL, 24, yh, SCR_W - 48, rh - 4);
    fill(C_ACCENT, 24, yh, 6, rh - 4);
    for (int i = 0; i < f->n; i++)
    {
        int y = y0 + i * rh;
        char buf[64] = "";
        int kind = VK_TEXT;
        if (f->value) f->value(i, buf, sizeof(buf), &kind);
        int ty = y + (rh - 4 - 23) / 2;
        txt(FONT_MED, kind == VK_INFO ? C_DIM : C_TEXT, 44, ty, f->labels[i]);
        if (kind == VK_ON || kind == VK_OFF)
            draw_pill(kind == VK_ON, SCR_W - 44, y + (rh - 4 - 26) / 2);
        else if (kind == VK_ACTION)
            txt_r(FONT_MED, C_ACCENT, SCR_W - 44, ty, buf[0] ? buf : "SET");
        else if (buf[0])
            txt_r(FONT_MED, kind == VK_WARN ? C_WARN : (i == s ? C_ACCENT : C_TEXT), SCR_W - 44, ty, buf);
    }
}

/* voci grandi con titolo e sottotitolo (menu Astro e Foto) */
struct menu_row { const char *title, *sub; int target; };

static void draw_menu_rows(const struct menu_row *rows, int n, int s, int rh)
{
    int y0 = 56;
    int yh = hl_y(y0 + s * rh);
    for (int i = 0; i < n; i++) fill(C_PANEL, 24, y0 + i * rh, SCR_W - 48, rh - 8);
    fill(C_SEL, 24, yh, SCR_W - 48, rh - 8);
    fill(C_ACCENT, 24, yh, 6, rh - 8);
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
