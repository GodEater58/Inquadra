/* Esegue il codice reale di Iride 0.2 su PC: test di logica, calcoli astronomici e schermate */
#include <math.h>
#include <sys/mman.h>
#include "../module/iride/iride.c"
#include "widths.h"

/* ================================================================ disegno registrato */
#define OPS_MAX (4 * 1024 * 1024)
static char *ops_idle, *ops_real;
static int len_idle = 0, len_real = 0, to_idle = 0;

static void op(const char *fmt, ...)
{
    char line[512];
    va_list ap; va_start(ap, fmt); int n = vsnprintf(line, sizeof(line), fmt, ap); va_end(ap);
    char *buf = to_idle ? ops_idle : ops_real;
    int *len = to_idle ? &len_idle : &len_real;
    if (*len + n + 1 < OPS_MAX) { memcpy(buf + *len, line, n); *len += n; buf[*len] = 0; }
}

int bmp_string_width(int fontspec, const char *s)
{
    int id = (fontspec >> 16) & 7, w = 0;
    for (; *s; s++) w += sim_w[id][(unsigned char)*s & 127];
    return w;
}
static long glyphs = 0;
int bmp_printf(uint32_t fontspec, int x, int y, const char *fmt, ...)
{
    char buf[256];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    glyphs += strlen(buf);
    op("T\t%d\t%d\t%d\t%d\t%d\t%s\n", (fontspec >> 16) & 7, fontspec & 0xFF, (fontspec >> 8) & 0xFF, x, y, buf);
    return bmp_string_width(fontspec, buf);
}
void bmp_fill(uint8_t c, int x, int y, int w, int h) { op("F\t%d\t%d\t%d\t%d\t%d\n", c, x, y, w, h); }
void bmp_draw_rect(int c, int x, int y, int w, int h) { op("R\t%d\t%d\t%d\t%d\t%d\n", c, x, y, w, h); }
void fill_circle(int x, int y, int r, int c) { op("C\t%d\t%d\t%d\t%d\t1\n", c, x, y, r); }
void draw_circle(int x, int y, int r, int c) { op("C\t%d\t%d\t%d\t%d\t0\n", c, x, y, r); }
void draw_line(int x1, int y1, int x2, int y2, int c) { op("L\t%d\t%d\t%d\t%d\t%d\n", c, x1, y1, x2, y2); }
void clrscr(void) { if (to_idle) len_idle = 0; else len_real = 0; op("X\n"); }
void bmp_draw_to_idle(int v) { to_idle = v; if (v) { len_idle = 0; } }
static int frames_shown = 0;
/* fotogrammi intermedi delle animazioni, salvati a tempi scelti */
static void shot(const char *name);
static int cap_on = 0, cap_t0 = 0, cap_i = 0, cap_n = 0, cap_times[24];
static char cap_name[32];
static int clock_ms;
static void capture(const char *name, const int *times, int n)
{
    snprintf(cap_name, sizeof(cap_name), "%s", name);
    memcpy(cap_times, times, n * sizeof(int)); cap_n = n; cap_i = 0; cap_t0 = clock_ms; cap_on = 1;
}
void bmp_idle_copy(int dir, int full)
{
    (void)full;
    if (!dir) return;
    memcpy(ops_real, ops_idle, len_idle + 1); len_real = len_idle; frames_shown++;
    while (cap_on && cap_i < cap_n && clock_ms - cap_t0 >= cap_times[cap_i])
    {
        char nm[64]; snprintf(nm, sizeof(nm), "%s_%02d", cap_name, cap_i); shot(nm); cap_i++;
    }
    if (cap_on && cap_i >= cap_n) cap_on = 0;
}
static uint8_t vram_a[960 * 540], vram_b[960 * 540];
uint8_t *bmp_vram_real() { return vram_a + 30 * 960; }
uint8_t *bmp_vram_idle() { return vram_b + 30 * 960; }

static void shot(const char *name)
{
    char path[128];
    snprintf(path, sizeof(path), "out/%s.ops", name);
    FILE *f = fopen(path, "w"); fwrite(ops_real, 1, len_real, f); fclose(f);
}

/* ================================================================ stub del firmware */
int gui_state = 0;   /* GUISTATE_IDLE su DryOS */
int pic_quality = PICQ_RAW_JPG_LARGE_FINE;
int dofpreview = 0;
static int sim_display = 1, sim_half = 0;
int display_is_on(void) { return sim_display; }
int sensor_cleaning = 0;
int get_halfshutter_pressed(void) { return sim_half; }
static int menu_redraw_calls = 0;
void menu_redraw(void) { menu_redraw_calls++; }
int menu_redraw_blocked, ml_shutdown_requested, shooting_mode = SHOOTMODE_M, lv = 0, lv_dispsize = 1, image_review_time = 2, RECORDING = 0;
int hdr_stepsize = 16;
struct lens_info lens_info;
static int clock_ms = 1000;
void msleep(int ms) { clock_ms += ms > 0 ? ms : 1; }
int get_ms_clock(void) { return clock_ms; }
static int tasks_created = 0;
void task_create(const char *n, int p, int s, void (*f)(void*), void *a) { (void)n; (void)p; (void)s; (void)f; (void)a; tasks_created++; }
static struct tm sim_tm;
void LoadCalendarFromRTC(struct tm *t) { *t = sim_tm; }
void menu_add(const char *n, struct menu_entry *m, int c) { (void)n; (void)m; (void)c; }
static int menu_open = 1, menu_closed_calls = 0, menu_opened_calls = 0;
int gui_menu_shown(void) { return menu_open; }
void gui_stop_menu(void) { menu_closed_calls++; }
void gui_open_menu(void) { menu_opened_calls++; }
int is_play_mode(void) { return 0; }
int is_camera(const char *m, const char *f) { return streq(m, "6D") && streq(f, "1.1.6"); }
static int fstack_runs = 0;
void focus_stack_run(int s) { (void)s; fstack_runs++; }
void lvinfo_add_items(struct lvinfo_item *items, int count) { (void)items; (void)count; }

