/* icons.inc - icone dei sottomenu Astro e Foto, disegnate in scala.
 * Ogni icona sta in un quadrato di lato s centrato in (cx, cy); t >= 0 e' il tempo (ms)
 * dell'animazione che parte quando la voce viene scelta, t < 0 = ferma. */

#define K(v) ((v) * s / 100)

/* avanzamento 0..1000 di una fase dell'animazione dell'icona */
static int ip(int t, int a, int d, int kind) { return t < 0 ? 1000 : ease_k(t - a, d, kind); }

/* --- Astro ------------------------------------------------------------------ */

/* Via Lattea: fascia di stelle in diagonale, stelle brillanti, montagne */
static void ic_milkyway(int cx, int cy, int s, int t)
{
    int x0 = cx - K(50), y0 = cy - K(50);
    icon_rng = 11;
    for (int i = 0; i < 26 + s / 3; i++)
    {
        int u = irand(100);
        int spread = 16 - ABS(u - 50) / 5;
        int px = x0 + K(u), py = y0 + K(78 - u * 62 / 100 + irand(2 * spread + 1) - spread);
        fill(tc(irand(3) ? COLOR_GRAY(50) : COLOR_WHITE), px, py, 1 + (s > 80 && irand(4) == 0), 1);
    }
    /* stelle brillanti che si accendono una dopo l'altra */
    static const signed char st[3][2] = { { 22, 22 }, { 70, 16 }, { 82, 46 } };
    for (int i = 0; i < 3; i++)
    {
        int e = ip(t, i * 120, 260, EZ_BACK);
        int r = MAX(1, K(3) * e / 1000);
        if (e > 0) star(x0 + K(st[i][0]), y0 + K(st[i][1]), r, tc(i == 1 ? COLOR_YELLOW : COLOR_WHITE));
    }
    /* montagne */
    int base = y0 + K(100);
    for (int x = 0; x < 100; x += 2)
    {
        int h = MAX(MAX(26 - ABS(x - 28) * 7 / 10, 18 - ABS(x - 74) * 6 / 10), 6);
        fill(tc(COLOR_GRAY(16)), x0 + K(x), base - K(h), MAX(K(2), 1), K(h));
    }
}

/* Intervallometro: cronometro con la lancetta che fa un giro */
static void ic_interval(int cx, int cy, int s, int t)
{
    int r = K(36), yc = cy + K(6);
    fill(C_ACCENT, cx - K(6), yc - r - K(12), K(12), K(7));
    fill(C_DIM, cx - K(2), yc - r - K(6), K(4), K(6));
    disc(cx, yc, r, C_DIM);
    disc(cx, yc, r - MAX(K(4), 2), C_BG);
    for (int i = 0; i < 12; i++)
    {
        float a = (float)i * 0.5236f;
        int r1 = r - K(i % 3 ? 9 : 13), r2 = r - K(6);
        draw_line(cx + (int)((float)r1 * sinf(a)), yc - (int)((float)r1 * cosf(a)),
                  cx + (int)((float)r2 * sinf(a)), yc - (int)((float)r2 * cosf(a)), i % 3 ? C_LINE : C_TEXT);
    }
    float a = 0.9f + (t >= 0 ? (float)ip(t, 0, 900, EZ_IO) * 0.0062832f : 0.0f);
    int hx = cx + (int)((float)(r - K(10)) * sinf(a)), hy = yc - (int)((float)(r - K(10)) * cosf(a));
    draw_line(cx, yc, hx, hy, C_ACCENT);
    draw_line(cx + 1, yc, hx + 1, hy, C_ACCENT);
    disc(cx, yc, MAX(K(4), 2), C_ACCENT);
}

/* Fuoco sulle stelle: una stella sfocata (anelli) che diventa un punto nitido */
static void ic_starfocus(int cx, int cy, int s, int t)
{
    int e = ip(t, 0, 800, EZ_OUT);
    int blur = K(30) * (1000 - e) / 1000;
    for (int k = 3; k >= 1; k--)
    {
        int r = K(8) + blur * k / 3 + K(6) * k;
        draw_circle(cx, cy, r, k == 1 ? C_DIM : C_LINE);
    }
    star(cx, cy, MAX(K(5), 2), tc(COLOR_WHITE));
    /* staffe del riquadro di misura */
    int b = K(40), l = K(12);
    int c = C_ACCENT;
    fill(c, cx - b, cy - b, l, 2); fill(c, cx - b, cy - b, 2, l);
    fill(c, cx + b - l, cy - b, l, 2); fill(c, cx + b - 2, cy - b, 2, l);
    fill(c, cx - b, cy + b - 2, l, 2); fill(c, cx - b, cy + b - l, 2, l);
    fill(c, cx + b - l, cy + b - 2, l, 2); fill(c, cx + b - 2, cy + b - l, 2, l);
}

