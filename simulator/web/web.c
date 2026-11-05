/* web.c - Iride 6D nel browser.
 *
 * Lo stesso codice di Iride (module/iride) e lo stesso simulatore dei test, compilati in
 * WebAssembly con Emscripten. Qui si aggiunge solo quello che serve per usarlo dal vivo:
 *  - il testo con i caratteri veri di Magic Lantern (fonts_ml.h, dai file RBF), linee e cerchi
 *    scritti nella memoria video, come sulla 6D;
 *  - il tempo vero (orologio e data del computer: Luna, cielo e Polare sono quelli di adesso);
 *  - i tasti, presi dalla pagina e passati a Iride come li passa Magic Lantern;
 *  - i "task" (Iride, Live View, intervallometro) eseguiti uno dopo l'altro;
 *  - una schermata di scatto finta quando Iride e' chiusa.
 * Viene incluso in fondo a preview.c quando si compila con -DIRIDE_WEB (vedi web/build.sh). */

#include <emscripten.h>
#include "fonts_ml.h"

/* ------------------------------------------------------------------ disegno */

static inline void web_put(uint8_t *b, int x, int y, int c)
{
    if ((unsigned)x < 720 && (unsigned)y < 480) b[y * 960 + x] = (uint8_t)c;
}

int web_str_width(int id, const char *s)
{
    const struct ml_font *f = &ml_fonts[id < 0 || id > 4 ? 2 : id];
    int w = 0;
    for (; *s; s++)
    {
        int c = (unsigned char)*s;
        if (c >= f->first && c <= f->last) w += f->w[c - f->first];
    }
    return w;
}

void web_text(int fontspec, int x, int y, const char *s)
{
    int id = (fontspec >> 16) & 7;
    const struct ml_font *f = &ml_fonts[id > 4 ? 2 : id];
    int fg = fontspec & 0xFF, bg = (fontspec >> 8) & 0xFF;
    uint8_t *b = bmp_vram();
    int bpr = f->width / 8;                              /* byte per riga del carattere */
    for (; *s; s++)
    {
        int c = (unsigned char)*s;
        if (c == '\n') { y += f->height; continue; }
        if (c < f->first || c > f->last) continue;
        int w = f->w[c - f->first];
        const unsigned char *g = f->c + (c - f->first) * f->charsize;
        if (bg != 0xFF) bmp_fill(bg, x, y, w, f->height);      /* come font_draw_char di ML */
        for (int yy = 0; yy < f->height; yy++)
            for (int xx = 0; xx < w; xx++)
                if (g[yy * bpr + xx / 8] & (1 << (xx % 8))) web_put(b, x + xx, y + yy, fg);
        x += w;
    }
}

void web_line(int x1, int y1, int x2, int y2, int c)
{
    uint8_t *b = bmp_vram();
    int steep = ABS(y2 - y1) > ABS(x2 - x1), t;
    if (steep) { t = x1; x1 = y1; y1 = t; t = x2; x2 = y2; y2 = t; }
    if (x1 > x2) { t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t; }
    int dx = x2 - x1, dy = ABS(y2 - y1), err = 0, y = y1, ys = y1 < y2 ? 1 : -1;
    for (int x = x1; x <= x2; x++)
    {
        if (steep) web_put(b, y, x, c); else web_put(b, x, y, c);
        err += dy;
        if (2 * err >= dx) { y += ys; err -= dx; }
    }
}

void web_circle(int x, int y, int r, int c)
{
    uint8_t *b = bmp_vram();
    int dx = 0, dy = r, p = 3 - 2 * r;
    do
    {
        web_put(b, x + dx, y + dy, c); web_put(b, x + dy, y + dx, c); web_put(b, x + dy, y - dx, c); web_put(b, x + dx, y - dy, c);
        web_put(b, x - dx, y - dy, c); web_put(b, x - dy, y - dx, c); web_put(b, x - dy, y + dx, c); web_put(b, x - dx, y + dy, c);
        ++dx;
        if (p < 0) p += 4 * dx + 6; else { --dy; p += 4 * (dx - dy) + 10; }
    } while (dx <= dy);
}

void web_fill_circle(int x, int y, int r, int c) { disc(x, y, r, c); }