/* menu di Magic Lantern simulato */
struct mlitem { const char *menu, *entry; int value, min, max, present; };
static struct mlitem mli[] = {
    { "Shoot", "Advanced Bracket", 0, 0, 1, 1 }, { "Advanced Bracket", "Frames", 1, 1, 12, 1 }, { "Advanced Bracket", "Sequence", 1, 0, 2, 1 },
    { "Focus Stacking", "Num. pics in front", 0, 0, 100, 1 }, { "Focus Stacking", "Num. pics behind", 0, 0, 100, 1 }, { "Focus Stacking", "Focus steps / picture", 1, 1, 10, 1 },
    { "Expo", "Dual ISO", 0, 0, 1, 1 }, { "Dual ISO", "Recovery ISO", 3, -12, 6, 1 }, { "Expo", "Auto ETTR", 0, 0, 1, 1 }, { "Auto ETTR", "Trigger mode", 0, 0, 3, 1 },
    { "Shoot", "Motion Detect", 0, 0, 1, 1 }, { "Motion Detect", "Trigger by", 0, 0, 2, 1 }, { "Motion Detect", "Trigger level", 8, 1, 30, 1 }, { "Focus", "Trap Focus", 0, 0, 2, 1 },
    { "Overlay", "Global Draw", 0, 0, 3, 1 }, { "Overlay", "Zebras", 0, 0, 1, 1 }, { "Overlay", "Focus Peak", 0, 0, 1, 1 }, { "Overlay", "Magic Zoom", 0, 0, 1, 1 },
    { "Overlay", "Histogram", 0, 0, 1, 1 }, { "Overlay", "False color", 0, 0, 1, 1 }, { "Overlay", "Spotmeter", 0, 0, 1, 1 },
    { "Expo", "Auto exposure", 0, 0, 1, 1 }, { "Shoot", "Post Deflicker", 0, 0, 1, 1 },
};
static struct mlitem *mlfind(const char *m, const char *e)
{
    for (unsigned i = 0; i < COUNT(mli); i++) if (mli[i].present && streq(mli[i].menu, m) && streq(mli[i].entry, e)) return &mli[i];
    return NULL;
}
int menu_get_value_from_script(const char *m, const char *e) { struct mlitem *it = mlfind(m, e); return it ? it->value : INT_MIN; }
int menu_set_value_from_script(const char *m, const char *e, int v)
{
    struct mlitem *it = mlfind(m, e);
    if (!it) return INT_MIN;
    if (v < it->min || v > it->max) return 0;
    it->value = v; return 1;
}
static int mlv(const char *m, const char *e) { return menu_get_value_from_script(m, e); }

/* stili immagine: cursori per stile */
static int ps_c[256], ps_s[256], ps_t[256];
static int last_review_set = -1, wb_mode_set = -1;
void prop_request_change(unsigned p, const void *a, size_t l)
{
    (void)l;
    int v = *(const int *)a;
    if (p == PROP_PICTURE_STYLE) lens_info.raw_picstyle = v;
    if (p == PROP_IMAGE_REVIEW_TIME) { image_review_time = v; last_review_set = v; }
    if (p == PROP_WB_MODE_PH || p == PROP_WB_MODE_LV) { lens_info.wb_mode = v; wb_mode_set = v; }
}
int lens_get_contrast(void) { return ps_c[lens_info.raw_picstyle]; }
int lens_get_saturation(void) { return ps_s[lens_info.raw_picstyle]; }
int lens_get_color_tone(void) { return ps_t[lens_info.raw_picstyle]; }
void lens_set_contrast(int v) { ps_c[lens_info.raw_picstyle] = v; }
void lens_set_saturation(int v) { ps_s[lens_info.raw_picstyle] = v; }
void lens_set_color_tone(int v) { ps_t[lens_info.raw_picstyle] = v; }