/* Allineamento polare: reticolo con la Polare che gira sul suo cerchio */
static void ic_polar(int cx, int cy, int s, int t)
{
    int R = K(42), rp = K(28);
    draw_circle(cx, cy, R, C_DIM);
    draw_circle(cx, cy, rp, C_LINE);
    fill(C_LINE, cx - R + K(6), cy, 2 * R - K(12), 1);
    fill(C_LINE, cx, cy - R + K(6), 1, 2 * R - K(12));
    for (int i = 0; i < 12; i++)
    {
        float a = (float)i * 0.5236f;
        draw_line(cx + (int)((float)(R - K(6)) * sinf(a)), cy - (int)((float)(R - K(6)) * cosf(a)),
                  cx + (int)((float)R * sinf(a)), cy - (int)((float)R * cosf(a)), C_DIM);
    }
    float a = 0.5f + (t >= 0 ? (float)ip(t, 0, 1000, EZ_IO) * 0.0062832f : 0.0f);
    int px = cx - (int)((float)rp * sinf(a)), py = cy - (int)((float)rp * cosf(a));
    disc(px, py, MAX(K(6), 3), C_ACCENT);
    disc(px, py, MAX(K(2), 1), C_BG);
}

/* Calendario del cielo: Luna che cambia fase davanti a un calendario */
static void ic_calendar(int cx, int cy, int s, int t)
{
    int x0 = cx - K(14), y0 = cy - K(30), w = K(58), h = K(62);
    fill(C_LINE, x0, y0, w, h);
    fill(C_ACCENT, x0, y0, w, K(12));
    fill(C_PANEL, x0 + 2, y0 + K(12) + 2, w - 4, h - K(12) - 4);
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 4; c++)
            fill(r == 1 && c == 2 ? C_ACCENT : C_LINE, x0 + K(6) + c * K(13), y0 + K(20) + r * K(13), MAX(K(7), 2), MAX(K(7), 2));
    /* Luna: la parte illuminata cresce */
    int mr = K(22), mx = cx - K(22), my = cy + K(12);
    int ill = t >= 0 ? ip(t, 0, 900, EZ_IO) * 7 / 10 : 700;
    disc(mx, my, mr + 2, C_BG);
    draw_moon(mx, my, mr, ill, 1, mr >= 12 ? 0 : -1);
}

/* Dove puntare la Via Lattea: orizzonte, arco della Via Lattea e il centro che sale */
static void ic_mwplan(int cx, int cy, int s, int t)
{
    int x0 = cx - K(46), x1 = cx + K(46), base = cy + K(30);
    /* l'arco: una fascia di puntini curva */
    icon_rng = 5;
    int e = ip(t, 0, 900, EZ_IO);
    for (int i = 0; i < 40 + s / 2; i++)
    {
        int u = irand(100);
        int px = x0 + K(u), py = base - K(10 + (50 - ABS(u - 50)) * 9 / 10) + irand(MAX(K(10), 2)) - K(5);
        fill(tc(irand(3) ? COLOR_GRAY(46) : COLOR_WHITE), px, py, 1, 1);
    }
    /* il centro: sale lungo l'arco con l'animazione */
    int u = 20 + 40 * e / 1000;
    int gx = x0 + K(u), gy = base - K(10 + (50 - ABS(u - 50)) * 9 / 10);
    disc(gx, gy, MAX(K(6), 2), C_OK);
    /* orizzonte e punti cardinali */
    fill(C_DIM, x0, base, x1 - x0, MAX(K(2), 1));
    for (int i = 0; i < 3; i++) fill(C_LINE, x0 + K(10 + i * 40), base + K(4), 1, MAX(K(6), 2));
    if (s >= 60) txt_c(FONT_SMALL, C_DIM, cx, base + K(10), "S");
}

