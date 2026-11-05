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

/* ------------------------------------------------------------------ salvataggio nel browser */

/* IRIDE.DAT e IRIDE.BAK della "scheda" finiscono nel localStorage: ricaricando la pagina
   rullino, profili e opzioni restano, come sulla 6D */
EM_JS(void, js_store, (const char *name, const uint8_t *p, int n), {
    try { let s = ""; for (let i = 0; i < n; i++) s += String.fromCharCode(HEAPU8[p + i]);
          localStorage.setItem("iride." + UTF8ToString(name), btoa(s)); } catch (e) {}
});
EM_JS(int, js_load, (const char *name, uint8_t *p, int max), {
    try { const v = localStorage.getItem("iride." + UTF8ToString(name)); if (!v) return 0;
          const s = atob(v); const n = Math.min(s.length, max);
          for (let i = 0; i < n; i++) HEAPU8[p + i] = s.charCodeAt(i); return n; } catch (e) { return 0; }
});

static void web_store_file(const char *name)
{
    static uint8_t buf[4096];
    char path[64]; snprintf(path, sizeof(path), "out/%s", name);
    FILE *f = fopen(path, "rb"); if (!f) return;
    int n = (int)fread(buf, 1, sizeof(buf), f); fclose(f);
    if (n > 0) js_store(name, buf, n);
}

static void web_load_file(const char *name)
{
    static uint8_t buf[4096];
    int n = js_load(name, buf, sizeof(buf));
    if (n <= 0) return;
    char path[64]; snprintf(path, sizeof(path), "out/%s", name);
    FILE *f = fopen(path, "wb"); if (!f) return;
    fwrite(buf, 1, n, f); fclose(f);
}

static int web_saved_seq = -1;
static void web_persist_sync(void)
{
    if (P.persist_seq == web_saved_seq) return;
    web_saved_seq = P.persist_seq;
    web_store_file("IRIDE.DAT"); web_store_file("IRIDE.BAK");
}

/* opzioni di Magic Lantern (tema, animazioni...): non hanno un file, si salvano a parte */
EM_JS(void, js_opts_save, (const int *o, int n), { try { localStorage.setItem("iride.opts", JSON.stringify(Array.from(HEAP32.subarray(o >> 2, (o >> 2) + n)))); } catch (e) {} });
EM_JS(int, js_opts_load, (int *o, int n), { try { const a = JSON.parse(localStorage.getItem("iride.opts") || "null"); if (!a || a.length != n) return 0;
                                                   for (let i = 0; i < n; i++) HEAP32[(o >> 2) + i] = a[i]; return 1; } catch (e) { return 0; } });
static int *const web_opts[] = { &opt_theme, &opt_anim, &opt_splash, &opt_boot_open, &opt_trash_open, &opt_light_key, &opt_set_wb,
                                 &opt_review_off, &opt_wind, &opt_sqm10, &opt_session, &opt_silent, &opt_dither_n, &opt_dither_s, &opt_lang };
#define WEB_NOPTS ((int)COUNT(web_opts))
static int web_opts_last[WEB_NOPTS];
static void web_opts_sync(int load)
{
    int v[WEB_NOPTS];
    if (load) { if (js_opts_load(v, WEB_NOPTS)) for (int i = 0; i < WEB_NOPTS; i++) *web_opts[i] = v[i]; }
    for (int i = 0; i < WEB_NOPTS; i++) v[i] = *web_opts[i];
    if (load || memcmp(v, web_opts_last, sizeof(v))) { memcpy(web_opts_last, v, sizeof(v)); if (!load) js_opts_save(v, WEB_NOPTS); }
}

/* ------------------------------------------------------------------ schermo */

EM_JS(void, js_present, (const uint8_t *vram, const uint8_t *lv), { if (Module.iridePresent) Module.iridePresent(vram, lv); });
EM_JS(int, js_pop_key, (), { return Module.iridePopKey ? Module.iridePopKey() : -1; });
EM_JS(void, js_state, (int open, int live, int screen_id), { if (Module.irideState) Module.irideState(open, live, screen_id); });

static int web_last_present = 0, web_last_tick = 0, web_in_tick = 0;
static void web_deliver(int key);

static void web_present(void)
{
    /* Live View dietro i pixel trasparenti: il cielo con la stella si vede in Fuoco stelle */
    int show_lv = lv && !sim_play && (!app_running || screen == SCR_STARS);
    js_present(vram_a + 30 * 960, show_lv ? lvbuf : 0);
    js_state(app_running, lv, screen);
}