int val2raw_iso(int v) { return (int)roundf(72.0f + 8.0f * log2f((float)v / 100.0f)); }
int val2raw_aperture(int v) { return v; }
int lens_set_rawaperture(int a) { lens_info.raw_aperture = a; lens_info.aperture = a; return 1; }
int lens_set_rawiso(int i) { lens_info.raw_iso = i; return 1; }
int lens_set_rawshutter(int s) { lens_info.raw_shutter = s; return 1; }
void lens_set_kelvin(int k) { lens_info.kelvin = k; lens_info.wb_mode = WB_KELVIN; }
int shutterf_to_raw(float s) { return (int)roundf(56.0f - log2f(s) * 8.0f); }
float raw2shutterf(int raw) { return powf(2.0f, (56.0f - (float)raw) / 8.0f); }
const char *lens_format_shutter(int r)
{
    static char b[16]; float t = raw2shutterf(r);
    if (t >= 0.3f) snprintf(b, sizeof(b), "%d\"", (int)(t + 0.5f)); else snprintf(b, sizeof(b), "1/%d", (int)(1.0f / t + 0.5f));
    return b;
}
const char *lens_format_aperture(int r) { static char b[16]; snprintf(b, sizeof(b), "f/%d.%d", r / 10, r % 10); return b; }
const char *lens_format_iso(int r) { static char b[16]; snprintf(b, sizeof(b), "%d", (int)(100.0f * powf(2.0f, (float)(r - 72) / 8.0f) + 0.5f)); return b; }
const char *get_shootmode_name_short(int m) { return m == SHOOTMODE_M ? "M" : m == SHOOTMODE_BULB ? "B" : m == SHOOTMODE_AV ? "Av" : m == SHOOTMODE_TV ? "Tv" : "P"; }
static int shots_taken = 0, bulb_ms_total = 0, last_bulb_ms = 0;
static struct card_info card = { 1234 };
int lens_take_picture(int w, int af) { (void)w; (void)af; shots_taken++; card.file_number++; return 0; }
int bulb_take_pic(int ms) { bulb_ms_total += ms; last_bulb_ms = ms; shots_taken++; card.file_number++; return 0; }
int lens_focus(int n, int s, int w, int e) { (void)n; (void)s; (void)w; (void)e; return 1; }
int is_manual_focus(void) { return 0; }
void set_lv_zoom(int z) { lv_dispsize = z; }
struct card_info *get_shooting_card(void) { return &card; }
char *get_config_dir(void) { return "out/"; }
int FIO_CreateDirectory(const char *d) { (void)d; return 0; }
static char sim_path(char *dst, const char *n) { (void)dst; (void)n; return 0; }
FILE *FIO_CreateFileOrAppend(const char *n) { (void)n; return fopen("out/IRIDE.CSV", "a"); }
FILE *FIO_CreateFile(const char *n)
{
    /* ML/LOGS/... della scheda -> out/ */
    const char *b = strrchr(n, '/');
    if (strncmp(n, "ML/", 3) == 0 && b) { char p[160]; snprintf(p, sizeof(p), "out/%s", b + 1); return fopen(p, "wb"); }
    return fopen(n, "wb");
}
FILE *FIO_OpenFile(const char *n, unsigned m) { (void)m; return fopen(n, "rb"); }
int FIO_ReadFile(FILE *f, void *p, size_t n) { return (int)fread(p, 1, n, f); }
int FIO_WriteFile(FILE *f, const void *p, size_t n) { return (int)fwrite(p, 1, n, f); }
void FIO_CloseFile(FILE *f) { fclose(f); }
int my_fprintf(FILE *f, const char *fmt, ...) { va_list ap; va_start(ap, fmt); int r = vfprintf(f, fmt, ap); va_end(ap); return r; }
static char last_notify[160];
void NotifyBox(int t, const char *fmt, ...) { (void)t; va_list ap; va_start(ap, fmt); vsnprintf(last_notify, sizeof(last_notify), fmt, ap); va_end(ap); }
void beep(void) {}
void beep_times(int n) { (void)n; }
static char sim_prefix[8] = "IMG_";
static int prefix_owner = 0;
int file_prefix_set(char *p) { if (prefix_owner) return 0; snprintf(sim_prefix, sizeof(sim_prefix), "%s", p); prefix_owner = 4242; return 4242; }
int file_prefix_reset(int k) { if (k != prefix_owner) return 0; snprintf(sim_prefix, sizeof(sim_prefix), "IMG_"); prefix_owner = 0; return 1; }
void display_on(void) {}
void display_off(void) {}

/* Live View finto con una stella */
static uint8_t lvbuf[480 * 720 * 2];
static struct vram_info lvinfo_s = { lvbuf, 720, 1440, 480 };
struct vram_info *get_yuv422_vram(void) { return &lvinfo_s; }
static unsigned rng = 12345;
static int rnd(int n) { rng = rng * 1103515245u + 12345u; return (int)((rng >> 16) % (unsigned)n); }
static void make_sky(float sigma, float amp, int sx, int sy)
{
    for (int y = 0; y < 480; y++)
        for (int x = 0; x < 720; x++)
        {
            float dx = (float)(x - sx), dy = (float)(y - sy);
            float v = 18.0f + (float)rnd(7) + amp * expf(-(dx * dx + dy * dy) / (2.0f * sigma * sigma));
            if (rnd(4000) == 0) v += 30.0f;
            if (v > 255.0f) v = 255.0f;
            lvbuf[y * 1440 + x * 2] = 128; lvbuf[y * 1440 + x * 2 + 1] = (uint8_t)v;
        }
}

/* ================================================================ aiuti per i test */
static int fails = 0, passes = 0;
#define CHECK(cond, ...) do { if (!(cond)) { fails++; printf("FALLITO: "); printf(__VA_ARGS__); printf("\n"); } else { passes++; printf("ok: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void set_time(int y, int mo, int d, int h, int mi) { memset(&sim_tm, 0, sizeof(sim_tm)); sim_tm.tm_year = y - 1900; sim_tm.tm_mon = mo - 1; sim_tm.tm_mday = d; sim_tm.tm_hour = h; sim_tm.tm_min = mi; }

static void camera_reset()
{
    shooting_mode = SHOOTMODE_M; lens_info.raw_iso = 80; lens_info.raw_shutter = 104; lens_info.raw_aperture = 40;
    lens_info.wb_mode = 0; lens_info.kelvin = 5200; lens_info.raw_picstyle = 0x81; image_review_time = 2;
    memset(ps_c, 0, sizeof(ps_c)); memset(ps_s, 0, sizeof(ps_s)); memset(ps_t, 0, sizeof(ps_t));
}

static void settle() { for (int i = 0; i < 6; i++) { clock_ms += 400; theme_apply(); render(0); } }
static void screenshot(int s, const char *name) { screen = s; hl_reset(); hr_cur[0] = -1; settle(); shot(name); }
static void press(int k) { handle_key(k); }