/* Luna da fotografare: la Luna con il riquadro di messa a fuoco */
static void ic_moonshot(int cx, int cy, int s, int t)
{
    int r = K(30);
    int ill = t >= 0 ? 300 + ip(t, 0, 900, EZ_IO) * 6 / 10 : 800;
    disc(cx, cy, r + 2, C_BG);
    draw_moon(cx, cy, r, ill, 1, r >= 12 ? 0 : -1);
    /* angoli del riquadro */
    int q = K(44), l = K(12);
    int c = C_ACCENT;
    fill(c, cx - q, cy - q, l, 2); fill(c, cx - q, cy - q, 2, l);
    fill(c, cx + q - l, cy - q, l, 2); fill(c, cx + q - 2, cy - q, 2, l);
    fill(c, cx - q, cy + q - 2, l, 2); fill(c, cx - q, cy + q - l, 2, l);
    fill(c, cx + q - l, cy + q - 2, l, 2); fill(c, cx + q - 2, cy + q - l, 2, l);
}

/* Etichetta file: un foglio con l'angolo piegato e la targhetta che entra */
static void ic_tag(int cx, int cy, int s, int t)
{
    int x0 = cx - K(28), y0 = cy - K(38), w = K(50), h = K(70), f = K(14);
    fill(C_DIM, x0, y0, w - f, h);
    fill(C_DIM, x0, y0 + f, w, h - f);
    fill_tri(x0 + w - f, y0, x0 + w - f, y0 + f, x0 + w, y0 + f, C_LINE);
    for (int i = 0; i < 4; i++) fill(C_LINE, x0 + K(8), y0 + K(28) + i * K(10), w - K(18), MAX(K(3), 1));
    int e = ip(t, 0, 500, EZ_BACK);
    int tx = cx - K(4) + K(40) * (1000 - e) / 1000, ty = cy + K(10);
    fill(C_ACCENT, tx, ty, K(44), K(20));
    fill_tri(tx - K(10), ty + K(10), tx, ty, tx, ty + K(20), C_ACCENT);
    disc(tx + K(2), ty + K(10), MAX(K(3), 1), C_BG);
    if (s >= 80) txt(FONT_SMALL, C_BG, tx + K(9), ty + K(10) - 6, "LGHT");
}

/* --- Foto ------------------------------------------------------------------- */

/* HDR: tre fotogrammi a esposizioni diverse che si aprono a ventaglio */
static void ic_hdr(int cx, int cy, int s, int t)
{
    int e = ip(t, 0, 600, EZ_BACK);
    static const unsigned char lum[3] = { 10, 45, 90 };
    for (int i = 0; i < 3; i++)
    {
        int d = (i - 1) * K(22) * e / 1000;
        int x = cx - K(26) + d, y = cy - K(20) - d / 2;
        int c = night ? (i == 2 ? COLOR_RED : COLOR_DARK_RED) : COLOR_GRAY(lum[i]);
        fill(C_BG, x - 2, y - 2, K(52) + 4, K(40) + 4);
        fill(c, x, y, K(52), K(40));
        frame_rect(i == 1 ? C_ACCENT : C_DIM, x, y, K(52), K(40), 1);
    }
}

/* Focus stacking: piani di fuoco uno dietro l'altro, il piano a fuoco scorre */
static void ic_fstack(int cx, int cy, int s, int t)
{
    int cur = t >= 0 ? MIN(ip(t, 0, 900, EZ_IO) * 4 / 1000, 3) : 1;
    for (int i = 3; i >= 0; i--)
    {
        int x = cx - K(36) + i * K(14), y = cy - K(30) + i * K(10);
        int px[4] = { x, x + K(44), x + K(30), x - K(14) };
        int py[4] = { y, y, y + K(30), y + K(30) };
        fill_poly(px, py, 4, i == cur ? C_ACCENT : C_PANEL2);
        draw_line(px[0], py[0], px[1], py[1], i == cur ? C_ACCENT : C_DIM);
        draw_line(px[3], py[3], px[2], py[2], i == cur ? C_ACCENT : C_DIM);
        draw_line(px[0], py[0], px[3], py[3], i == cur ? C_ACCENT : C_DIM);
        draw_line(px[1], py[1], px[2], py[2], i == cur ? C_ACCENT : C_DIM);
    }
}

/* Dual ISO: righe del sensore a due luminosita' che si scoprono */
static void ic_dual(int cx, int cy, int s, int t)
{
    int x0 = cx - K(36), y0 = cy - K(36), w = K(72), h = K(72);
    int e = ip(t, 0, 600, EZ_OUT);
    int n = 8;
    for (int i = 0; i < n; i++)
    {
        int y = y0 + i * h / n, hh = (i + 1) * h / n - i * h / n;
        int ww = w * MIN(1000, MAX(0, e * 2 - i * 120)) / 1000;
        if (t < 0) ww = w;
        int c = i % 4 < 2 ? (night ? COLOR_DARK_RED : COLOR_GRAY(24)) : (night ? COLOR_RED : COLOR_GRAY(72));
        fill(c, x0, y, ww, hh);
    }
    frame_rect(C_DIM, x0 - 1, y0 - 1, w + 2, h + 2, 1);
}