static void web_shooting_screen(void);
static void web_play_screen(void);
static int web_last_shoot_draw = 0;

void web_sleep(int ms)
{
    int now = web_ms();
    /* Iride chiusa (anche mentre un task aspetta, per esempio il Live View che si accende):
       la schermata di scatto, come sulla 6D */
    if (!app_running && job_running == JOB_NONE && now - web_last_shoot_draw > 150)
    {
        web_last_shoot_draw = now;
        if (sim_play) web_play_screen(); else web_shooting_screen();
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
        if (persist_dirty) persist_save();                 /* nel browser si salva subito (non c'e' una scheda da proteggere) */
        web_persist_sync();
        web_opts_sync(0);
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

/* ------------------------------------------------------------------ foto scattate (per PLAY) */

struct web_shot { int fn, film, roll, iso, kelvin, style, contrast; char pre[8]; };
#define WEB_SHOTS 48
static struct web_shot web_shots[WEB_SHOTS];
static int web_nshots = 0, web_play_i = 0;

static void web_play_keys(int key)
{
    int n = MIN(web_nshots, WEB_SHOTS);
    switch (key)
    {
        case MODULE_KEY_PLAY: case MODULE_KEY_MENU: case MODULE_KEY_PRESS_HALFSHUTTER:
            sim_play = 0; return;
        case MODULE_KEY_PRESS_LEFT: case MODULE_KEY_WHEEL_LEFT: case MODULE_KEY_WHEEL_UP:
            if (n) web_play_i = MAX(web_play_i - 1, 0); return;
        case MODULE_KEY_PRESS_RIGHT: case MODULE_KEY_WHEEL_RIGHT: case MODULE_KEY_WHEEL_DOWN:
            if (n) web_play_i = MIN(web_play_i + 1, n - 1); return;
        case MODULE_KEY_TRASH:
            web_canon_msg = "Qui la 6D chiederebbe se cancellare la foto"; web_canon_msg_until = web_ms() + 2500; return;
    }
}

static void web_deliver(int key)
{
    static struct event ev;
    if (sim_play) { web_play_keys(key); return; }          /* in riproduzione i tasti sono di Canon */
    memset(&ev, 0, sizeof(ev));
    ev.param = web_raw_param(key);
    key_fake = 0;
    if (key == MODULE_KEY_TRASH && app_running)
    {
        /* dentro Iride il cestino porta al menu di Magic Lantern, che la demo non ha */
        web_canon_msg = "Qui si aprirebbe il menu di Magic Lantern (non e' nella demo)";
        web_canon_msg_until = web_ms() + 3500;
    }
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
        if (key == MODULE_KEY_PLAY)
        {
            sim_play = 1; web_play_i = MAX(MIN(web_nshots, WEB_SHOTS) - 1, 0);
        }
    }
}

/* scambio con la pagina: aggiunge un tasto alla coda */
EMSCRIPTEN_KEEPALIVE int web_vram_ptr(void) { return (int)(uintptr_t)(vram_a + 30 * 960); }

/* ------------------------------------------------------------------ schermata di scatto */

static void web_shooting_screen(void)
{
    to_idle = 0;
    uint8_t *b = bmp_vram();
    /* in Live View lo sfondo e' trasparente: sotto la pagina mette il cielo con la stella */
    for (int y = 0; y < 480; y++) memset(b + y * 960, lv ? 0 : COLOR_GRAY(6), 720);
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
    const char *h2 = "LV: Live View   MEZZO SCATTO: foto   PLAY: rivedi";
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
    if (app_running || sim_play) return;
    card.file_number = card.file_number % 9999 + 1;
    if (web_nshots >= WEB_SHOTS) { memmove(web_shots, web_shots + 1, sizeof(web_shots[0]) * (WEB_SHOTS - 1)); web_nshots = WEB_SHOTS - 1; }
    struct web_shot *s = &web_shots[web_nshots++];
    s->fn = card.file_number;
    s->roll = P.roll_loaded && !P.roll_paused;
    s->film = P.roll_film;
    s->iso = lens_info.raw_iso;
    s->kelvin = lens_info.wb_mode == WB_KELVIN ? lens_info.kelvin : 0;
    s->style = s->roll ? films[s->film].style : PS_STD;
    s->contrast = s->roll ? films[s->film].contrast : 0;
    snprintf(s->pre, sizeof(s->pre), "%s", sim_prefix);
}

/* una "foto": paesaggio al tramonto disegnato con la tavolozza di ML, diverso per ogni scatto,
   col carattere del rullino (contrasto, bianco caldo o freddo, bianco e nero) */
static void web_play_screen(void)
{
    to_idle = 0;
    uint8_t *b = bmp_vram();
    int n = MIN(web_nshots, WEB_SHOTS);
    for (int y = 0; y < 480; y++) memset(b + y * 960, COLOR_BLACK, 720);
    if (!n)
    {
        const char *t = "Nessuna foto: premi PLAY per uscire, poi MEZZO SCATTO per scattare";
        web_text(FONT(FONT_MED, COLOR_WHITE, 0xFF), 360 - web_str_width(2, t) / 2, 220, t);
        return;
    }
    web_play_i = COERCE(web_play_i, 0, n - 1);
    struct web_shot *s = &web_shots[web_play_i];
    unsigned h = (unsigned)s->fn * 2654435761u;
    int bw = s->roll && films[s->film].style == PS_MONO;
    int c = COERCE(s->contrast, -4, 4);
    int k = s->kelvin ? s->kelvin : 5200;
    /* cielo: gradiente di grigi, piu' ampio col contrasto alto */
    int top = 52 + c * 3, bot = 92 - c, horizon = 250 + (int)(h % 60);
    for (int y = 0; y < 410; y++)
    {
        int g = top + (bot - top) * MIN(y, horizon) / MAX(horizon, 1);
        memset(b + y * 960 + 0, COLOR_GRAY(COERCE(g, 0, 100)), 720);
    }
    /* sole: caldo o freddo secondo il bianco della pellicola */
    int sx = 120 + (int)((h >> 8) % 480), sy = horizon - 40 - (int)((h >> 16) % 80);
    int sun = bw ? COLOR_WHITE : k < 4500 ? COLOR_LIGHT_BLUE : k > 6000 ? COLOR_ORANGE : COLOR_YELLOW;
    web_fill_circle(sx, sy, 34, COLOR_GRAY(COERCE(bot + 4, 0, 100)));
    web_fill_circle(sx, sy, 24, sun);
    /* colline e prato */
    for (int x = 0; x < 720; x++)
    {
        int hy = horizon + (int)(18.0f * sinf((float)x * 0.013f + (float)(h % 7))) + (int)(10.0f * sinf((float)x * 0.041f));
        int g1 = COERCE(26 - c * 3, 2, 60);
        for (int y = hy; y < 410; y++)
            web_put(b, x, y, y < hy + 4 ? COLOR_GRAY(g1 + 12) : COLOR_GRAY(COERCE(g1 - (y - hy) / 12, 0, 100)));
    }
    /* barra delle informazioni, come in riproduzione */
    bmp_fill(COLOR_BLACK, 0, 410, 720, 70);
    char line[128], name[24];
    snprintf(name, sizeof(name), "%s%04d.CR2", s->pre, s->fn % 10000);
    const char *iso = s->iso ? lens_format_iso(s->iso) : "Auto";
    if (s->roll)
        snprintf(line, sizeof(line), "%s   %s %s   ISO %s   %d K   %s", name, films[s->film].brand, films[s->film].name, iso, k, style_name(s->style));
    else
        snprintf(line, sizeof(line), "%s   ISO %s   %s", name, iso, s->kelvin ? "bianco in K" : "bianco auto");
    web_text(FONT(FONT_MED, COLOR_WHITE, 0xFF), 16, 418, line);
    snprintf(line, sizeof(line), "foto %d di %d   </> sfoglia   PLAY o mezzo scatto: esci", web_play_i + 1, n);
    web_text(FONT(FONT_MED, COLOR_GRAY(40), 0xFF), 16, 448, line);
    if (web_ms() < web_canon_msg_until)
        web_text(FONT(FONT_MED, COLOR_YELLOW, 0xFF), 360 - web_str_width(2, web_canon_msg) / 2, 40, web_canon_msg);
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
    /* obiettivo finto: la stella parte sfocata, Fuoco stelle (a mano o con Q) la mette a fuoco */
    sim_af = 1; sim_fpos = sim_fmot = 0; sim_fbest = 22; sim_focus_sky();
    web_load_file("IRIDE.DAT"); web_load_file("IRIDE.BAK");
    web_opts_sync(1);
    iride_init();
    web_saved_seq = P.persist_seq;
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