int main(void)
{
    ops_idle = malloc(OPS_MAX); ops_real = malloc(OPS_MAX); ops_idle[0] = ops_real[0] = 0;
    set_time(2026, 11, 1, 22, 47);
    persist_defaults();
    camera_reset();
    snprintf(lens_info.name, sizeof(lens_info.name), "EF24-105mm f/4L IS USM");
    lens_info.lens_exists = 1; lens_info.focal_len = 24; lens_info.aperture = 40;
    remove("out/IRIDE.CSV"); remove("out/IRIDE.DAT");

    /* ---------- calcoli di base */
    CHECK(npf_tenths(24, 28) == 122, "NPF 24mm f/2.8 = 12.2 s");
    CHECK(npf_shutter_code(30.0f) == 16, "NPF 30 s -> codice 30\"");
    CHECK(npf_shutter_code(12.2f) == 27, "NPF 12.2 s -> 13\"");
    CHECK(step_in(bulb_steps, COUNT(bulb_steps), 100, -1) == 90 && step_in(bulb_steps, COUNT(bulb_steps), 100, 1) == 120, "scala posa B tra due gradini");

    /* ---------- HFR */
    lv = 1;
    float sig[] = { 1.5f, 2.0f, 3.0f, 4.0f };
    for (unsigned i = 0; i < COUNT(sig); i++)
    {
        make_sky(sig[i], 180.0f, 300 + (int)i * 40, 200);
        int h, p, cx, cy, r;
        int ok = hfr_measure(&h, &p, &cx, &cy, &r);
        float ex = sig[i] * 1.2533f;
        CHECK(ok && fabsf((float)h / 100.0f - ex) < 0.3f * ex, "HFR sigma %.1f -> %.2f (atteso ~%.2f)", (double)sig[i], (double)h / 100.0, (double)ex);
    }
    make_sky(2.0f, 0.0f, 300, 200);
    { int h, p, cx, cy, r; CHECK(!hfr_measure(&h, &p, &cx, &cy, &r), "cielo senza stelle: nessuna misura"); }

    /* ---------- salvataggio su scheda */
    P.roll_num = 7; P.lat100 = 4400; P.prof[2].valid = 1; P.prof[2].iso = 96;
    persist_save();
    P.roll_num = 0; P.lat100 = 0; P.prof[2].valid = 0;
    persist_load();
    CHECK(P.roll_num == 7 && P.lat100 == 4400 && P.prof[2].valid == 1 && P.prof[2].iso == 96, "salvataggio IRIDE.DAT e rilettura");
    { FILE *f = fopen("out/IRIDE.DAT", "r+b"); fseek(f, 40, SEEK_SET); fputc(0x55, f); fclose(f); }
    persist_load();
    CHECK(P.roll_num == 0 && P.lat100 == 4190, "file rovinato: valori di fabbrica (nessun crash)");
    persist_defaults();

    /* ---------- rullino + Torna alla 6D */
    camera_reset();
    ps_c[PS_PORTRAIT] = 2; ps_s[PS_PORTRAIT] = 3; ps_t[PS_PORTRAIT] = -2;   /* cursori dell'utente */
    roll_load(1, 36, 1);                         /* Portra 400 tirata a 800 */
    CHECK(lens_info.raw_picstyle == PS_PORTRAIT && ps_c[PS_PORTRAIT] == -1 && ps_t[PS_PORTRAIT] == 1, "Portra 400: stile Ritratto e cursori della pellicola");
    CHECK(lens_info.raw_iso == val2raw_iso(800) && lens_info.kelvin == 5300 && image_review_time == 0, "push +1: ISO 800, 5300 K, anteprima spenta");
    CHECK(streq(sim_prefix, "R001"), "nomi file R001: %s", sim_prefix);
    card.file_number += 2; iride_clock(0);
    CHECK(P.roll_shots == 2, "contatore: 2 pose");
    roll_pause();
    CHECK(P.roll_paused && lens_info.raw_picstyle == 0x81 && lens_info.raw_iso == 80 && wb_mode_set == 0 && image_review_time == 2,
        "pausa: torna stile Standard, ISO 200, bianco auto, anteprima 2 s");
    CHECK(ps_c[PS_PORTRAIT] == 2 && ps_s[PS_PORTRAIT] == 3 && ps_t[PS_PORTRAIT] == -2, "pausa: cursori di Ritratto dell'utente rimessi (2, 3, -2)");
    card.file_number += 3; iride_clock(0);
    CHECK(P.roll_shots == 2, "in pausa le foto non consumano il rullino");
    CHECK(streq(sim_prefix, "IMG_"), "in pausa i file tornano IMG_");
    roll_resume();
    card.file_number += 1; iride_clock(0);
    CHECK(P.roll_shots == 3 && lens_info.raw_picstyle == PS_PORTRAIT, "ripreso: conta di nuovo (3) e pellicola attiva");

    /* ---------- qualita' immagine: solo lettura, avviso se solo RAW */
    pic_quality = PICQ_RAW;
    CHECK(picq_raw_only(), "solo RAW riconosciuto");
    pic_quality = PICQ_RAW_JPG_LARGE_FINE;
    CHECK(!picq_raw_only(), "RAW+L: nessun avviso");
    pic_quality = PICQ_LARGE_FINE;
    CHECK(!picq_raw_only(), "solo JPEG L: nessun avviso");
    {
        struct persist saved = P;   /* il test carica rullini di prova: poi tutto torna com'era */
        pic_quality = PICQ_RAW;
        conf_mode = CONF_LOAD; shelf_cat = 0; shelf_pos[0] = 1; load_exp = 36; load_push = 0;
        conf_activate(2);
        CHECK(strstr(msg, "RAW+L") != NULL && msg_color == C_WARN, "carico in solo RAW: avviso '%s'", msg);
        CHECK(pic_quality == PICQ_RAW, "la qualita' non viene mai cambiata dal firmware");
        pic_quality = PICQ_RAW_JPG_LARGE_FINE;
        conf_mode = CONF_LOAD; conf_activate(2);
        CHECK(strstr(msg, "RAW") == NULL, "carico in RAW+L: nessun avviso ('%s')", msg);
        P = saved; prefix_refresh(); film_activate();
    }

    /* ---------- funzioni di Magic Lantern dall'interfaccia */
    P.hdr_frames = 5; P.hdr_step = 1;
    hdr_change(0, 1, 0);
    CHECK(mlv("Shoot", "Advanced Bracket") == 1 && mlv("Advanced Bracket", "Frames") == 5 && hdr_stepsize == 12, "HDR acceso: 5 scatti, passo 1,5 EV");
    lv_change(0, 1, 0);
    CHECK(mlv("Overlay", "Focus Peak") == 1 && mlv("Overlay", "Global Draw") == 3, "peaking acceso (e disegno globale acceso)");
    dual_change(0, 1, 0);
    CHECK(mlv("Expo", "Dual ISO") == 1 && mlv("Dual ISO", "Recovery ISO") == 4, "Dual ISO acceso con recupero 1600");
    mli[8].present = 0;   /* modulo ETTR non caricato */
    { char b[64]; int k; ettr_value(0, b, sizeof(b), &k); CHECK(k == VK_WARN && strstr(b, "modulo"), "ETTR senza modulo: '%s'", b); }
    ettr_change(0, 1, 0);
    CHECK(strstr(msg, "Modules") != NULL, "ETTR senza modulo: messaggio '%s'", msg);
    mli[8].present = 1;

    /* ---------- profili */
    camera_reset();
    lens_info.raw_iso = 104; lens_info.raw_shutter = 120; lens_info.raw_aperture = 28; lens_info.raw_picstyle = PS_LANDSCAPE;
    ps_c[PS_LANDSCAPE] = 2;
    profile_save(0);
    camera_reset();
    int full = profile_load(0);
    CHECK(full && lens_info.raw_iso == 104 && lens_info.raw_shutter == 120 && lens_info.raw_aperture == 28 && lens_info.raw_picstyle == PS_LANDSCAPE && ps_c[PS_LANDSCAPE] == 2,
        "profilo salvato e ricaricato (ISO, tempo, diaframma, stile)");

    /* ---------- Torna alla 6D: tutto */
    restore_all();
    CHECK(mlv("Shoot", "Advanced Bracket") == 0 && mlv("Advanced Bracket", "Frames") == 1 && hdr_stepsize == 16, "Torna alla 6D: HDR spento e valori ML di prima");
    CHECK(mlv("Overlay", "Focus Peak") == 0 && mlv("Overlay", "Global Draw") == 0 && mlv("Expo", "Dual ISO") == 0, "Torna alla 6D: overlay e Dual ISO come prima");
    CHECK(restore_pending_count() == 0 && P.roll_paused, "Torna alla 6D: niente in sospeso, rullino in pausa");

    /* ---------- filtri ND */
    P.nd_base = 104; P.nd_idx = 7;   /* 1/60 con ND1000 */
    { char b[64]; int k; nd_value(2, b, sizeof(b), &k); CHECK(nd_result_tenths() == 160, "1/60 + ND1000 = %s", b); }
    shooting_mode = SHOOTMODE_M; nd_activate(3);
    CHECK(lens_info.raw_shutter == 24, "ND fino a 30 s: tempo impostato (%s)", lens_format_shutter(lens_info.raw_shutter));
    P.nd_base = 80; job_request = JOB_NONE; nd_activate(3);
    CHECK(job_request == JOB_BULB && job_bulb_s == 128, "1/8 + ND1000 = 128 s: posa B programmata");
    job_request = JOB_NONE;

    /* ---------- intervallometro: dark e giorno-notte */
    roll_resume();
    preset_select(5); P.int_bulb = 60; P.int_count = 3; P.int_pause = 0; P.int_delay_idx = 0;
    shots_taken = 0; bulb_ms_total = 0;
    int shots_before = P.roll_shots;
    job_running = JOB_SEQ; job_task(NULL);
    iride_clock(0);
    CHECK(shots_taken == 3 && bulb_ms_total == 180000 && P.roll_shots == shots_before, "Dark: 3 pose B da 60 s, rullino non toccato");
    CHECK(strstr(last_notify, "finita") != NULL && streq(sim_prefix, "R001"), "fine sequenza: '%s', prefisso %s", last_notify, sim_prefix);
    preset_select(PRESET_HG); lv = 0;
    CHECK(!interval_check() && strstr(msg, "Live View"), "giorno-notte senza Live View: rifiutato");
    lv = 1; P.int_count = 2; P.int_pause = 0;
    job_running = JOB_SEQ; job_task(NULL);
    CHECK(mlv("Expo", "Auto exposure") == 0 && mlv("Shoot", "Post Deflicker") == 0 && shots_taken == 5, "giorno-notte: autoexpo e deflicker spenti di nuovo a fine sequenza");

    /* ---------- tasti e navigazione */
    app_running = 1; screen = SCR_HOME; sel[SCR_HOME] = 0;
    press(MODULE_KEY_PRESS_SET); CHECK(screen == SCR_ASTRO, "Home + SET -> Astro");
    for (int i = 0; i < 3; i++) press(MODULE_KEY_WHEEL_DOWN);
    press(MODULE_KEY_PRESS_SET); CHECK(screen == SCR_POLAR, "Astro, 4a voce -> Allineamento polare");
    press(MODULE_KEY_MENU); CHECK(screen == SCR_ASTRO, "MENU torna ad Astro");
    press(MODULE_KEY_MENU); press(MODULE_KEY_PRESS_RIGHT); press(MODULE_KEY_PRESS_SET);
    CHECK(screen == SCR_FOTO, "Home: destra + SET -> Foto");
    press(MODULE_KEY_MENU); press(MODULE_KEY_PRESS_DOWN); CHECK(sel[SCR_HOME] == 4, "Home: giu' -> Profili");
    CHECK(iride_keypress(MODULE_KEY_TRASH) == 0 && to_ml_menu == 1, "cestino dentro Iride: si passa al menu di Magic Lantern");
    to_ml_menu = 0;
    CHECK(iride_keypress(MODULE_KEY_UNPRESS_SET) == 0, "i rilasci non arrivano a Canon");
    app_running = 0; menu_open = 0;
    struct event *ev = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    unsigned int evp = (unsigned int)(uintptr_t)ev;
    { ev->param = 0x20; int before = menu_opened_calls;
      CHECK(iride_keypress_raw(evp) == 0 && menu_opened_calls == before + 1, "tasto luce LCD apre Iride"); }
    { ev->param = 0x07; CHECK(iride_keypress_raw(evp) == 1, "gli altri tasti passano"); }
    opt_light_key = 0;
    { ev->param = 0x20; CHECK(iride_keypress_raw(evp) == 1, "tasto luce normale se l'opzione e' spenta"); }
    opt_light_key = 1;

    /* ---------- cestino dalla schermata di scatto */
    { int before = menu_opened_calls; screen = SCR_SHELF; menu_redraw_blocked = 0;
      CHECK(iride_keypress(MODULE_KEY_TRASH) == 0 && menu_opened_calls == before + 1 && screen == SCR_HOME && menu_redraw_blocked == 1,
          "cestino apre Iride sulla Home, senza lampo del menu ML");
      menu_redraw_blocked = 0; }
    gui_state = 1;   /* riproduzione: il cestino cancella le foto, non si tocca */
    { int before = menu_opened_calls; CHECK(iride_keypress(MODULE_KEY_TRASH) == 1 && menu_opened_calls == before, "in riproduzione il cestino resta di Canon"); }
    { ev->param = 0x20; CHECK(iride_keypress_raw(evp) == 1, "in riproduzione il tasto luce resta di Canon (zebre ML)"); }
    gui_state = 0;
    dofpreview = 1;
    CHECK(iride_keypress(MODULE_KEY_TRASH) == 1, "con l'anteprima di profondita' bloccata non si apre (come ML)");
    dofpreview = 0;
    opt_trash_open = 0;
    CHECK(iride_keypress(MODULE_KEY_TRASH) == 1, "opzione spenta: il cestino apre il menu ML come sempre");
    opt_trash_open = 1;

    /* ---------- apertura all'accensione */
    menu_open = 0; app_running = 0; gui_state = 0;
    { int before = menu_opened_calls; boot_done = 0; screen = SCR_SHELF; int t0 = clock_ms;
      boot_task(NULL);
      CHECK(menu_opened_calls == before + 1 && screen == SCR_HOME && clock_ms - t0 >= 2500, "all'accensione si apre la Home (dopo %d ms)", clock_ms - t0);
      menu_redraw_blocked = 0; }
    { int before = menu_opened_calls; boot_done = 0; sim_half = 1; boot_task(NULL); sim_half = 0;
      CHECK(menu_opened_calls == before, "acceso col mezzo scatto premuto: niente Iride"); }
    { int before = menu_opened_calls; boot_done = 0; sensor_cleaning = 1; int t0 = clock_ms; boot_task(NULL); sensor_cleaning = 0;
      CHECK(menu_opened_calls == before && clock_ms - t0 < 15000, "pulizia del sensore infinita: rinuncia dopo %d ms", clock_ms - t0); }
    { int before = menu_opened_calls; boot_done = 0; opt_boot_open = 0; boot_task(NULL); opt_boot_open = 1;
      CHECK(menu_opened_calls == before, "opzione spenta: niente Iride all'accensione"); }
    { int before = menu_opened_calls; boot_done = 0; sim_display = 0; boot_task(NULL); sim_display = 1;
      CHECK(menu_opened_calls == before, "display spento: niente Iride"); }

    /* ---------- diagnostica tasti */
    { int n0 = klog_count;
      ev->param = 0x69; iride_keypress_raw(evp);
      CHECK(klog_count == n0, "aggiornamenti interni del display non registrati");
      ev->param = 0x20; ev->arg = 0; iride_keypress_raw(evp);
      struct klog_entry *k = &klog[(klog_count - 1) % KLOG_N];
      CHECK(klog_count == n0 + 1 && k->param == 0x20 && k->opened == 1 && k->gs == 0, "tasto luce registrato: 0x%02x, aperta %d", k->param, k->opened);
      CHECK(streq(key_name(0x20), "LUCE LCD") && streq(key_name(0x0d), "CESTINO"), "nomi dei tasti 6D");
      for (int i = 0; i < 20; i++) { ev->param = 0x07; iride_keypress_raw(evp); }
      CHECK(klog[(klog_count - 1) % KLOG_N].param == 0x07, "registro circolare: 20 eventi senza sforare");
      klog_save();
      FILE *kf = fopen("out/IRIDEKEY.TXT", "r"); char l1[160] = ""; if (kf) { if (!fgets(l1, sizeof(l1), kf)) l1[0] = 0; if (!fgets(l1, sizeof(l1), kf)) l1[0] = 0; if (!fgets(l1, sizeof(l1), kf)) l1[0] = 0; fclose(kf); }
      CHECK(strncmp(l1, "0x07,", 5) == 0, "IRIDEKEY.TXT scritto: '%.30s'", l1); }
    menu_open = 1;

    /* ---------- ora legale e calcoli astronomici */
    CHECK(!eu_dst(2026, 3, 29, 0) && eu_dst(2026, 3, 29, 1) && eu_dst(2026, 10, 25, 0) && !eu_dst(2026, 10, 25, 1), "ora legale 2026: 29 marzo - 25 ottobre alle 01 UTC");
    FILE *jf = fopen("out/astro.json", "w");
    fprintf(jf, "[\n");
    int cases[][5] = { {2026,11,1,22,47}, {2026,11,2,3,15}, {2027,1,15,21,0}, {2027,7,20,23,30}, {2026,12,24,18,0}, {2028,3,10,4,0} };
    for (unsigned i = 0; i < COUNT(cases); i++)
    {
        int *c = cases[i];
        int d70 = days_from_civil(c[0], c[1], c[2]);
        int off = utc_offset_min(c[0], c[1], c[2], c[3]);
        struct jtime j = jt_from_local(d70, c[3] * 60 + c[4], off);
        float ra, dec, lst = lst_deg(j, lon_deg());
        sun_pos(j, &ra, &dec, 0); float sun = altitude(ra, dec, lst, lat_deg());
        moon_pos(j, &ra, &dec, 0); float moon = altitude(ra, dec, lst, lat_deg());
        int wax; int ill = moon_illum(j, &wax);
        float gc = altitude(GC_RA, GC_DEC, lst, lat_deg());
        fprintf(jf, " {\"local\":\"%04d-%02d-%02d %02d:%02d\",\"utc_offset\":%d,\"polar_ha\":%.4f,\"sun_alt\":%.3f,\"moon_alt\":%.3f,\"moon_illum\":%.4f,\"gc_alt\":%.3f}%s\n",
            c[0], c[1], c[2], c[3], c[4], off, (double)polar_ha_hours(j), (double)sun, (double)moon, ill / 1000.0, (double)gc, i + 1 < COUNT(cases) ? "," : "");
    }
    fprintf(jf, "]\n"); fclose(jf);
    sky_enter(); while (!sky_ready) sky_tick();
    jf = fopen("out/nights.json", "w"); fprintf(jf, "[\n");
    for (int i = 0; i < SKY_NIGHTS; i++)
        fprintf(jf, " {\"day1970\":%d,\"sunset\":%d,\"dusk\":%d,\"dawn\":%d,\"sunrise\":%d,\"moon_illum\":%d,\"moonless\":%d,\"gc_start\":%d,\"gc_end\":%d}%s\n",
            nights[i].day1970, nights[i].sunset, nights[i].dusk, nights[i].dawn, nights[i].sunrise, nights[i].moon_illum, nights[i].moonless_min,
            nights[i].gc_start, nights[i].gc_end, i + 1 < SKY_NIGHTS ? "," : "");
    fprintf(jf, "]\n"); fclose(jf);

    /* ---------- schermate */
    camera_reset();
    lens_info.raw_iso = 104;
    opt_theme = 0;
    P.roll_paused = 0;
    P.prof[0].valid = 1; P.prof[0].name = 0; P.prof[1].valid = 1; P.prof[1].name = 3; P.prof[1].mode = SHOOTMODE_M;
    P.prof[1].shutter = 32; P.prof[1].aperture = 28; P.prof[1].iso = 120; P.prof[1].style = PS_NEUTRAL;
    sel[SCR_HOME] = 0; screenshot(SCR_HOME, "01_home");
    { int th = opt_theme; opt_theme = 1; screenshot(SCR_HOME, "01b_home_notte"); opt_theme = th; }
    sel[SCR_ASTRO] = 1; screenshot(SCR_ASTRO, "02_astro");
    P.npf_focal = 24; P.npf_ap10 = 28; sel[SCR_NPF] = 2; msg[0] = 0; screenshot(SCR_NPF, "03_npf");
    preset_select(PRESET_HG); sel[SCR_INTERVAL] = 0; screenshot(SCR_INTERVAL, "04_intervallometro");
    lv = 1; lv_dispsize = 10; stars_reset();
    float seq[] = { 6.5f, 5.0f, 3.6f, 2.6f, 2.0f, 1.75f, 1.8f, 2.2f, 1.9f, 1.75f };
    for (unsigned i = 0; i < COUNT(seq); i++) { make_sky(seq[i] * 1.6f, 170.0f, 352, 222); stars_tick(); }
    screenshot(SCR_STARS, "05_stelle");
    { FILE *g = fopen("out/05_lv.pgm", "wb"); fprintf(g, "P5 720 480 255\n"); for (int y = 0; y < 480; y++) for (int x = 0; x < 720; x++) fputc(lvbuf[y * 1440 + x * 2 + 1], g); fclose(g); }
    screenshot(SCR_POLAR, "06_polare");
    sky_enter(); while (!sky_ready) sky_tick(); screenshot(SCR_SKY, "07_cielo");
    sel[SCR_FOTO] = 0; screenshot(SCR_FOTO, "08_foto");
    sel[SCR_HDR] = 1; screenshot(SCR_HDR, "09_hdr");
    sel[SCR_FSTACK] = 3; screenshot(SCR_FSTACK, "10_focus_stack");
    sel[SCR_LVTOOLS] = 0; ml_set(MLF_PEAK, 1); ml_set(MLF_HIST, 1); screenshot(SCR_LVTOOLS, "11_strumenti_lv");
    P.nd_base = 80; P.nd_idx = 7; sel[SCR_ND] = 3; screenshot(SCR_ND, "12_filtri_nd");
    sel[SCR_MOTION] = 0; screenshot(SCR_MOTION, "13_movimento");
    shelf_enter(); screenshot(SCR_SHELF, "14_rullini");
    { int th = opt_theme; opt_theme = 1; screenshot(SCR_SHELF, "14c_rullini_notte"); opt_theme = th; }
    conf_mode = CONF_LOAD; load_exp = 36; load_push = 1; sel[SCR_CONFIRM] = 1; screenshot(SCR_CONFIRM, "15_carica");
    { int rl = P.roll_loaded; P.roll_loaded = 0; pic_quality = PICQ_RAW; conf_mode = CONF_LOAD; screenshot(SCR_CONFIRM, "15b_carica_solo_raw");
      P.roll_loaded = 1; P.roll_paused = 0; screenshot(SCR_SHELF, "14b_scaffale_solo_raw"); P.roll_loaded = rl; pic_quality = PICQ_RAW_JPG_LARGE_FINE; }
    conf_mode = CONF_ROLL; sel[SCR_CONFIRM] = 1; screenshot(SCR_CONFIRM, "16_rullino");
    sel[SCR_PROFILES] = 1; screenshot(SCR_PROFILES, "17_profili");
    sel[SCR_OPTIONS] = 0; screenshot(SCR_OPTIONS, "18_opzioni");
    snap_take(); screenshot(SCR_RESTORE, "19_torna_6d");
    opt_theme = 2; screenshot(SCR_POLAR, "20_polare_notte");
    screenshot(SCR_SKY, "21_cielo_notte");
    opt_theme = 0;
    /* caricamento animato: un fotogramma a meta' */
    { screen = SCR_SHELF; theme_apply(); P.roll_push = 0; P.roll_film = 3;
      static int tl[24];
      for (int i = 0; i < 24; i++) tl[i] = i * 92;   /* 24 fotogrammi per la GIF del README */
      capture("anim_carica", tl, COUNT(tl));
      anim_load(&films[3]); shot("22_caricamento");
      static const int tr[] = { 80, 300, 600, 900, 1150, 1300, 1420, 1600, 1900 };
      capture("anim_riavvolgi", tr, COUNT(tr));
      anim_rewind(&films[1], 24); cap_on = 0; }

    { int t0 = clock_ms; splash(); anim_rewind(&films[1], 12); frame_show(1); frame_show(-1);
      CHECK(clock_ms - t0 < 5000, "splash, riavvolgimento e transizioni finiscono da soli (%d ms simulati)", clock_ms - t0); }

    /* ---------- tavola dei 16 rullini (due schermate da 8) */
    for (int page = 0; page < 2; page++)
    {
        theme_apply();
        frame_begin();
        fill(C_BG, 0, 0, SCR_W, SCR_H);
        for (int i = 0; i < 8; i++)
        {
            int fi = page * 8 + i;
            int cx = 46 + i * 89;
            draw_canister(&films[fi], cx, 40, 100, 0);
            draw_canister(&films[fi], cx, 250, 60, 0);
            txt_c(FONT_SMALL, C_DIM, cx, 340, films[fi].name);
        }
        draw_canister(&films[page * 8 + 2], 200, 360, 80, 30);
        draw_canister(&films[page * 8 + 5], 480, 360, 80, 30);
        frame_show(0);
        shot(page ? "rullini_2" : "rullini_1");
    }

    /* ---------- tutte le schermate in entrambi i temi, con tutti i tasti: nessun crash */
    int keys[] = { MODULE_KEY_PRESS_UP, MODULE_KEY_PRESS_DOWN, MODULE_KEY_PRESS_LEFT, MODULE_KEY_PRESS_RIGHT, MODULE_KEY_WHEEL_LEFT, MODULE_KEY_WHEEL_RIGHT,
                   MODULE_KEY_WHEEL_UP, MODULE_KEY_WHEEL_DOWN, MODULE_KEY_INFO, MODULE_KEY_Q };
    int crashes = 0;
    for (int th = 0; th < 2; th++)
        for (int s = 0; s < SCR_COUNT; s++)
        {
            opt_theme = th;
            screen = s;
            for (unsigned k = 0; k < COUNT(keys); k++) { handle_key(keys[k]); if (screen != s) screen = s; render(0); }
        }
    CHECK(crashes == 0, "tutte le %d schermate x 2 temi x %d tasti disegnate senza errori", SCR_COUNT, (int)COUNT(keys));

    /* ---------- costo di disegno: lettere per fotogramma (indicatore di velocita') */
    long maxg = 0; int maxs = 0;
    for (int s = 0; s < SCR_COUNT; s++) { screen = s; glyphs = 0; render(0); if (glyphs > maxg) { maxg = glyphs; maxs = s; } }
    printf("info: schermata piu' pesante %s con %ld caratteri per fotogramma\n", screens[maxs].title, maxg);

    printf("\n%d test superati, %d falliti\n", passes, fails);
    return fails ? 1 : 0;
}