/* ETTR: istogramma che scivola verso destra fino al limite */
static void ic_ettr(int cx, int cy, int s, int t)
{
    static const unsigned char hb[9] = { 10, 22, 38, 56, 70, 76, 64, 40, 16 };
    int e = ip(t, 0, 700, EZ_BACK);
    int shift = K(-22) * (1000 - e) / 1000;
    int base = cy + K(32), x0 = cx - K(40);
    for (int i = 0; i < 9; i++)
    {
        int x = x0 + K(10) + i * K(7) + shift, h = K(hb[i]) * 8 / 10;
        if (x >= x0) fill(i >= 6 ? C_ACCENT : C_DIM, x, base - h, MAX(K(6), 2), h);
    }
    fill(C_LINE, x0, base, K(80), 2);
    fill(C_WARN, x0 + K(80), base - K(66), 2, K(66));
}

/* Movimento: fulmine che lampeggia */
static void ic_motion(int cx, int cy, int s, int t)
{
    int flash = t >= 0 && ((t > 80 && t < 170) || (t > 260 && t < 330));
    int c = flash ? tc(COLOR_WHITE) : C_ACCENT;
    if (flash) { draw_circle(cx, cy, K(40), C_DIM); draw_circle(cx, cy, K(46), C_LINE); }
    fill_tri(cx + K(4), cy - K(40), cx - K(20), cy + K(6), cx - K(2), cy + K(6), c);
    fill_tri(cx + K(2), cy - K(4), cx + K(20), cy - K(4), cx - K(8), cy + K(42), c);
}

/* Strumenti del Live View: mirino con le zebre che scorrono e un fuoco evidenziato */
static void ic_lvtools(int cx, int cy, int s, int t)
{
    int b = K(40), l = K(14);
    int x0 = cx - b, y0 = cy - K(30), w = 2 * b, h = K(60);
    int sh = t >= 0 ? ip(t, 0, 900, EZ_OUT) * K(16) / 1000 : 0;
    for (int i = -6; i < 10; i++)
    {
        int xa = x0 + i * K(12) + sh;
        for (int k = 0; k < h; k += 2)
        {
            int x = xa + k / 2;
            if (x >= x0 + K(30) && x < x0 + w - 2) fill(i % 2 ? C_LINE : C_DIM, x, y0 + k, 2, 2);
        }
    }
    /* soggetto con il peaking */
    disc(x0 + K(18), cy, K(10), C_PANEL2);
    draw_circle(x0 + K(18), cy, K(10), tc(COLOR_RED));
    draw_circle(x0 + K(18), cy, K(11), tc(COLOR_RED));
    int c = C_TEXT;
    fill(c, x0, y0, l, 2); fill(c, x0, y0, 2, l);
    fill(c, x0 + w - l, y0, l, 2); fill(c, x0 + w - 2, y0, 2, l);
    fill(c, x0, y0 + h - 2, l, 2); fill(c, x0, y0 + h - l, 2, l);
    fill(c, x0 + w - l, y0 + h - 2, l, 2); fill(c, x0 + w - 2, y0 + h - l, 2, l);
}

/* Filtri ND: il filtro scuro scivola davanti all'obiettivo */
static void ic_nd(int cx, int cy, int s, int t)
{
    int R = K(34);
    disc(cx, cy, R, C_DIM);
    disc(cx, cy, R - K(6), tc(COLOR_BLUE));
    disc(cx - K(8), cy - K(8), K(8), tc(COLOR_LIGHT_BLUE));
    int e = ip(t, 0, 650, EZ_BACK);
    int fx = cx + K(18) + K(40) * (1000 - e) / 1000;
    disc(fx, cy, R - K(2), C_LINE);
    disc(fx, cy, R - K(7), night ? COLOR_BLACK : COLOR_GRAY(10));
    disc(fx, cy, R - K(16), night ? COLOR_BLACK : COLOR_GRAY(16));
    if (s >= 80) txt_c(FONT_SMALL, C_DIM, fx, cy - 6, "ND");
}

#undef K