void web_rect(int c, int x, int y, int w, int h)
{
    web_line(x, y, x + w - 1, y, c); web_line(x, y + h - 1, x + w - 1, y + h - 1, c);
    web_line(x, y, x, y + h - 1, c); web_line(x + w - 1, y, x + w - 1, y + h - 1, c);
}

/* ------------------------------------------------------------------ valori come li scrive la 6D */

/* diaframma: codice Canon (8 per stop da f/1.0) -> f in decimi, sui valori nominali a terzi */
int web_raw2ap(int raw)
{
    static const short f[] = { 10, 11, 12, 14, 16, 18, 20, 22, 25, 28, 32, 35, 40, 45, 50, 56, 63, 71, 80, 90, 100, 110, 130, 140, 160, 180, 200, 220, 250, 290, 320 };
    if (raw <= 0) return 0;
    int k = (int)floorf((float)(raw - 8) * 3.0f / 8.0f + 0.5f);
    return f[COERCE(k, 0, (int)COUNT(f) - 1)];
}

/* tempo: codice Canon (8 per stop, 56 = 1 s) -> testo nominale come sullo schermo della 6D */
const char *web_fmt_shutter(int raw)
{
    /* terzi di stop da 1 s: indice 6 = 1/4, 18 = 1/60, 36 = 1/4000 (come i valori della 6D) */
    static const short den[] = { 1, 1, 1, 1, 1, 1, 4, 5, 6, 8, 10, 13, 15, 20, 25, 30, 40, 50, 60, 80, 100, 125, 160, 200, 250, 320, 400, 500, 640, 800, 1000, 1250, 1600, 2000, 2500, 3200, 4000 };
    static const char *const slow[] = { "1\"", "1\"3", "1\"6", "2\"", "2\"5", "3\"2", "4\"", "5\"", "6\"", "8\"", "10\"", "13\"", "15\"", "20\"", "25\"", "30\"" };
    static char b[16];
    if (raw <= 12) return "BULB";
    int k = (int)floorf((float)(raw - 56) * 3.0f / 8.0f + 0.5f);
    if (k >= 6) { snprintf(b, sizeof(b), "1/%d", den[COERCE(k, 0, (int)COUNT(den) - 1)]); return b; }
    if (k > 0) { static const char *const n[] = { "", "0\"8", "0\"6", "0\"5", "0\"4", "0\"3" }; return n[k]; }
    return slow[COERCE(-k, 0, (int)COUNT(slow) - 1)];
}

/* ------------------------------------------------------------------ tempo */

EM_JS(double, js_now, (), { return performance.now(); });
int web_ms(void) { return (int)js_now(); }

EM_JS(void, js_date, (int *out), {
    const d = new Date();
    HEAP32[(out >> 2) + 0] = d.getFullYear() - 1900; HEAP32[(out >> 2) + 1] = d.getMonth();
    HEAP32[(out >> 2) + 2] = d.getDate(); HEAP32[(out >> 2) + 3] = d.getHours();
    HEAP32[(out >> 2) + 4] = d.getMinutes(); HEAP32[(out >> 2) + 5] = d.getSeconds();
    HEAP32[(out >> 2) + 6] = d.getDay();
});

void web_now(struct tm *t)
{
    int v[7];
    js_date(v);
    memset(t, 0, sizeof(*t));
    t->tm_year = v[0]; t->tm_mon = v[1]; t->tm_mday = v[2]; t->tm_hour = v[3]; t->tm_min = v[4]; t->tm_sec = v[5]; t->tm_wday = v[6];
}

/* ------------------------------------------------------------------ schermo */

EM_JS(void, js_present, (const uint8_t *vram), { if (Module.iridePresent) Module.iridePresent(vram); });
EM_JS(int, js_pop_key, (), { return Module.iridePopKey ? Module.iridePopKey() : -1; });
EM_JS(void, js_state, (int open, int live, int screen_id), { if (Module.irideState) Module.irideState(open, live, screen_id); });

static int web_last_present = 0, web_last_tick = 0, web_in_tick = 0;
static void web_deliver(int key);

static void web_present(void)
{
    js_present(vram_a + 30 * 960);
    js_state(app_running, lv, screen);
}

static void web_shooting_screen(void);
static int web_last_shoot_draw = 0;

