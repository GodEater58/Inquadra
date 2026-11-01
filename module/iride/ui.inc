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

static void icon_stars(int x, int y)
{
    int c1 = tc(COLOR_WHITE), c2 = tc(COLOR_YELLOW), c3 = tc(COLOR_LIGHT_BLUE), cl = C_LINE;
    draw_line(x, y, x + 40, y + 22, cl);
    draw_line(x + 40, y + 22, x + 76, y - 6, cl);
    draw_line(x + 76, y - 6, x + 110, y + 18, cl);
    fill_circle(x, y, 3, c1);
    fill_circle(x + 40, y + 22, 2, c1);
    fill_circle(x + 76, y - 6, 4, c2);
    fill_circle(x + 110, y + 18, 2, c1);
    fill_circle(x + 140, y + 2, 3, c3);
}

static void icon_camera(int x, int y)
{
    int c = tc(COLOR_GRAY(75)), d = tc(COLOR_GRAY(35));
    fill(c, x + 30, y - 10, 34, 12);          /* pentaprisma */
    fill(c, x, y, 96, 52);                    /* corpo */
    fill(d, x + 6, y + 6, 18, 8);             /* impugnatura */
    fill_circle(x + 48, y + 26, 21, d);       /* obiettivo */
    fill_circle(x + 48, y + 26, 13, tc(COLOR_BLACK));
    fill_circle(x + 52, y + 21, 4, tc(COLOR_LIGHT_BLUE));
}

static void icon_restore(int x, int y)
{
    int c = C_ACCENT;
    draw_circle(x, y, 14, c);
    draw_circle(x, y, 13, c);
    fill(C_PANEL, x - 16, y - 16, 14, 10);    /* apertura dell'anello */
    draw_line(x - 14, y - 6, x - 6, y - 12, c);
    draw_line(x - 14, y - 6, x - 16, y - 16, c);
}

static void icon_sliders(int x, int y)
{
    int c = C_ACCENT, l = C_LINE;
    for (int i = 0; i < 3; i++)
    {
        fill(l, x - 16, y - 12 + i * 12, 32, 2);
        fill_circle(x - 10 + ((i * 13) % 22), y - 11 + i * 12, 4, c);
    }
}

static void icon_gear(int x, int y)
{
    int c = C_ACCENT;
    for (int i = 0; i < 8; i++)
    {
        float a = (float)i * 0.785398f;
        int x1 = x + (int)(17.0f * cosf(a)), y1 = y + (int)(17.0f * sinf(a));
        fill_circle(x1, y1, 3, c);
    }
    fill_circle(x, y, 13, c);
    fill_circle(x, y, 6, C_PANEL);
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