void web_sleep(int ms)
{
    int now = web_ms();
    /* Iride chiusa (anche mentre un task aspetta, per esempio il Live View che si accende):
       la schermata di scatto, come sulla 6D */
    if (!app_running && job_running == JOB_NONE && now - web_last_shoot_draw > 150)
    {
        web_last_shoot_draw = now;
        web_shooting_screen();
    }
    if (now - web_last_present >= 16) { web_last_present = now; web_present(); }
    emscripten_sleep(ms > 0 ? ms : 1);
    int k;
    while ((k = js_pop_key()) >= 0) web_deliver(k);
    /* il "secondo" di Magic Lantern: contatore del rullino e lavori su scheda */
    now = web_ms();
    if (now - web_last_tick >= 1000 && !web_in_tick)
    {
        web_last_tick = now;
        web_in_tick = 1;
        iride_clock(0);
        iride_bg_tick();
        web_in_tick = 0;
    }
}

/* ------------------------------------------------------------------ task */

struct web_task_t { void (*f)(void *); void *arg; };
static struct web_task_t web_q[16];
static int web_qh = 0, web_qt = 0;

void web_task(const char *name, void (*f)(void *), void *arg)
{
    if (streq(name, "iride_bg")) return;               /* gira gia' dentro web_sleep, ogni secondo */
    int n = (web_qh + 1) % 16;
    if (n == web_qt) return;
    web_q[web_qh].f = f; web_q[web_qh].arg = arg; web_qh = n;
}

/* ------------------------------------------------------------------ tasti */

/* codici dei tasti della 6D per il gestore "grezzo" di Iride (platform/6D.116/gui.h) */
static int web_raw_param(int key)
{
    switch (key)
    {
        case MODULE_KEY_TRASH: return 0x0d;
        case MODULE_KEY_MENU: return 0x06;
        case MODULE_KEY_INFO: return 0x07;
        case MODULE_KEY_Q: return 0x1d;
        case MODULE_KEY_LV: return 0x1e;
        case MODULE_KEY_PRESS_SET: return 0x04;
        case MODULE_KEY_PRESS_HALFSHUTTER: return 0x50;
        case 100: return BGMT_LIGHT_6D;               /* tasto luce: solo evento grezzo */
    }
    return 0x2e;
}

static int web_canon_msg_until = 0;
static const char *web_canon_msg = "";

static void web_deliver(int key)
{
    static struct event ev;
    memset(&ev, 0, sizeof(ev));
    ev.param = web_raw_param(key);
    key_fake = 0;
    if (iride_keypress_raw((unsigned int)(uintptr_t)&ev) == 0) return;   /* tasto luce: Iride aperta */
    if (key == 100) return;
    int pass = iride_keypress(key);
    if (!pass) return;
    /* passa a Canon: quello che farebbe la 6D */
    if (key == MODULE_KEY_PRESS_HALFSHUTTER && menu_open) { gui_stop_menu(); return; }
    if (!app_running)
    {
        if (key == MODULE_KEY_LV || key == MODULE_KEY_REC) { lv = !lv; return; }
        if (key == MODULE_KEY_MENU) { web_canon_msg = "Il menu Canon non e' nella demo"; web_canon_msg_until = web_ms() + 2500; }
        if (key == MODULE_KEY_TRASH) { web_canon_msg = "Il menu di Magic Lantern non e' nella demo"; web_canon_msg_until = web_ms() + 2500; }
        if (key == MODULE_KEY_PRESS_SET) { web_canon_msg = "SET: in Opzioni puoi farlo aprire il bianco"; web_canon_msg_until = web_ms() + 2500; }
        if (key == MODULE_KEY_PLAY) { web_canon_msg = "La riproduzione delle foto non e' nella demo"; web_canon_msg_until = web_ms() + 2500; }
    }
}

/* scambio con la pagina: aggiunge un tasto alla coda */
EMSCRIPTEN_KEEPALIVE int web_vram_ptr(void) { return (int)(uintptr_t)(vram_a + 30 * 960); }

/* ------------------------------------------------------------------ schermata di scatto */

static void web_shooting_screen(void)
{
    to_idle = 0;
    uint8_t *b = bmp_vram();
    for (int y = 0; y < 480; y++) memset(b + y * 960, lv ? 2 : COLOR_GRAY(6), 720);
    if (lv)
    {
        /* "Live View": un cielo con qualche stella */
        for (int i = 0; i < 160; i++)
        {
            unsigned h = (unsigned)i * 2654435761u;
            int x = (int)(h % 720), y = (int)((h >> 10) % 400) + 40;
            web_put(b, x, y, i % 7 ? COLOR_GRAY(20 + i % 20) : COLOR_WHITE);
        }
    }
    char line[96];
    int a10 = web_raw2ap(lens_info.raw_aperture);
    snprintf(line, sizeof(line), "%s   %s   f/%d.%d   ISO %s", get_shootmode_name_short(shooting_mode),
        lens_format_shutter(lens_info.raw_shutter), a10 / 10, a10 % 10, lens_info.raw_iso ? lens_format_iso(lens_info.raw_iso) : "Auto");
    web_text(FONT(FONT_LARGE, COLOR_WHITE, 0xFF), 360 - web_str_width(4, line) / 2, 150, line);
    const char *t = lv ? "Live View (simulato)" : "Schermata di scatto della 6D (simulata)";
    web_text(FONT(FONT_MED, COLOR_GRAY(30), 0xFF), 360 - web_str_width(2, t) / 2, 60, t);
    if (P.roll_loaded)
    {
        snprintf(line, sizeof(line), "Rullino R%03d  %d/%d%s", P.roll_num % 1000, P.roll_shots, P.roll_exp, P.roll_paused ? "  (pausa)" : "");
        web_text(FONT(FONT_MED, COLOR_ORANGE, 0xFF), 360 - web_str_width(2, line) / 2, 200, line);
    }
    const char *h1 = "CESTINO o LUCE LCD: apri Iride";
    const char *h2 = "LV: Live View    MEZZO SCATTO: scatta una foto";
    web_text(FONT(FONT_MED, COLOR_WHITE, 0xFF), 360 - web_str_width(2, h1) / 2, 300, h1);
    web_text(FONT(FONT_MED, COLOR_GRAY(30), 0xFF), 360 - web_str_width(2, h2) / 2, 334, h2);
    if (web_ms() < web_canon_msg_until)
        web_text(FONT(FONT_MED, COLOR_YELLOW, 0xFF), 360 - web_str_width(2, web_canon_msg) / 2, 400, web_canon_msg);
    /* i NotifyBox di Iride (rullino finito, sequenza finita...) come li mostra Magic Lantern */
    if (web_ms() < notify_until && last_notify[0])
    {
        int w = web_str_width(2, last_notify) + 24;
        bmp_fill(COLOR_BLACK, 360 - w / 2, 236, w, 36);
        web_rect(COLOR_WHITE, 360 - w / 2, 236, w, 36);
        web_text(FONT(FONT_MED, COLOR_WHITE, 0xFF), 360 - w / 2 + 12, 242, last_notify);
    }
}

/* mezzo scatto a Iride chiusa: una "foto" (il contatore Canon avanza, il rullino conta) */
EMSCRIPTEN_KEEPALIVE void web_shoot(void)
{
    if (app_running) return;
    card.file_number = card.file_number % 9999 + 1;
}

/* ------------------------------------------------------------------ avvio */

int main(void)
{
    mkdir("out", 0755); mkdir("out/DCIM", 0755); mkdir("out/DCIM/100CANON", 0755);
    ops_idle = malloc(16); ops_real = malloc(16); ops_idle[0] = ops_real[0] = 0;
    web_now(&sim_tm);
    camera_reset();
    snprintf(lens_info.name, sizeof(lens_info.name), "EF24-105mm f/4L IS USM");
    lens_info.lens_exists = 1; lens_info.focal_len = 24; lens_info.aperture = 40;
    lens_info.raw_aperture_min = 40; lens_info.raw_aperture_max = 80;      /* f/4 - f/22 */
    lens_info.iso = 400;
    menu_open = 0; app_running = 0; gui_state = 0; lv = 0;
    make_sky(1.8f, 34.0f, 360, 240);                    /* una stella per Fuoco sulle stelle */
    iride_init();
    web_last_tick = web_ms();
    for (;;)
    {
        if (web_qt != web_qh)
        {
            struct web_task_t t = web_q[web_qt];
            web_qt = (web_qt + 1) % 16;
            t.f(t.arg);
            continue;
        }
        if (!app_running && job_running == JOB_NONE) menu_open = 0;   /* la schermata la disegna web_sleep */
        web_sleep(30);
    }
    return 0;
}
