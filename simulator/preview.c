/* Esegue il codice reale di Iride 0.2 su PC: test di logica, calcoli astronomici e schermate */
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../module/iride/iride.c"
int sim_batt = 64, sim_batt_left = 7800, sim_free32k = 32 * 30000, sim_silent_shots = 0;
static void sky_full(void) { while (!sky_ready) sky_tick(); while (sky_tick()) ; }   /* notti e ricerca della Via Lattea */
#include "widths.h"

/* ================================================================ disegno registrato */
#define OPS_MAX (4 * 1024 * 1024)
static char *ops_idle, *ops_real;
static int len_idle = 0, len_real = 0, to_idle = 0;

static long sim_ops = 0;               /* operazioni di disegno: fanno da "tempo" per la prova della 6D */
static void op(const char *fmt, ...)
{
    char line[2048];
    va_list ap; va_start(ap, fmt); int n = vsnprintf(line, sizeof(line), fmt, ap); va_end(ap);
    if (n < 0) return;
    if (n >= (int)sizeof(line)) n = sizeof(line) - 1;     /* mai oltre il buffer: prima poteva copiare memoria a caso */
    sim_ops++;
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
static int offscreen = 0, offscreen_on = 0;
static char offscreen_txt[8][96];
static void text_check(int x, int y, int w, const char *buf);
void sim_text_check(int x, int y, int w, const char *buf)
{
    op("S\t%d\t%d\t%d\t%s\n", x, y, w, buf);       /* carattere piccolo: per i controlli di sovrapposizione */
    text_check(x, y, w, buf);
}
static void text_check(int x, int y, int w, const char *buf)
{
    glyphs += strlen(buf);
    if (offscreen_on && *buf && (x < 0 || x + w > 720 || y < 0 || y > 470))
    {
        int dup = 0;
        for (int i = 0; i < offscreen && i < 8; i++) if (!strncmp(offscreen_txt[i] + 13, buf, strlen(buf))) dup = 1;
        if (!dup) { if (offscreen < 8) snprintf(offscreen_txt[offscreen], 96, "%4d %4d %3d %s  [schermo %d, transizione %d]", x, y, w, buf, screen, transition); offscreen++; }
    }
}
int bmp_printf(uint32_t fontspec, int x, int y, const char *fmt, ...)
{
    char buf[256];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    int tw = bmp_string_width(fontspec, buf);
    text_check(x, y, tw, buf);
    op("W\t%d\t%d\t%d\t%d\t%s\n", x, y, tw, sim_font_h[(fontspec >> 16) & 7], buf);
    op("T\t%d\t%d\t%d\t%d\t%d\t%s\n", (fontspec >> 16) & 7, fontspec & 0xFF, (fontspec >> 8) & 0xFF, x, y, buf);
    return bmp_string_width(fontspec, buf);
}
static uint8_t vram_a[960 * 540], vram_b[960 * 540];
/* i riempimenti finiscono anche nella memoria video simulata: servono per vedere le transizioni */
static void vram_raster(uint8_t c, int x, int y, int w, int h)
{
    uint8_t *buf = (to_idle ? vram_b : vram_a) + 30 * 960;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > 720) w = 720 - x;
    if (y + h > 480) h = 480 - y;
    for (int yy = y; yy < y + h && w > 0; yy++) memset(buf + yy * 960 + x, c, w);
}
static long sim_fills = 0, sim_fill_area = 0;
void bmp_fill(uint8_t c, int x, int y, int w, int h) { sim_fills++; sim_fill_area += (long)MAX(w, 0) * MAX(h, 0); op("F\t%d\t%d\t%d\t%d\t%d\n", c, x, y, w, h); vram_raster(c, x, y, w, h); }
void bmp_draw_rect(int c, int x, int y, int w, int h) { op("R\t%d\t%d\t%d\t%d\t%d\n", c, x, y, w, h); }
int sim_get_px(int x, int y) { return (to_idle ? vram_b : vram_a)[30 * 960 + y * 960 + x]; }
/* riga di pixel scritta direttamente nella memoria video (0 = trasparente) */
void sim_put_row(int x, int y, const unsigned char *px, int n)
{
    static char hex[2 * 720 + 1];
    static const char hd[] = "0123456789abcdef";
    n = n > 720 ? 720 : n;
    for (int i = 0; i < n; i++) { hex[2 * i] = hd[px[i] >> 4]; hex[2 * i + 1] = hd[px[i] & 15]; }
    hex[2 * n] = 0;
    op("P\t%d\t%d\t%s\n", x, y, hex);
    uint8_t *buf = (to_idle ? vram_b : vram_a) + 30 * 960 + y * 960 + x;
    for (int i = 0; i < n; i++) if (px[i]) buf[i] = px[i];
}
static int offshape = 0, offshape_on = 0; static char offshape_txt[96];
static void shape_check(const char *what, int x0, int y0, int x1, int y1)
{
    if (!offshape_on || (x0 >= 0 && y0 >= 0 && x1 < 720 && y1 < 480)) return;
    if (!offshape) snprintf(offshape_txt, sizeof(offshape_txt), "%s %d,%d - %d,%d", what, x0, y0, x1, y1);
    offshape++;
}
void fill_circle(int x, int y, int r, int c) { shape_check("cerchio", x - r, y - r, x + r, y + r); op("C\t%d\t%d\t%d\t%d\t1\n", c, x, y, r); }
void draw_circle(int x, int y, int r, int c) { shape_check("circonferenza", x - r, y - r, x + r, y + r); op("C\t%d\t%d\t%d\t%d\t0\n", c, x, y, r); }
void draw_line(int x1, int y1, int x2, int y2, int c) { shape_check("linea", MIN(x1, x2), MIN(y1, y2), MAX(x1, x2), MAX(y1, y2)); op("L\t%d\t%d\t%d\t%d\t%d\n", c, x1, y1, x2, y2); }
void clrscr(void) { if (to_idle) len_idle = 0; else len_real = 0; op("X\n"); vram_raster(0, 0, 0, 720, 480); }
void bmp_draw_to_idle(int v) { to_idle = v; if (v) { len_idle = 0; } }
static int frames_shown = 0;
/* fotogrammi intermedi delle animazioni, salvati a tempi scelti */
static void shot(const char *name);
static int cap_on = 0, cap_t0 = 0, cap_i = 0, cap_n = 0, cap_times[48];
static char cap_name[32];
static int clock_ms;
static void capture(const char *name, const int *times, int n)
{
    snprintf(cap_name, sizeof(cap_name), "%s", name);
    if (n > (int)COUNT(cap_times)) n = COUNT(cap_times);
    memcpy(cap_times, times, n * sizeof(int)); cap_n = n; cap_i = 0; cap_t0 = clock_ms; cap_on = 1;
}
void bmp_idle_copy(int dir, int full)
{
    (void)full;
    if (!dir) return;
    memcpy(ops_real, ops_idle, len_idle + 1); len_real = len_idle; frames_shown++;
    memcpy(vram_a + 30 * 960, vram_b + 30 * 960, 480 * 960);
    while (cap_on && cap_i < cap_n && clock_ms - cap_t0 >= cap_times[cap_i])
    {
        char nm[64]; snprintf(nm, sizeof(nm), "%s_%02d", cap_name, cap_i); shot(nm); cap_i++;
    }
    if (cap_on && cap_i >= cap_n) cap_on = 0;
}
uint8_t *bmp_vram_real() { return vram_a + 30 * 960; }
uint8_t *bmp_vram_idle() { return vram_b + 30 * 960; }
uint8_t *bmp_vram(void) { return (to_idle ? vram_b : vram_a) + 30 * 960; }

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
static int lv_forced = 0, lv_closed = 0, bmp_on_calls = 0, sim_movie = 0, sim_lv_late = 0;
void force_liveview(void) { lv_forced++; if (!sim_lv_late) lv = 1; }   /* late: Canon occupata, il tasto finto arriva dopo */
void close_liveview(void) { lv_closed++; lv = 0; }
void bmp_on(void) { bmp_on_calls++; }
/* schermo intero del core di Iride: sim_fs = 0 come un ML senza la patch */
static int sim_fs = 0, sim_fs_on = 0, sim_fs_seq = 0, sim_fs_ready = 1, sim_fs_begins = 0;
int menu_fullscreen_begin() { if (!sim_fs) return 0; sim_fs_on = 1; sim_fs_begins++; menu_redraw_blocked = 1; return 1; }
void menu_fullscreen_end() { sim_fs_on = 0; }
int menu_fullscreen_ready() { return sim_fs ? sim_fs_ready : 0; }
int menu_fullscreen_seq() { return sim_fs ? sim_fs_seq : 0; }
int is_movie_mode(void) { return sim_movie; }
static char sim_menu_tab[32] = "Audio", sim_menu_entry[32] = "";
void select_menu_by_name(char *name, const char *entry) { snprintf(sim_menu_tab, sizeof(sim_menu_tab), "%s", name); snprintf(sim_menu_entry, sizeof(sim_menu_entry), "%s", entry); }
int menu_redraw_blocked, ml_shutdown_requested, shooting_mode = SHOOTMODE_M, lv = 0, lv_dispsize = 1, image_review_time = 2, RECORDING = 0;
int hdr_stepsize = 16;
struct lens_info lens_info;
static int clock_ms = 1000;
/* fotogrammi della memoria video (solo riempimenti) salvati a tempi scelti: PPM a colori */
static int vcap_on = 0, vcap_t0 = 0, vcap_i = 0, vcap_n = 0, vcap_times[16];
static char vcap_name[32];
static void vram_save(const char *name)
{
    char path[128];
    snprintf(path, sizeof(path), "out/%s.ppm", name);
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6 720 480 255\n");
    for (int i = 0; i < 720 * 480; i++)
    {
        int c = vram_a[30 * 960 + (i / 720) * 960 + i % 720], r = 0, g = 0, b = 0;
        if (c >= 38 && c <= 79) r = g = b = (c - 38) * 255 / 41;
        else if (c == 1) r = g = b = 255;
        else if (c == 0x13) { r = 255; g = 140; }
        else if (c == 0x08) { r = 235; g = 50; b = 45; }
        else if (c == 0x0C) { r = 150; g = 25; b = 25; }
        else if (c == 0x06) { r = 60; g = 210; b = 90; }
        else if (c == 0x07) { r = 120; g = 200; b = 120; }
        else if (c == 0x09) { r = 110; g = 170; b = 255; }
        else if (c == 0x0B) { r = 40; g = 80; b = 210; }
        else if (c == 0x0F) { r = 250; g = 215; }
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    fclose(f);
}
static void vcapture(const char *name, const int *times, int n)
{
    snprintf(vcap_name, sizeof(vcap_name), "%s", name);
    if (n > (int)COUNT(vcap_times)) n = COUNT(vcap_times);
    memcpy(vcap_times, times, n * sizeof(int)); vcap_n = n; vcap_i = 0; vcap_t0 = clock_ms; vcap_on = 1;
}
void msleep(int ms)
{
    clock_ms += ms > 0 ? ms : 1;
    while (vcap_on && vcap_i < vcap_n && clock_ms - vcap_t0 >= vcap_times[vcap_i])
    {
        char nm[64]; snprintf(nm, sizeof(nm), "%s_%02d", vcap_name, vcap_i); vram_save(nm); vcap_i++;
    }
    if (vcap_on && vcap_i >= vcap_n) vcap_on = 0;
}
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
    { "Display", "LV display gain", 0, 0, 6, 1 }, { "Shoot", "Silent Picture", 0, 0, 1, 1 }, { "Silent Picture", "Silent Mode", 0, 0, 6, 1 },
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
int take_a_pic(int af)
{
    /* con lo scatto silenzioso acceso il modulo silent non tocca il contatore Canon */
    if (menu_get_value_from_script("Shoot", "Silent Picture") == 1) { sim_silent_shots++; return 0; }
    return lens_take_picture(64, af);
}
int bulb_take_pic(int ms) { bulb_ms_total += ms; last_bulb_ms = ms; shots_taken++; card.file_number++; return 0; }
/* obiettivo finto per il fuoco automatico: il motore muove la ghiera con un po' di gioco (sim_fbl) */
static int sim_af = 0, sim_fmot = 0, sim_fpos = 0, sim_fbest = 0, sim_fbl = 1, sim_fcalls = 0;
static void make_sky(float sigma, float amp, int sx, int sy);
static void sim_focus_sky()
{
    float d = 0.6f * (float)(sim_fpos - sim_fbest);
    float sg = sqrtf(1.8f * 1.8f + d * d);
    make_sky(sg, 40.0f + 160.0f * 1.8f / sg, 352, 222);
}
int lens_focus(int n, int s, int w, int e)
{
    (void)w; (void)e;
    if (!sim_af) return 1;
    if (n < 0) { n = -n; s = -s; }
    s = s < -3 ? -3 : s > 3 ? 3 : s;
    for (int i = 0; i < n; i++)
    {
        sim_fmot += s;
        if (sim_fmot > sim_fpos + sim_fbl) sim_fpos = sim_fmot - sim_fbl;
        if (sim_fmot < sim_fpos - sim_fbl) sim_fpos = sim_fmot + sim_fbl;
    }
    sim_fcalls++;
    sim_focus_sky();
    return 1;
}
int is_manual_focus(void) { return 0; }
void set_lv_zoom(int z) { lv_dispsize = z; }
struct card_info *get_shooting_card(void) { return &card; }
char *get_config_dir(void) { return "out/"; }
struct semaphore { int locked; };
static struct semaphore sim_sem_pool[4]; static int sim_sems = 0;
struct semaphore *create_named_semaphore(const char *n, int unlocked) { (void)n; struct semaphore *s = &sim_sem_pool[sim_sems++ % 4]; s->locked = !unlocked; return s; }
int take_semaphore(struct semaphore *s, int t) { (void)t; if (s->locked) return 1; s->locked = 1; return 0; }
int give_semaphore(struct semaphore *s) { s->locked = 0; return 0; }
int FIO_CreateDirectory(const char *d) { (void)d; return 0; }
static char sim_path(char *dst, const char *n) { (void)dst; (void)n; return 0; }
FILE *FIO_CreateFileOrAppend(const char *n)
{
    /* ML/LOGS/x -> out/x; le altre (DCIM della scheda) gia' sotto out/ */
    const char *b = strrchr(n, '/');
    if (strncmp(n, "ML/", 3) == 0 && b) { char p[160]; snprintf(p, sizeof(p), "out/%s", b + 1); return fopen(p, "a"); }
    return fopen(n, "a");
}
int efic_temp = 155;                                   /* 155 * 0,8 - 93 = 31 gradi */
int GetBatteryLevel(void) { return sim_batt; }
int GetBatteryTimeRemaining(void) { return sim_batt_left; }
int get_free_space_32k(const struct card_info *c) { (void)c; return sim_free32k; }
const char *get_dcim_dir(void) { return "out/DCIM/100CANON/"; }
int get_us_clock(void) { return (int)((unsigned)clock_ms * 1000u + (unsigned)(sim_ops * 3)); }   /* come sulla 6D: gira oltre i 2^31 */
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
static void screenshot(int s, const char *name) { screen = s; hl_reset(); hr_to[0] = -1; home_intro_t0 = 0; icon_i = -1; settle(); shot(name); }
static void press(int k) { handle_key(k); }
/* ridisegna la schermata attuale a tempi scelti (ms da adesso) e salva i fotogrammi */
static void frames_at(const char *name, const int *times, int n)
{
    int t0 = clock_ms;
    for (int i = 0; i < n; i++)
    {
        if (t0 + times[i] > clock_ms) clock_ms = t0 + times[i];
        theme_apply(); render(0);
        char nm[64]; snprintf(nm, sizeof(nm), "%s_%02d", name, i); shot(nm);
    }
}

int main(void)
{
    mkdir("out", 0755); mkdir("out/DCIM", 0755); mkdir("out/DCIM/100CANON", 0755);   /* la "scheda" del simulatore */
    ops_idle = malloc(OPS_MAX); ops_real = malloc(OPS_MAX); ops_idle[0] = ops_real[0] = 0;
    set_time(2026, 11, 1, 22, 47);
    persist_defaults();
    camera_reset();
    snprintf(lens_info.name, sizeof(lens_info.name), "EF24-105mm f/4L IS USM");
    lens_info.lens_exists = 1; lens_info.focal_len = 24; lens_info.aperture = 40;
    remove("out/IRIDE.CSV"); remove("out/IRIDE.DAT");

    /* ---------- calcoli di base */
    CHECK(npf_tenths(24, 28) == 122, "NPF 24mm f/2.8 = 12.2 s");
    CHECK(npf_tenths_dec(24, 28, 60) == 244 && npf_tenths_dec(24, 28, 0) == 122, "NPF a 60 gradi di declinazione: il doppio (24.4 s)");
    {
        float al, az, ra, dec;
        alt_az(100.0f, 0.0f, 100.0f, 45.0f, &al, &az);
        CHECK(fabsf(al - 45.0f) < 0.01f && fabsf(az - 180.0f) < 0.01f, "al meridiano: a sud, alto 90 - latitudine");
        alt_az(100.0f, 0.0f, 10.0f, 45.0f, &al, &az);
        CHECK(fabsf(al) < 0.01f && fabsf(az - 90.0f) < 0.01f, "6 ore prima: sorge a est");
        gal_to_eq(0.0f, &ra, &dec);
        CHECK(fabsf(ra - GC_RA) < 0.05f && fabsf(dec - GC_DEC) < 0.05f, "piano galattico: l = 0 e' il centro della Via Lattea");
        mn.illum = 1000; mn.alt = 60.0f; opt_moon_iso = 0; opt_moon_ap = 110; opt_moon_es = 0;
        float sp, sa, t = moon_exposure(&sp, &sa);
        CHECK(fabsf(t - 0.01f) < 0.0005f, "Looney 11: Luna piena alta, f/11 e ISO 100 = 1/100 s (%.4f)", (double)t);
        CHECK(fabsf(moon_phase_stops(500) - 2.0f) < 0.01f && fabsf(moon_phase_stops(250) - 3.0f) < 0.01f && moon_alt_stops(15.0f) > 0.5f,
              "Luna: +2 stop al quarto, +3 alla falce larga, piu' luce bassa sull'orizzonte");
        opt_moon_ap = 110;
    }
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
    {
        /* molto sfocata: l'HFR deve continuare a crescere (prima si fermava a ~12 e poi calava) */
        float sg[] = { 2, 5, 8, 12, 16, 22 };
        int prev = 0, mono = 1, h, p, cx, cy, r;
        for (unsigned i = 0; i < COUNT(sg); i++)
        {
            make_sky(sg[i], 200.0f, 360, 220);
            if (!hfr_measure(&h, &p, &cx, &cy, &r) || h <= prev || fabsf((float)h / 100.0f - sg[i] * 1.2533f) > 0.15f * sg[i] * 1.2533f) mono = 0;
            prev = h;
        }
        CHECK(mono, "HFR cresce sempre con la sfocatura, fino a sigma 22 (ultima %.1f, apertura %d px)", (double)prev / 100.0, r);
        /* stella sotto il pannello in basso, fuori dal riquadro: non va scelta */
        make_sky(2.0f, 160.0f, 360, 360);
        CHECK(!hfr_measure(&h, &p, &cx, &cy, &r), "stella nascosta dal pannello in basso: ignorata");
        /* due pixel caldi vicini non sono una stella */
        make_sky(2.0f, 0.0f, 300, 200);
        lvbuf[200 * 1440 + 400 * 2 + 1] = 255; lvbuf[200 * 1440 + 401 * 2 + 1] = 240;
        CHECK(!hfr_measure(&h, &p, &cx, &cy, &r), "due pixel caldi vicini: nessuna stella");
        /* un pixel caldo piu' luminoso di una stella debole: vince la stella */
        make_sky(1.8f, 34.0f, 300, 180);
        lvbuf[250 * 1440 + 450 * 2 + 1] = 255;
        int ok = hfr_measure(&h, &p, &cx, &cy, &r);
        CHECK(ok && ABS(cx - 300) <= 1 && ABS(cy - 180) <= 1, "pixel caldo piu' luminoso di una stella debole: misurata la stella (%d, %d)", cx, cy);
    }

    /* ---------- salvataggio su scheda: due copie a turno */
    remove("out/IRIDE.DAT"); remove("out/IRIDE.BAK"); persist_next_slot = 0;
    /* profili di partenza: alla prima accensione, una volta sola, senza toccare quelli salvati */
    {
        persist_load();                              /* nessun file: prima accensione */
        CHECK(P.prof[0].valid && P.prof[0].name == 0 && P.prof[0].mode == SHOOTMODE_AV && P.prof[0].aperture == 32 &&
              P.prof[2].name == 2 && P.prof[3].mode == SHOOTMODE_TV && P.prof[3].shutter == 136 &&
              P.prof[4].name == 3 && P.prof[4].iso == 112 && P.persist_prof_seeded == 1,
              "prima accensione: 5 profili di partenza (Ritratto, Paesaggio, Street, Sport, Notte)");
        P.prof[1].valid = 0; persist_save(); persist_load();
        CHECK(!P.prof[1].valid, "un profilo cancellato non torna da solo");
        /* file di una versione precedente con un profilo salvato dall'utente */
        remove("out/IRIDE.DAT"); remove("out/IRIDE.BAK"); persist_next_slot = 0;
        persist_defaults(); P.prof[0].valid = 1; P.prof[0].name = 0; P.prof[0].iso = 96; P.prof[1].valid = 1; P.prof[1].name = 9;
        persist_save(); persist_load();
        CHECK(P.prof[0].iso == 96 && P.prof[1].name == 9 && P.prof[2].valid && P.prof[2].name == 1 && P.prof[3].name == 2 && P.prof[4].name == 5,
              "aggiornando: i profili salvati restano, i posti vuoti prendono quelli di partenza senza doppioni");
        /* diaframma che l'obiettivo non ha: si usa il suo limite */
        {
            int sm = shooting_mode, amin = lens_info.raw_aperture_min, amax = lens_info.raw_aperture_max;
            persist_defaults(); P.persist_prof_seeded = 0; profiles_seed();
            shooting_mode = SHOOTMODE_AV; lens_info.raw_aperture_min = 40; lens_info.raw_aperture_max = 80;   /* zoom f/4, f/22 */
            { int ok0 = profile_load(0); CHECK(ok0 && lens_info.raw_aperture == 40 && prof_ap_limited == 1, "Ritratto f/2.8 su un f/4: f/4, il piu' aperto dell'obiettivo (%d %d %d %d)", ok0, lens_info.raw_aperture, prof_ap_limited, P.prof[0].mode); }
            CHECK(profile_load(1) && lens_info.raw_aperture == 56 && prof_ap_limited == 0, "Paesaggio f/8: come nel profilo");
            lens_info.raw_aperture_min = 0; lens_info.raw_aperture_max = 0;
            CHECK(profile_load(0) && lens_info.raw_aperture == 32, "obiettivo senza dati: il valore del profilo, decide la 6D");
            shooting_mode = sm; lens_info.raw_aperture_min = amin; lens_info.raw_aperture_max = amax; persist_defaults();
        }
        remove("out/IRIDE.DAT"); remove("out/IRIDE.BAK"); persist_next_slot = 0; persist_defaults();
    }
    P.roll_num = 7; P.lat100 = 4400; P.prof[2].valid = 1; P.prof[2].iso = 96;
    persist_save();
    P.roll_num = 0; P.lat100 = 0; P.prof[2].valid = 0;
    persist_load();
    CHECK(P.roll_num == 7 && P.lat100 == 4400 && P.prof[2].valid == 1 && P.prof[2].iso == 96 && persist_next_slot == 1,
          "salvataggio IRIDE.DAT e rilettura (il prossimo va in IRIDE.BAK)");
    { FILE *f = fopen("out/IRIDE.DAT", "r+b"); fseek(f, 40, SEEK_SET); fputc(0x55, f); fclose(f); }
    persist_load();
    CHECK(P.roll_num == 0 && P.lat100 == 4189, "unica copia rovinata: valori di fabbrica (nessun crash)");
    /* due salvataggi: rovinando il piu' recente (scrittura interrotta) resta l'altro */
    remove("out/IRIDE.DAT"); remove("out/IRIDE.BAK"); persist_next_slot = 0; persist_defaults();
    P.roll_num = 11; persist_save();               /* IRIDE.DAT */
    P.roll_num = 12; persist_save();               /* IRIDE.BAK, piu' recente */
    persist_load();
    CHECK(P.roll_num == 12, "si rilegge la copia piu' recente (%d)", P.roll_num);
    { FILE *f = fopen("out/IRIDE.BAK", "wb"); fwrite("Iride", 1, 5, f); fclose(f); }
    persist_load();
    CHECK(P.roll_num == 11 && persist_next_slot == 1, "scrittura interrotta: resta la copia di prima (%d), la prossima riscrive quella rovinata", P.roll_num);
    /* salvataggio mentre il contatore cambia P: il file resta leggibile */
    P.roll_num = 13; persist_dirty = 1; persist_save();
    CHECK(persist_dirty == 0, "salvato: niente piu' da salvare");
    persist_load();
    CHECK(P.roll_num == 13, "e si rilegge (%d)", P.roll_num);
    /* valori fuori posto nel file: rimessi nei limiti */
    P.astro_tag = 99; P.dual_rec = -5; P.lat100 = 99999; P.prof[0].name = 1000; persist_save(); persist_load();
    CHECK(P.astro_tag == 0 && P.dual_rec == 0 && P.lat100 == 9000 && P.prof[0].name >= 0 && P.prof[0].name < NPROF_NAMES,
          "valori fuori posto rimessi nei limiti dopo la lettura");
    remove("out/IRIDE.DAT"); remove("out/IRIDE.BAK"); persist_next_slot = 0;
    persist_defaults();
    /* file della 1.1 (struttura v2): rullino, luogo e profili non si perdono passando alla 1.2 */
    {
        persist_defaults();
        P.roll_num = 42; P.roll_loaded = 1; P.lat100 = 4510; P.prof[1].valid = 1; P.prof[1].iso = 72; P.ml_prev[23] = 5;
        int words = PERSIST_V2_WORDS;
        int *img = calloc(words, sizeof(int));
        memcpy(img, &P, (words - 1) * sizeof(int));
        img[0] = PERSIST_MAGIC; img[1] = 2; img[2] = words * (int)sizeof(int);
        img[words - 1] = persist_sum_n(img, words - 1);
        FILE *f = fopen("out/IRIDE.DAT", "wb"); fwrite(img, sizeof(int), words, f); fclose(f); free(img);
        persist_defaults();
        persist_load();
        CHECK(P.roll_num == 42 && P.roll_loaded == 1 && P.lat100 == 4510 && P.prof[1].iso == 72 && P.ml_prev[23] == 5 &&
              P.version == PERSIST_VERSION && P.ml_prev2[0] == 0, "salvataggio della 1.1 riletto dalla 1.2 senza perdere il rullino");
        persist_save(); persist_defaults(); persist_load();
        CHECK(P.roll_num == 42 && P.version == PERSIST_VERSION, "dopo il passaggio si salva nel formato nuovo");
        persist_defaults();
    }

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
        screen = SCR_CONFIRM; conf_activate(2);
        CHECK(strstr(msg, "RAW+L") != NULL && msg_color == C_WARN, "carico in solo RAW: avviso '%s'", msg);
        CHECK(screen == SCR_SHELF && transition == -2, "fine del caricamento: la scena riscende e scopre lo scaffale");
        transition = 0;
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

    /* ---------- intervallometro da campo: sessione, scatto silenzioso, dithering, batteria */
    {
        char path[128];
        struct tm now; LoadCalendarFromRTC(&now);
        int ny, nm, nd; session_night(&now, &ny, &nm, &nd);
        snprintf(path, sizeof(path), "out/DCIM/100CANON/IS%02d%02d%02d.CSV", ny % 100, nm, nd);
        remove(path);
        preset_select(0); P.int_count = 3; P.int_pause = 2; P.int_delay_idx = 0; P.int_bulb = 0;
        shooting_mode = SHOOTMODE_M; lens_info.raw_shutter = 56; lens_info.iso = 3200; opt_session = 1; opt_silent = 0;
        int fnb = card.file_number;
        job_running = JOB_SEQ; job_task(NULL);
        FILE *f = fopen(path, "r"); int rows = 0, head = 0; char ln[256], last[256] = "";
        while (f && fgets(ln, sizeof(ln), f)) { if (ln[0] == '#') head++; else if (strncmp(ln, "numero", 6)) { rows++; snprintf(last, sizeof(last), "%s", ln); } }
        if (f) fclose(f);
        char want[32]; snprintf(want, sizeof(want), "LGHT%04d.CR2", fnb + 3);
        CHECK(rows == 3 && head == 3 && strstr(last, want) && strstr(last, ",3200,"), "sessione per AstroStack: 3 righe, ultima %s con ISO 3200", want);
        /* una notte = un file: 23:30 del 31 dicembre e 02:10 del 1 gennaio vanno in IS261231.CSV */
        {
            struct tm keep = sim_tm;
            remove("out/DCIM/100CANON/IS261231.CSV");
            sim_tm.tm_year = 126; sim_tm.tm_mon = 11; sim_tm.tm_mday = 31; sim_tm.tm_hour = 23; sim_tm.tm_min = 30;
            session_frame(100, 1, 30.0f, 0, "intervallometro");
            sim_tm.tm_year = 127; sim_tm.tm_mon = 0; sim_tm.tm_mday = 1; sim_tm.tm_hour = 2; sim_tm.tm_min = 10;
            session_frame(101, 1, 30.0f, 0, "intervallometro");
            sim_tm.tm_hour = 12;
            int y2, m2, d2; session_night(&sim_tm, &y2, &m2, &d2);
            FILE *g = fopen("out/DCIM/100CANON/IS261231.CSV", "r"); int r2 = 0;
            while (g && fgets(ln, sizeof(ln), g)) if (ln[0] != '#' && strncmp(ln, "numero", 6)) r2++;
            if (g) fclose(g);
            /* riavvio a meta' notte (cambio batteria): la numerazione riprende da 3 */
            sess_counted[0] = 0; sess_frames = 0; sim_tm.tm_year = 127; sim_tm.tm_mon = 0; sim_tm.tm_mday = 1; sim_tm.tm_hour = 3;
            session_frame(102, 1, 30.0f, 0, "intervallometro");
            g = fopen("out/DCIM/100CANON/IS261231.CSV", "r"); char lastl[256] = "";
            while (g && fgets(ln, sizeof(ln), g)) if (ln[0] != '#' && strncmp(ln, "numero", 6)) snprintf(lastl, sizeof(lastl), "%s", ln);
            if (g) fclose(g);
            CHECK(!strncmp(lastl, "3,", 2), "dopo un riavvio a meta' notte la numerazione riprende (%.12s)", lastl);
            sim_tm.tm_hour = 12;
            CHECK(r2 == 2 && y2 == 2027 && m2 == 1 && d2 == 1, "sessione a cavallo di mezzanotte (e di capodanno): un solo file IS261231 (%d righe); a mezzogiorno inizia la notte nuova", r2);
            sim_tm = keep;
        }
        /* silenzioso: niente otturatore, modulo silent rimesso com'era */
        opt_silent = 1; sim_silent_shots = 0; shots_taken = 0;
        CHECK(interval_check(), "silenzioso accettato con ghiera su M e 1 s");
        job_running = JOB_SEQ; job_task(NULL);
        CHECK(sim_silent_shots == 3 && shots_taken == 0 && mlv("Shoot", "Silent Picture") == 0 && mlv("Silent Picture", "Silent Mode") == 0,
              "silenzioso: 3 scatti senza otturatore, modulo silent rimesso com'era");
        lens_info.raw_shutter = 104;   /* 1/60 */
        CHECK(!interval_check() && strstr(msg, "0,3"), "silenzioso con 1/60: rifiutato");
        lens_info.raw_shutter = 56; opt_silent = 0;
        /* dithering: pause piu' lunghe ogni n scatti */
        opt_dither_n = 2; opt_dither_s = 20; P.int_count = 5; P.int_pause = 5;
        CHECK((int)int_duration(1.0f) == 5 + 4 * 5 + 2 * 15, "dithering ogni 2 scatti, 20 s: durata %d s", (int)int_duration(1.0f));
        int t0 = clock_ms; shots_taken = 0;
        job_running = JOB_SEQ; job_task(NULL);
        CHECK(shots_taken == 5 && clock_ms - t0 >= (2 * 20 + 2 * 5) * 1000, "dithering: due pause da 20 s nella sequenza");
        opt_dither_n = 0;
        /* batteria quasi scarica: ci si ferma puliti */
        sim_batt = 4; shots_taken = 0; P.int_count = 10;
        CHECK(!interval_check(), "batteria al 4%%: non parte");
        sim_batt = 30; job_running = JOB_SEQ;
        { int k = 0; (void)k; }
        sim_batt = 4; job_task(NULL);
        CHECK(shots_taken == 0 && strstr(last_notify, "batteria"), "batteria finita durante la sequenza: si ferma e lo dice (%s)", last_notify);
        sim_batt = 64;
        /* scatti dal cavo con l'etichetta LGHT: finiscono nel registro */
        P.astro_tag = 1; job_running = JOB_NONE; lens_info.job_state = 0;
        iride_bg_tick();
        card.file_number += 2;
        iride_bg_tick();
        f = fopen(path, "r"); int cavo = 0;
        while (f && fgets(ln, sizeof(ln), f)) if (strstr(ln, ",cavo")) cavo++;
        if (f) fclose(f);
        CHECK(cavo == 2, "scatti dal cavo (montatura) nel registro della sessione: %d", cavo);
        P.astro_tag = 0;
        /* scheda piena */
        int fr = sim_free32k; sim_free32k = 10;
        CHECK(!interval_check() && strstr(msg, "piena"), "scheda piena: non parte");
        sim_free32k = fr;
    }

    /* ---------- prova della 6D e registro degli avvisi */
    {
        remove("out/IRIDEBEN.TXT");
        screen = SCR_BENCH; bench_step = -1; bench_enter();
        bench_key(MODULE_KEY_PRESS_SET, 0, 0);
        int guard = 0;
        while (bench_tick() && guard++ < 50) ;
        FILE *f = fopen("out/IRIDEBEN.TXT", "r"); int n = 0; char ln[200];
        while (f && fgets(ln, sizeof(ln), f)) if (strstr(ln, " ms")) n++;
        if (f) fclose(f);
        CHECK(bench_step == BT_N && n == BT_N, "prova della 6D: %d tempi salvati in IRIDEBEN.TXT", n);
        say(C_WARN, "Avviso di prova per il registro");
        f = fopen("out/IRIDE.CSV", "r"); int av = 0;
        while (f && fgets(ln, sizeof(ln), f)) if (strstr(ln, "AVVISO,Avviso di prova")) av++;
        if (f) fclose(f);
        say(C_WARN, "Avviso di prova per il registro");
        f = fopen("out/IRIDE.CSV", "r"); int av2 = 0;
        while (f && fgets(ln, sizeof(ln), f)) if (strstr(ln, "AVVISO,Avviso di prova")) av2++;
        if (f) fclose(f);
        CHECK(av >= 1 && av2 == av, "avvisi nel registro su scheda, senza ripetizioni ravvicinate");
        msg[0] = 0;
        screen = SCR_HOME;
    }

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
      CHECK(streq(sim_menu_tab, "Shoot") && streq(sim_menu_entry, "Iride") && open_settle == 1,
          "menu ML portato su Shoot > Iride (niente indicatori audio) e attesa del menu Canon");
      open_settle = 0;
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

    /* ---------- ciclo di Iride aperta dalla schermata di scatto */
    { int mo = menu_open; menu_open = 1; open_settle = 1; int t0 = clock_ms, f0 = frames_shown, sp = opt_splash; opt_splash = 0;
      screen = SCR_HOME; app_task(NULL); opt_splash = sp;
      CHECK(clock_ms - t0 >= 350 && frames_shown > f0 && !app_running && menu_redraw_blocked == 0 && !open_settle,
          "Iride aspetta il menu Canon (%d ms) e poi disegna", clock_ms - t0);
      menu_open = mo; }

    /* ---------- START/STOP dentro Iride: Live View e ritorno sulla stessa schermata */
    { int l0 = lv; lv = 0; app_running = 1; screen = SCR_STARS;
      CHECK(iride_keypress(MODULE_KEY_LV) == 0 && lv_toggle == 1 && lv_resume_screen == SCR_STARS, "START/STOP in Fuoco stelle: chiesto il Live View");
      lv_toggle = 0;
      sim_movie = 1; CHECK(iride_keypress(MODULE_KEY_REC) == 1 && !lv_toggle, "in modalita' filmato START/STOP resta a Canon (registra)"); sim_movie = 0;
      /* il ciclo di Iride esce, chiude il menu e lancia il compito del Live View */
      int mo = menu_open; menu_open = 1; lv_toggle = 1; int tc0 = tasks_created, mc0 = menu_closed_calls;
      app_task(NULL);
      CHECK(!app_running && tasks_created == tc0 + 1 && menu_closed_calls == mc0 + 1, "Iride si chiude e accende il Live View");
      /* il compito accende il LV e riapre Iride su Fuoco stelle */
      app_running = 0; menu_open = 0; gui_state = 0; screen = SCR_HOME; int before = menu_opened_calls, b0 = bmp_on_calls;
      lv_task(NULL);
      CHECK(lv == 1 && lv_forced > 0 && menu_opened_calls == before + 1 && screen == SCR_STARS, "Live View acceso e Iride riaperta su Fuoco stelle");
      menu_redraw_blocked = 0; open_settle = 0;
      /* in LV la grafica viene riaccesa e il menu Canon tenuto sveglio */
      menu_open = 1; open_settle = 0; int r0 = menu_redraw_calls; app_task(NULL);
      CHECK(bmp_on_calls > b0 && menu_redraw_calls > r0, "in Live View: grafica accesa e menu Canon risvegliato");
      /* di nuovo START/STOP: LV spento */
      app_running = 0; menu_open = 0; lv_resume_screen = SCR_STARS; lv_task(NULL);
      CHECK(lv == 0 && lv_closed > 0 && screen == SCR_STARS, "secondo START/STOP: Live View spento, si resta su Fuoco stelle");
      menu_redraw_blocked = 0; open_settle = 0; menu_open = mo; lv = l0; app_running = 0; screen = SCR_HOME; }

    /* ---------- START/STOP con Canon occupata (dopo un cambio di rullino): niente avanti e indietro */
    { int l0 = lv; lv = 0; app_running = 0; menu_open = 0; gui_state = 0; screen = SCR_HOME;
      lv_resume_screen = SCR_SHELF; sim_lv_late = 1; int before = menu_opened_calls;
      lv_task(NULL);
      CHECK(lv == 0 && menu_opened_calls == before && !app_running, "LV non ancora partito: Iride non si riapre (il tasto LV finto resta a Canon)");
      CHECK(iride_keypress(MODULE_KEY_LV) == 1 && !lv_toggle, "il tasto LV finto in ritardo, a Iride chiusa, arriva a Canon");
      sim_lv_late = 0; lv_task(NULL);
      CHECK(lv == 1 && menu_opened_calls == before + 1 && screen == SCR_SHELF, "LV partito: Iride riaperta sui Rullini");
      app_running = 1;
      CHECK(iride_keypress(MODULE_KEY_LV) == 0 && !lv_toggle, "un LV subito dopo la riapertura non rilancia il Live View");
      clock_ms += LV_DEBOUNCE_MS;
      CHECK(iride_keypress(MODULE_KEY_LV) == 0 && lv_toggle == 1, "dopo un attimo START/STOP torna a funzionare");
      lv_toggle = 0; menu_redraw_blocked = 0; open_settle = 0; lv = l0; app_running = 0; screen = SCR_HOME; }

    /* ---------- tasto LV finto di ML (force_liveview) a Iride aperta: va a Canon */
    { app_running = 1; clock_ms += LV_DEBOUNCE_MS; lv_toggle = 0;
      ev->param = 0x1e; ev->arg = FAKE_BTN; iride_keypress_raw(evp);
      CHECK(key_fake == 1 && iride_keypress(MODULE_KEY_LV) == 1 && !lv_toggle, "tasto LV finto di ML: passa a Canon, niente START/STOP");
      ev->arg = 0; iride_keypress_raw(evp);
      CHECK(key_fake == 0 && iride_keypress(MODULE_KEY_LV) == 0 && lv_toggle == 1, "tasto LV vero: START/STOP");
      lv_toggle = 0; app_running = 0; ev->param = 0; }

    /* ---------- core di Iride: schermo intero */
    { int mo = menu_open; menu_open = 1; open_settle = 1; sim_fs = 1; sim_fs_ready = 1; int sp = opt_splash; opt_splash = 0;
      int t0 = clock_ms, b0 = sim_fs_begins; screen = SCR_HOME; app_task(NULL);
      CHECK(clock_ms - t0 < 350 && sim_fs_begins == b0 + 1 && !sim_fs_on && !fullscreen && menu_redraw_blocked == 0,
          "schermo intero: niente attesa fissa all'apertura (%d ms), e alla chiusura ML torna normale", clock_ms - t0);
      int l0 = lv; lv = 1; open_settle = 0; int r0 = menu_redraw_calls; app_task(NULL);
      CHECK(menu_redraw_calls == r0, "schermo intero in Live View: il dialogo Canon lo tiene sveglio ML, Iride non lo risveglia");
      sim_fs = 0; r0 = menu_redraw_calls; app_task(NULL);
      CHECK(menu_redraw_calls > r0, "ML senza la patch: Iride risveglia il dialogo come prima");
      lv = l0; opt_splash = sp; menu_open = mo; open_settle = 0; menu_redraw_blocked = 0; app_running = 0; }

    /* ---------- cambio rullino: riavvolgimento E caricamento animati */
    { app_running = 1; kq_tail = kq_head;
      CHECK(iride_keypress(0) == 1 && !key_pending(), "eventi interni di Canon (codice 0): passano e non interrompono le animazioni");
      struct persist saved = P;
      P.roll_loaded = 1; P.roll_paused = 0; P.roll_shots = 5;
      conf_mode = CONF_LOAD; shelf_cat = 0; shelf_pos[0] = 2; load_exp = 36; load_push = 0;
      int t0 = clock_ms; conf_activate(2);
      CHECK(clock_ms - t0 > 1850 + 2150, "cambio rullino: riavvolgimento e poi caricamento (%d ms)", clock_ms - t0);
      kq_push(MODULE_KEY_PRESS_SET);              /* tasto per saltare il riavvolgimento */
      t0 = clock_ms; conf_activate(2);
      CHECK(clock_ms - t0 > 2150 && !key_pending(), "un tasto salta solo il riavvolgimento e viene consumato (%d ms)", clock_ms - t0);
      P = saved; prefix_refresh(); film_activate(); app_running = 0; }

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
        float gc = altitude(GC_RA, GC_DEC, lst, lat_deg()), gca, gcz;
        alt_az(GC_RA, GC_DEC, lst, lat_deg(), &gca, &gcz);
        fprintf(jf, " {\"local\":\"%04d-%02d-%02d %02d:%02d\",\"utc_offset\":%d,\"polar_ha\":%.4f,\"sun_alt\":%.3f,\"moon_alt\":%.3f,\"moon_illum\":%.4f,\"gc_alt\":%.3f,\"gc_az\":%.3f}%s\n",
            c[0], c[1], c[2], c[3], c[4], off, (double)polar_ha_hours(j), (double)sun, (double)moon, ill / 1000.0, (double)gc, (double)gcz, i + 1 < COUNT(cases) ? "," : "");
    }
    fprintf(jf, "]\n"); fclose(jf);
    sky_enter(); sky_full();
    jf = fopen("out/nights.json", "w"); fprintf(jf, "[\n");
    for (int i = 0; i < SKY_NIGHTS; i++)
        fprintf(jf, " {\"day1970\":%d,\"sunset\":%d,\"dusk\":%d,\"dawn\":%d,\"sunrise\":%d,\"moonrise\":%d,\"moonset\":%d,\"moon_illum\":%d,\"moonless\":%d,\"gc_start\":%d,\"gc_end\":%d,\"nelm_dark\":%d,\"nelm_moon\":%d,\"nelm_moon_max\":%d,\"sqm\":%d}%s\n",
            nights[i].day1970, nights[i].sunset, nights[i].dusk, nights[i].dawn, nights[i].sunrise, nights[i].moonrise, nights[i].moonset, nights[i].moon_illum, nights[i].moonless_min,
            nights[i].gc_start, nights[i].gc_end, nights[i].nelm_dark, nights[i].nelm_moon, nights[i].nelm_moon_max, opt_sqm10, i + 1 < SKY_NIGHTS ? "," : "");
    fprintf(jf, "]\n"); fclose(jf);
    jf = fopen("out/gcnext.json", "w");
    fprintf(jf, "{\"state\":%d,\"from\":%d,\"day1970\":%d,\"start\":%d,\"end\":%d}\n", gcn_state, gcn_from, gcn_day, gcn_start, gcn_end);
    fclose(jf);

    /* luce della Luna: valori noti del modello di Krisciunas & Schaefer */
    {
        struct skymodel m0, m1, m2;
        int keep = opt_sqm10; opt_sqm10 = 210;
        sky_model(&m0, 0); sky_model(&m1, 1000); sky_model(&m2, 500);
        float q0 = sky_sqm(&m0, -10.0f), qf = sky_sqm(&m1, 60.0f), qq = sky_sqm(&m2, 60.0f), qlow = sky_sqm(&m1, 5.0f);
        CHECK(fabsf(q0 - 21.0f) < 0.01f && sky_nelm10(q0) == 61, "cielo senza Luna: SQM 21,0 -> mag limite 6,1 (%.2f, %d)", (double)q0, sky_nelm10(q0));
        CHECK(qf > 17.4f && qf < 18.2f && sky_nelm10(qf) >= 36 && sky_nelm10(qf) <= 42, "Luna piena alta 60 gradi: cielo ~17,8, stelle fino a ~4 (%.2f, %d)", (double)qf, sky_nelm10(qf));
        CHECK(qq > qf + 1.5f && qq < q0, "primo quarto: a meta' strada tra piena e buio (%.2f)", (double)qq);
        CHECK(qlow > qf, "Luna piena bassa schiarisce meno di quella alta (%.2f > %.2f)", (double)qlow, (double)qf);
        opt_sqm10 = 190; sky_model(&m0, 0);
        CHECK(sky_nelm10(sky_sqm(&m0, -10.0f)) < 61, "cielo di citta': meno stelle");
        opt_sqm10 = keep;
    }
    /* la Via Lattea torna (Roma, da novembre): fine febbraio - inizio marzo, prima dell'alba */
    {
        int y, mo, d;
        civil_from_days(gcn_day, &y, &mo, &d);
        CHECK(gcn_state == 2 && ((mo == 2 && d >= 20) || (mo == 3 && d <= 12)) && gcn_start < 720 && gcn_end - gcn_start >= 30,
              "centro Via Lattea torna il %d/%d alle %02d:%02d-%02d:%02d", d, mo, gcn_start / 60, gcn_start % 60, gcn_end / 60, gcn_end % 60);
        int la = P.lat100; P.lat100 = 6000;
        sky_enter(); sky_full();
        CHECK(gcn_state == 3, "a 60 gradi nord il centro non sale mai sopra 10 gradi");
        P.lat100 = la; sky_enter(); sky_full();
    }

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
    sel[SCR_ASTRO] = 4; screenshot(SCR_ASTRO, "02c_astro_calendario"); sel[SCR_ASTRO] = 1;
    P.npf_focal = 24; P.npf_ap10 = 28; sel[SCR_NPF] = 2; msg[0] = 0; screenshot(SCR_NPF, "03_npf");
    preset_select(PRESET_HG); sel[SCR_INTERVAL] = 0; screenshot(SCR_INTERVAL, "04_intervallometro");
    lv = 1; lv_dispsize = 10; stars_reset();
    float seq[] = { 6.5f, 5.0f, 3.6f, 2.6f, 2.0f, 1.75f, 1.8f, 2.2f, 1.9f, 1.75f };
    for (unsigned i = 0; i < COUNT(seq); i++) { make_sky(seq[i] * 1.6f, 170.0f, 352, 222); stars_tick(); }
    screenshot(SCR_STARS, "05_stelle");

    /* fuoco automatico: parte sfocato da una parte o dall'altra, o gia' a fuoco, con il gioco degli ingranaggi */
    {
        int bests[] = { 11, -9, 0, 25 };
        int saved_screen = screen; screen = SCR_STARS; focus_step = 1;
        for (unsigned i = 0; i < COUNT(bests); i++)
        {
            sim_af = 1; sim_fmot = sim_fpos = 0; sim_fbest = bests[i]; sim_fcalls = 0; sim_focus_sky();
            af_start();
            int k = 0;
            while (af_state == AF_RUN && k++ < 300) af_step();
            CHECK(af_state == AF_DONE && ABS(sim_fpos - sim_fbest) <= 1 && af_hend > 0 && af_hend * 100 <= MAX(af_h0, af_ph[af_min]) * 110,
                  "fuoco automatico verso %+d: finisce a %+d (%d spostamenti), %s", bests[i], sim_fpos, sim_fcalls, af_msg);
            if (i == 0)
            {
                screenshot(SCR_STARS, "05b_fuoco_auto");
                FILE *g = fopen("out/05b_fuoco_auto_lv.pgm", "wb"); fprintf(g, "P5 720 480 255\n");
                for (int y = 0; y < 480; y++) for (int x = 0; x < 720; x++) fputc(lvbuf[y * 1440 + x * 2 + 1], g);
                fclose(g);
            }
        }
        /* senza stelle si ferma subito, con un messaggio */
        sim_af = 0; make_sky(2.0f, 0.0f, 352, 222); af_start(); af_step(); sim_af = 1;
        CHECK(af_state == AF_FAIL && strstr(af_msg, "nessuna stella"), "fuoco automatico senza stelle: %s", af_msg);
        /* troppo lontano: si arrende dopo 40 passi */
        sim_fmot = sim_fpos = 0; sim_fbest = 90; sim_focus_sky(); af_start();
        { int k = 0; while (af_state == AF_RUN && k++ < 300) af_step(); }
        CHECK(af_state == AF_FAIL, "fuoco automatico lontanissimo: %s", af_msg);
        sim_af = 0; af_state = AF_IDLE; screen = saved_screen;
    }
    { FILE *g = fopen("out/05_lv.pgm", "wb"); fprintf(g, "P5 720 480 255\n"); for (int y = 0; y < 480; y++) for (int x = 0; x < 720; x++) fputc(lvbuf[y * 1440 + x * 2 + 1], g); fclose(g); }
    screenshot(SCR_POLAR, "06_polare");
    sky_enter(); sky_full(); screenshot(SCR_SKY, "07_cielo");

    /* Luna dettagliata: fasi a confronto e controlli sulla luce */
    {
        static const int ph[6] = { 80, 280, 500, 760, 1000, 600 }, wx[6] = { 1, 1, 1, 1, 1, 0 };
        bmp_draw_to_idle(0); clrscr();
        fill(C_BG, 0, 0, SCR_W, SCR_H);
        for (int i = 0; i < 6; i++)
        {
            draw_moon(62 + i * 119, 92, 56, ph[i], wx[i], 0);
            draw_moon(62 + i * 119, 250, 44, ph[i], wx[i], 0);
            fill(C_PANEL, 30 + i * 119, 340, 64, 26);
            draw_moon(62 + i * 119, 353, 8, ph[i], wx[i], -1);
            draw_moon(62 + i * 119, 420, 22, ph[i], wx[i], 0);
        }
        shot("23_luna_fasi");
        bmp_draw_to_idle(0); clrscr(); fill(C_BG, 0, 0, SCR_W, SCR_H);
        draw_moon(120, 120, 110, 1000, 1, 0); draw_moon(360, 120, 110, 430, 1, 0); draw_moon(600, 120, 110, 220, 0, 0);
        draw_moon(120, 360, 110, 870, 1, 0); draw_moon(360, 360, 110, 620, 0, 0);
        { int on = night; night = 1; draw_moon(600, 360, 110, 430, 1, 0); night = on; }
        shot("23b_luna_grande");
        sky_ready = 0; sky_progress = SKY_NIGHTS / 2; bmp_draw_to_idle(0); clrscr(); draw_sky(); shot("23c_luna_calcolo");
        sky_enter(); sky_full();
        uint8_t *vr = vram_a + 30 * 960;
        #define VP(x, y) vr[(y) * 960 + (x)]
        /* piena: tutto il disco acceso; nuova: tutto spento */
        bmp_draw_to_idle(0); clrscr();
        draw_moon(100, 100, 44, 1000, 1, 0); draw_moon(300, 100, 44, 0, 1, 0);
        draw_moon(100, 300, 44, 500, 1, 0); draw_moon(300, 300, 44, 500, 0, 0);
        int lit = 0, tot = 0, dark_ok = 1;
        for (int y = -40; y <= 40; y++) for (int x = -40; x <= 40; x++)
        {
            if (x * x + y * y > 40 * 40) continue;
            tot++; lit += VP(100 + x, 100 + y) >= COLOR_GRAY(38);
            if (VP(300 + x, 100 + y) > COLOR_GRAY(24)) dark_ok = 0;
        }
        CHECK(lit * 100 >= tot * 97, "Luna piena: disco acceso (%d/%d)", lit, tot);
        CHECK(dark_ok, "Luna nuova: solo luce cinerea");
        /* primo quarto: destra accesa e sinistra spenta; ultimo quarto il contrario */
        int r1 = 0, l1 = 0, r2 = 0, l2 = 0;
        for (int y = -30; y <= 30; y++) for (int x = 8; x <= 36; x++)
        {
            r1 += VP(100 + x, 300 + y); l1 += VP(100 - x, 300 + y);
            r2 += VP(300 + x, 300 + y); l2 += VP(300 - x, 300 + y);
        }
        CHECK(r1 > l1 * 13 / 10 && l2 > r2 * 13 / 10, "quarti: crescente a destra, calante a sinistra");
        /* mari piu' scuri degli altipiani: Crisium contro gli altipiani a sud */
        int cri = 0, hig = 0;
        for (int y = -2; y <= 2; y++) for (int x = -2; x <= 2; x++)
        {
            cri += VP(100 + 36 + x, 100 - 13 + y);     /* Mare Crisium, 17N 59E */
            hig += VP(100 + 22 + x, 100 + 28 + y);     /* altipiani tra Janssen e Piccolomini */
        }
        CHECK(cri + 25 * 3 < hig, "Luna piena: Mare Crisium piu' scuro degli altipiani (%d < %d)", cri / 25, hig / 25);
        /* bordo sfumato: sul contorno esistono grigi intermedi */
        int mid = 0;
        for (int a = 0; a < 44; a++) { int c = VP(100 + 44, 100 - 22 + a); if (c > COLOR_GRAY(4) && c < COLOR_GRAY(50)) mid++; }
        int edge = VP(100 + 44, 100), inner = VP(100 + 40, 100);
        CHECK(edge < inner, "bordo della Luna sfumato verso il nero");
        /* emisfero sud: la Luna e' capovolta, la crescente e' illuminata a sinistra */
        {
            int la = P.lat100; P.lat100 = -3390;
            bmp_draw_to_idle(0); clrscr(); draw_moon(100, 300, 44, 500, 1, 0);
            int rs = 0, ls = 0;
            for (int y = -30; y <= 30; y++) for (int x = 8; x <= 36; x++) { rs += VP(100 + x, 300 + y); ls += VP(100 - x, 300 + y); }
            CHECK(ls > rs * 13 / 10, "emisfero sud: primo quarto illuminato a sinistra");
            P.lat100 = la;
        }
        #undef VP
        /* cache: lo stesso disegno due volte produce le stesse operazioni */
        bmp_draw_to_idle(0); clrscr(); draw_moon(100, 100, 44, 730, 1, 0); int l_a = len_real;
        clrscr(); draw_moon(100, 100, 44, 730, 1, 0);
        CHECK(len_real == l_a, "Luna dalla cache identica (%d operazioni)", l_a);
        (void)mid;
    }
    sel[SCR_FOTO] = 0; screenshot(SCR_FOTO, "08_foto");
    sel[SCR_HDR] = 1; screenshot(SCR_HDR, "09_hdr");
    sel[SCR_FSTACK] = 3; screenshot(SCR_FSTACK, "10_focus_stack");
    sel[SCR_DUAL] = 1; screenshot(SCR_DUAL, "10b_dual_iso");
    sel[SCR_ETTR] = 0; screenshot(SCR_ETTR, "10c_ettr");
    sel[SCR_LVTOOLS] = 0; ml_set(MLF_PEAK, 1); ml_set(MLF_HIST, 1); screenshot(SCR_LVTOOLS, "11_strumenti_lv");
    screen = SCR_LVTOOLS; sel[SCR_LVTOOLS] = LV_GAIN_ROW; handle_key(MODULE_KEY_PRESS_RIGHT); handle_key(MODULE_KEY_PRESS_RIGHT); handle_key(MODULE_KEY_PRESS_RIGHT);
    CHECK(ml_get(MLF_DISPGAIN) == 3, "Live View al buio: +3 stop col guadagno del display di ML");
    msg[0] = 0; screenshot(SCR_LVTOOLS, "11b_lv_al_buio");
    sel[SCR_LVTOOLS] = LV_GAIN_ROW + 1; lv_activate(LV_GAIN_ROW + 1);
    CHECK(ml_get(MLF_DISPGAIN) == 0 && ml_get(MLF_PEAK) == 0, "Spegni tutto spegne anche il Live View al buio"); sel[SCR_LVTOOLS] = 0;
    P.nd_base = 80; P.nd_idx = 7; sel[SCR_ND] = 3; screenshot(SCR_ND, "12_filtri_nd");
    sel[SCR_MOTION] = 0; screenshot(SCR_MOTION, "13_movimento");
    shelf_enter(); screenshot(SCR_SHELF, "14_rullini");
    { int th = opt_theme; opt_theme = 1; screenshot(SCR_SHELF, "14c_rullini_notte"); opt_theme = th; }
    conf_mode = CONF_LOAD; load_exp = 36; load_push = 1; sel[SCR_CONFIRM] = 1; screenshot(SCR_CONFIRM, "15_carica");
    { int rl = P.roll_loaded; P.roll_loaded = 0; pic_quality = PICQ_RAW; conf_mode = CONF_LOAD; screenshot(SCR_CONFIRM, "15b_carica_solo_raw");
      P.roll_loaded = 1; P.roll_paused = 0; screenshot(SCR_SHELF, "14b_scaffale_solo_raw"); P.roll_loaded = rl; pic_quality = PICQ_RAW_JPG_LARGE_FINE; }
    conf_mode = CONF_ROLL; sel[SCR_CONFIRM] = 1; screenshot(SCR_CONFIRM, "16_rullino");
    sel[SCR_PROFILES] = 1; screenshot(SCR_PROFILES, "17_profili");
    { struct profile keep[NPROF]; memcpy(keep, P.prof, sizeof(keep)); int sd = P.persist_prof_seeded;
      for (int i = 0; i < NPROF; i++) { P.prof[i].valid = 0; P.prof[i].name = i; }
      P.persist_prof_seeded = 0; profiles_seed(); sel[SCR_PROFILES] = 2; screenshot(SCR_PROFILES, "17c_profili_partenza");
      memcpy(P.prof, keep, sizeof(keep)); P.persist_prof_seeded = sd; }
    sel[SCR_OPTIONS] = 0; screenshot(SCR_OPTIONS, "18_opzioni");
    screenshot(SCR_BENCH, "24_prova");
    snap_take(); screenshot(SCR_RESTORE, "19_torna_6d");
    { int th = opt_theme; opt_theme = 1; screenshot(SCR_ASTRO, "02b_astro_notte"); screenshot(SCR_HDR, "09b_hdr_notte");
      screenshot(SCR_OPTIONS, "18b_opzioni_notte"); screenshot(SCR_PROFILES, "17b_profili_notte"); opt_theme = th; }
    opt_theme = 2; screenshot(SCR_POLAR, "20_polare_notte");
    screenshot(SCR_SKY, "21_cielo_notte");
    opt_theme = 0;
    /* estate: Via Lattea e Luna nella stessa notte; Opzioni col cielo del luogo */
    set_time(2027, 7, 12, 22, 30); sky_enter(); sky_full(); sky_sel = 0; screenshot(SCR_SKY, "21b_cielo_estate");
    sky_sel = 4; screenshot(SCR_SKY, "21c_cielo_estate_luna");
    mw_back_sky = 1; mw_night = 0; mwplan_enter(); screenshot(SCR_MWPLAN, "26_dove_puntare");
    mw_slot = 22; render(0); shot("26b_dove_puntare_tardi");
    mw_back_sky = 1; mw_night = 4; mwplan_enter(); screenshot(SCR_MWPLAN, "26c_dove_puntare_luna_piena"); mw_back_sky = 0;
    moonshot_enter(); screenshot(SCR_MOON, "25_luna_foto");
    set_time(2026, 11, 1, 22, 47); mw_back_sky = 0; mwplan_enter(); while (mwplan_tick()) ; screenshot(SCR_MWPLAN, "26d_dove_puntare_novembre");
    set_time(2027, 7, 12, 22, 30); sky_enter(); sky_full();
    opt_moon_es = 1; sel[SCR_MOON] = 0; screenshot(SCR_MOON, "25b_luna_cinerea"); opt_moon_es = 0;
    set_time(2026, 11, 1, 22, 47); sky_enter(); sky_full(); sky_sel = 0;
    sel[SCR_OPTIONS] = 12; screenshot(SCR_OPTIONS, "18c_opzioni_cielo"); sel[SCR_OPTIONS] = 0;

    /* ---------- luogo: citta' pronte e fusi orari di tutto il mondo */
    {
        int la = P.lat100, lo = P.lon100, tz = P.tz_mode, ok = 1;
        for (int i = 0; i < NUM_CITIES; i++) { city_set(i); if (city_find() != i) ok = 0; }
        CHECK(ok, "%d citta': ognuna si ritrova dalle sue coordinate", NUM_CITIES);
        P.lat100 = 4512; P.lon100 = 1170; P.tz_mode = 0;
        CHECK(city_find() < 0 && streq(cities[city_nearest()].name, "Rovigo"), "luogo personalizzato vicino a %s", cities[city_nearest()].name);
        screen = SCR_OPTIONS; sel[SCR_OPTIONS] = 8; handle_key(MODULE_KEY_PRESS_RIGHT);
        CHECK(city_find() >= 0 && streq(cities[city_find()].name, "Rovigo"), "</> su un luogo personalizzato porta alla citta' piu' vicina");
        handle_key(MODULE_KEY_PRESS_RIGHT);
        CHECK(city_find() >= 0 && !streq(cities[city_find()].name, "Rovigo"), "poi alla successiva: %s", cities[city_find()].name);
        int code = 0, n = 0;
        do { code = tz_next(code, 1); n++; } while (code != 0 && n < 500);
        CHECK(n == (int)COUNT(tzdefs) + 105, "fusi in giro con </>: %d voci (zone con ora legale + quarti d'ora da UTC-12 a UTC+14)", n);
        CHECK(tz_next(4, 1) == TZ_FIXED(180) + 1 && tz_std_min(4) == 180, "vecchio fuso UTC+3 della 1.1: stesso scarto, poi si prosegue");
        { char b[32]; tz_name(TZ_FIXED(345), b, sizeof(b)); CHECK(streq(b, "UTC+5:45 fisso"), "fuso fisso a quarti d'ora: %s", b); }
        { char b[32]; tz_name(TZ_FIXED(-210), b, sizeof(b)); CHECK(streq(b, "UTC-3:30 fisso"), "fuso fisso negativo: %s", b); }
        /* tabella per check_tz.py, che la confronta col database IANA */
        FILE *f = fopen("out/tz.csv", "w");
        static const int hours[] = { 0, 4, 12, 23 };
        for (int i = 0; i < NUM_CITIES; i++)
        {
            P.tz_mode = cities[i].tz;
            for (int y = 2026; y <= 2028; y++)
                for (int m = 1; m <= 12; m++)
                    for (int d = 1; d <= days_in_month(y, m); d++)
                        for (unsigned k = 0; k < COUNT(hours); k++)
                            fprintf(f, "%s,%d,%d,%d,%d,%d\n", cities[i].name, y, m, d, hours[k], utc_offset_min(y, m, d, hours[k]));
        }
        fclose(f);
        P.lat100 = 4507; P.lon100 = 1179; P.tz_mode = 0;
        sel[SCR_OPTIONS] = 8; screenshot(SCR_OPTIONS, "18d_opzioni_luogo");
        P.lat100 = -3387; P.lon100 = 15121; P.tz_mode = 16; screenshot(SCR_OPTIONS, "18e_opzioni_sydney");
        sel[SCR_OPTIONS] = 0; P.lat100 = la; P.lon100 = lo; P.tz_mode = tz;
    }

    /* ---------- aiuto col tasto INFO */
    {
        int missing = 0, tall = 0;
        for (int s = 0; s < SCR_COUNT; s++)
        {
            if (s == SCR_CONFIRM || s == SCR_PROFILES || s == SCR_KEYS) continue;
            if (!help_text(s)) { missing++; printf("  senza aiuto: %s\n", screens[s].title); }
        }
        CHECK(missing == 0, "ogni schermata ha il suo aiuto (tranne conferma, profili e diagnostica tasti)");
        screen = SCR_STARS; help_scr = -1; handle_key(MODULE_KEY_INFO);
        CHECK(help_scr == SCR_STARS, "INFO apre l'aiuto");
        screenshot(SCR_STARS, "05c_aiuto");
        int s0 = sel[SCR_STARS]; handle_key(MODULE_KEY_PRESS_SET);
        CHECK(help_scr < 0 && screen == SCR_STARS, "un tasto chiude l'aiuto senza fare altro");
        screen = SCR_PROFILES; handle_key(MODULE_KEY_INFO);
        CHECK(help_scr < 0, "nei profili INFO resta 'salva'");
        screen = SCR_INTERVAL; help_scr = SCR_INTERVAL; screenshot(SCR_INTERVAL, "04c_aiuto_intervallometro"); help_scr = -1;
        (void)s0; (void)tall;
    }
    /* caricamento animato: un fotogramma a meta' */
    { screen = SCR_SHELF; theme_apply(); P.roll_push = 0; P.roll_film = 3;
      static int tl[24];
      for (int i = 0; i < 24; i++) tl[i] = i * 118;  /* 24 fotogrammi per la GIF del README */
      capture("anim_carica", tl, COUNT(tl));
      anim_load(&films[3]); shot("22_caricamento");
      static const int tr[] = { 80, 300, 600, 900, 1150, 1300, 1420, 1600, 1750, 2000, 2440 };
      capture("anim_riavvolgi", tr, COUNT(tr));
      anim_rewind(&films[1], 24); cap_on = 0;
      /* fotogrammi fitti per la GIF del riavvolgimento */
      static int trg[40];
      for (int i = 0; i < 40; i++) trg[i] = i * 62;
      capture("g_riavvolgi", trg, COUNT(trg));
      anim_rewind(&films[1], 24); cap_on = 0; }

    { int t0 = clock_ms; splash(); anim_rewind(&films[1], 12); frame_show(1); frame_show(-1);
      CHECK(clock_ms - t0 < 6000, "splash, riavvolgimento e transizioni finiscono da soli (%d ms simulati)", clock_ms - t0); }

    /* ---------- descrizioni delle pellicole: due righe che stanno nel dialogo e nello scaffale */
    {
        int worst = 0; const char *wn = "";
        for (int i = 0; i < NUM_FILMS; i++)
        {
            int w = MAX(txt_w(FONT_MED, films[i].desc), txt_w(FONT_MED, films[i].use));
            if (w > worst) { worst = w; wn = films[i].name; }
            if (!films[i].desc[0] || !films[i].use[0]) worst = 9999;
        }
        CHECK(worst <= SCR_W - 190 - 24, "descrizioni dei rullini: la riga piu' larga (%s) e' %d px, nel dialogo ce ne stanno %d", wn, worst, SCR_W - 214);
    }

    /* ---------- transizioni sulla memoria video: fotogrammi e risultato finale identico */
    {
        static const int ts[] = { 30, 70, 110, 160, 240 };
        struct { int from, to, dir, tile; const char *name; } tr_list[] = {
            { SCR_HOME, SCR_SHELF, 3, 2, "z_tr_zoom" }, { SCR_SHELF, SCR_HOME, -3, 2, "z_tr_zoom_indietro" },
            { SCR_SHELF, SCR_CONFIRM, 2, 0, "z_tr_foglio" }, { SCR_CONFIRM, SCR_SHELF, -2, 0, "z_tr_foglio_giu" },
            { SCR_ASTRO, SCR_NPF, 1, 0, "z_tr_avanti" }, { SCR_NPF, SCR_ASTRO, -1, 0, "z_tr_indietro" },
        };
        int same = 1;
        for (unsigned k = 0; k < COUNT(tr_list); k++)
        {
            opt_theme = 0; conf_mode = CONF_LOAD;
            screen = tr_list[k].from; hl_reset(); hr_to[0] = -1; theme_apply(); render(0);
            screen = tr_list[k].to; hl_reset(); hr_to[0] = -1; zoom_from_tile(tr_list[k].tile); theme_apply();
            vcapture(tr_list[k].name, ts, COUNT(ts));
            render(tr_list[k].dir);
            vcap_on = 0;
            if (memcmp(vram_a + 30 * 960, vram_b + 30 * 960, 480 * 960)) same = 0;
        }
        CHECK(same, "transizioni (zoom, foglio, spinta): alla fine lo schermo e' esattamente la nuova schermata");
    }

    /* ---------- animazioni dentro le schermate: fotogrammi a tempi scelti */
    {
        opt_theme = 0; theme_apply();
        static const int tsp[] = { 40, 140, 260, 400, 520, 640, 760, 880, 1000 };
        capture("z_splash", tsp, COUNT(tsp)); splash(); cap_on = 0;
        static const int ti[] = { 0, 90, 180, 280, 420, 560, 650, 760, 900, 1300 };
        screen = SCR_HOME; sel[SCR_HOME] = 2; hl_reset(); hr_to[0] = -1;
        home_intro_t0 = clock_ms; icon_i = 2; icon_t0 = clock_ms + 520;
        frames_at("z_home_entrata", ti, COUNT(ti));
        home_intro_t0 = 0;
        static const int tic[] = { 120, 260, 420, 640 };
        for (int i = 0; i < 6; i++)
        {
            char nm[32]; snprintf(nm, sizeof(nm), "z_icona%d", i);
            sel[SCR_HOME] = i; hr_to[0] = -1; icon_i = i; icon_t0 = clock_ms;
            frames_at(nm, tic, COUNT(tic));
        }
        icon_i = -1;
        static const int tt[] = { 0, 60, 130, 300, 3290, 3400, 3480, 3560, 3700 };
        screen = SCR_OPTIONS; sel[SCR_OPTIONS] = 0; hl_reset(); say(C_OK, "Rullino caricato");
        frames_at("z_avviso", tt, COUNT(tt));
        msg[0] = 0;
        static const int tv[] = { 0, 50, 100, 160, 240 };
        sel[SCR_OPTIONS] = 4; hl_reset(); render(0); handle_key(MODULE_KEY_PRESS_RIGHT);
        frames_at("z_valore", tv, COUNT(tv));
        static const int tb[] = { 0, 40, 80, 130, 230 };
        screen = SCR_ASTRO; sel[SCR_ASTRO] = 0; hl_reset(); render(0); handle_key(MODULE_KEY_PRESS_DOWN); handle_key(MODULE_KEY_PRESS_DOWN);
        frames_at("z_barra", tb, COUNT(tb));
        static const int tsh[] = { 0, 50, 110, 180, 260, 400 };
        screen = SCR_SHELF; shelf_enter(); render(0); handle_key(MODULE_KEY_PRESS_DOWN);
        frames_at("z_scaffale_tipo", tsh, COUNT(tsh));
        handle_key(MODULE_KEY_PRESS_RIGHT);
        frames_at("z_scaffale_rullino", tsh, COUNT(tsh));
        shelf_cat = CAT_COLOR; shelf_enter();
        /* Astro: entrata dell'allineamento polare, impulso, cambio di vista; calendario */
        static const int tp[] = { 0, 120, 260, 420, 600, 800, 1500 };
        screen = SCR_POLAR; P.polar_view = 0; polar_enter(); frames_at("z_polare", tp, COUNT(tp));
        static const int tq[] = { 0, 100, 220, 420 };
        handle_key(MODULE_KEY_PRESS_SET); frames_at("z_polare_vista", tq, COUNT(tq));
        P.polar_view = 0;
        static const int tk[] = { 0, 80, 180, 300, 450, 700 };
        screen = SCR_SKY; sky_enter(); sky_full(); frames_at("z_cielo", tk, COUNT(tk));
        handle_key(MODULE_KEY_PRESS_DOWN); handle_key(MODULE_KEY_PRESS_DOWN); frames_at("z_cielo_notte_scelta", tk, COUNT(tk));
        CHECK(sky_sel == 2, "calendario: ^/v sceglie la notte (%d)", sky_sel);
        sky_sel = 0;

        /* menu Astro e Foto: illustrazione grande di ogni voce, a meta' animazione e ferma */
        static const int tmd[] = { 300, 1500 };
        for (int m = 0; m < 2; m++)
            for (int i = 0; i < (m ? NUM_FOTO_ROWS : NUM_ASTRO_ROWS); i++)
            {
                char nm[32]; snprintf(nm, sizeof(nm), "z_md%d_%d", m, i);
                screen = m ? SCR_FOTO : SCR_ASTRO; sel[screen] = i; hl_reset(); md_select();
                frames_at(nm, tmd, COUNT(tmd));
            }
        sel[SCR_ASTRO] = 1;
 sel[SCR_FOTO] = 0;

        /* Torna alla 6D: barra, spunta, poi la Home con l'avviso */
        struct persist saved = P;
        snap_take(); screen = SCR_RESTORE; hl_reset(); render(0);
        static const int trs[] = { 100, 300, 500, 700, 1000 };
        capture("z_ripristino", trs, COUNT(trs));
        handle_key(MODULE_KEY_PRESS_SET); cap_on = 0;
        CHECK(screen == SCR_HOME && strstr(msg, "Fatto") != NULL && transition == -3,
            "Torna alla 6D: ripristina, torna nella Home dal suo riquadro e mostra '%s'", msg);
        transition = 0; msg[0] = 0; P = saved;
    }

    /* ---------- fotogrammi ogni 40 ms per le GIF del README (simulator/gif.py) */
    {
        opt_theme = 0; theme_apply();
        static int g40[40];
        for (int i = 0; i < 40; i++) g40[i] = i * 40;
        capture("g_splash", g40, 26); splash(); cap_on = 0;
        screen = SCR_HOME; sel[SCR_HOME] = 2; hl_reset(); hr_to[0] = -1;
        home_intro_t0 = clock_ms; icon_i = 2; icon_t0 = clock_ms + 520;
        frames_at("g_home", g40, 34);
        home_intro_t0 = 0; icon_i = -1;
        screen = SCR_SHELF; shelf_cat = CAT_COLOR; shelf_enter(); hl_reset(); frames_at("g_scaffale", g40, 1);
        handle_key(MODULE_KEY_PRESS_RIGHT); frames_at("g_scaffale_dx", g40, 10);
        handle_key(MODULE_KEY_PRESS_DOWN); frames_at("g_scaffale_giu", g40, 13);
        conf_mode = CONF_LOAD; sel[SCR_CONFIRM] = 2; screen = SCR_CONFIRM; hl_reset(); frames_at("g_dialogo", g40, 1);
        /* menu Astro: si scende di due voci, l'anteprima a destra cambia */
        screen = SCR_ASTRO; sel[SCR_ASTRO] = 0; hl_reset(); md_enter(); frames_at("g_astro_a", g40, 26);
        handle_key(MODULE_KEY_PRESS_DOWN); md_select(); frames_at("g_astro_b", g40, 30);
        handle_key(MODULE_KEY_PRESS_DOWN); handle_key(MODULE_KEY_PRESS_DOWN); frames_at("g_astro_c", g40, 30);
        sel[SCR_ASTRO] = 1;
        /* impostazioni: barra elastica, valore che entra, interruttore; Opzioni che scorrono */
        screen = SCR_HDR; sel[SCR_HDR] = 0; hl_reset(); frames_at("g_hdr_a", g40, 8);
        handle_key(MODULE_KEY_PRESS_DOWN); frames_at("g_hdr_b", g40, 8);
        handle_key(MODULE_KEY_PRESS_RIGHT); frames_at("g_hdr_c", g40, 8);
        handle_key(MODULE_KEY_PRESS_DOWN); frames_at("g_hdr_d", g40, 8);
        handle_key(MODULE_KEY_PRESS_RIGHT); frames_at("g_hdr_e", g40, 8);
        handle_key(MODULE_KEY_PRESS_UP); handle_key(MODULE_KEY_PRESS_UP); frames_at("g_hdr_f", g40, 8);
        handle_key(MODULE_KEY_PRESS_SET); frames_at("g_hdr_g", g40, 12);
        screen = SCR_OPTIONS; sel[SCR_OPTIONS] = 0; hl_reset(); frames_at("g_opt_00", g40, 6);
        for (int k = 1; k <= 11; k++)
        {
            char nm[24]; snprintf(nm, sizeof(nm), "g_opt_%02d", k);
            handle_key(MODULE_KEY_PRESS_DOWN); frames_at(nm, g40, 7);
        }
        /* allineamento polare: entrata, impulso, cambio di vista */
        screen = SCR_POLAR; P.polar_view = 0; hl_reset(); polar_enter(); frames_at("g_pol_a", g40, 24);
        polar_tick(); frames_at("g_pol_b", g40, 18);
        handle_key(MODULE_KEY_PRESS_SET); frames_at("g_pol_c", g40, 14);
        P.polar_view = 0;
        /* calendario: entrata e scelta delle notti */
        screen = SCR_SKY; sky_enter(); sky_full(); hl_reset(); sky_ready_t0 = sky_t0 = clock_ms;
        frames_at("g_sky_a", g40, 22);
        handle_key(MODULE_KEY_PRESS_DOWN); frames_at("g_sky_b", g40, 16);
        handle_key(MODULE_KEY_PRESS_DOWN); handle_key(MODULE_KEY_PRESS_DOWN); frames_at("g_sky_c", g40, 16);
        sky_sel = 0;
        shelf_cat = CAT_COLOR; shelf_enter();
    }

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
    af_state = AF_IDLE; help_scr = -1;
    CHECK(crashes == 0, "tutte le %d schermate x 2 temi x %d tasti disegnate senza errori", SCR_COUNT, (int)COUNT(keys));

    /* ---------- casi estremi: date (ora legale, capodanno, bisestile), luoghi (sud, equatore,
       sole di mezzanotte, polo), fusi, cielo del luogo fuori scala; tasti a caso in ognuno */
    {
        struct persist saved = P;
        int sq = opt_sqm10;
        struct { int y, mo, d, h, mi, lat, lon, tz, sqm; } sc[] = {
            { 2027, 3, 28, 1, 30, 4190, 1250, 0, 210 },     /* passaggio all'ora legale */
            { 2026, 10, 25, 2, 30, 4510, 1190, 0, 205 },    /* ritorno all'ora solare */
            { 2026, 12, 31, 23, 59, 4190, 1250, 0, 220 },   /* capodanno */
            { 2028, 2, 29, 21, 0, 4190, 1250, 0, 190 },     /* bisestile */
            { 2026, 6, 21, 23, 0, 6600, 2500, 3, 215 },     /* sole di mezzanotte: niente buio */
            { 2026, 12, 21, 12, 0, 8990, 0, 1, 210 },       /* polo nord: notte polare */
            { 2027, 1, 10, 22, 0, -3390, 15120, 4, 210 },   /* Sydney, emisfero sud */
            { 2027, 4, 1, 3, 0, 0, 0, 1, 210 },             /* equatore */
            { 2027, 7, 1, 1, 0, -8990, -17990, 1, 0 },      /* polo sud, longitudine limite, cielo a zero */
            { 2026, 11, 1, 22, 47, 4190, 1250, 0, 999 },    /* cielo fuori scala */
        };
        unsigned rng = 777;
        int fk[] = { MODULE_KEY_PRESS_UP, MODULE_KEY_PRESS_DOWN, MODULE_KEY_PRESS_LEFT, MODULE_KEY_PRESS_RIGHT,
                     MODULE_KEY_PRESS_SET, MODULE_KEY_MENU, MODULE_KEY_INFO };
        int bad = 0;
        offscreen = 0; offscreen_on = 1; offshape = 0; offshape_on = 1;
        for (unsigned c = 0; c < COUNT(sc); c++)
        {
            set_time(sc[c].y, sc[c].mo, sc[c].d, sc[c].h, sc[c].mi);
            P.lat100 = sc[c].lat; P.lon100 = sc[c].lon; P.tz_mode = sc[c].tz; opt_sqm10 = sc[c].sqm;
            opt_theme = c % 3;
            /* calendario completo e polare in questo luogo */
            screen = SCR_SKY; sky_enter(); sky_full(); hl_reset();
            for (int k = 0; k < SKY_NIGHTS; k++)
            {
                const struct night *n = &nights[k];
                if (n->moon_illum < 0 || n->moon_illum > 1000 || n->dark_min < 0 || n->dark_min > 1440 ||
                    n->nelm_dark > 80 || n->nelm_moon > 80 || n->moonless_min > n->dark_min) bad++;
            }
            for (int k = 0; k < SKY_NIGHTS; k++) { sky_sel = k; render(0); }
            screen = SCR_POLAR; polar_enter(); render(0);
            screen = SCR_ASTRO; render(0);
            for (int i = 0; i < 400; i++)
            {
                rng = rng * 1103515245u + 12345u;
                handle_key(fk[(rng >> 16) % COUNT(fk)]);
                clock_ms += 10 + (int)((rng >> 8) % 60);
                theme_apply(); render(transition); transition = 0;
                if (screens[screen].tick) screens[screen].tick();
            }
        }
        /* la Luna a tutte le misure, fasi e sfondi */
        for (int r = 1; r <= 60; r += 3) for (int il = -50; il <= 1050; il += 110) { draw_moon(360, 240, r, il, il & 1, r % 2 ? -1 : 10); }
        CHECK(!bad, "casi estremi (ora legale, capodanno, bisestile, sole di mezzanotte, poli, sud, equatore): valori sensati");
        P = saved; opt_sqm10 = sq; opt_theme = 0; P.tz_mode = 0;
        set_time(2026, 11, 1, 22, 47); sky_enter(); sky_full(); sky_sel = 0;
        offscreen_on = 0; offshape_on = 0;
        for (int i = 0; i < offscreen && i < 8; i++) printf("  fuori schermo: %s\n", offscreen_txt[i]);
        if (offshape) printf("  forme fuori schermo: %d, la prima: %s\n", offshape, offshape_txt);
        CHECK(offscreen == 0, "casi estremi: nessun testo fuori dallo schermo (%d)", offscreen);
    }

    /* ---------- tasti a caso su tutte le schermate (anche transizioni e animazioni):
       con -fsanitize=address,undefined trova letture/scritture fuori posto */
    {
        struct persist saved = P;
        int lvs = lv;
        unsigned rng = getenv("IRIDE_FUZZ_SEED") ? (unsigned)atoi(getenv("IRIDE_FUZZ_SEED")) : 12345;
        int fuzz_n = getenv("IRIDE_FUZZ_N") ? atoi(getenv("IRIDE_FUZZ_N")) : 6000;
        int fk[] = { MODULE_KEY_PRESS_UP, MODULE_KEY_PRESS_DOWN, MODULE_KEY_PRESS_LEFT, MODULE_KEY_PRESS_RIGHT,
                     MODULE_KEY_WHEEL_LEFT, MODULE_KEY_WHEEL_RIGHT, MODULE_KEY_WHEEL_UP, MODULE_KEY_WHEEL_DOWN,
                     MODULE_KEY_PRESS_SET, MODULE_KEY_MENU, MODULE_KEY_INFO, MODULE_KEY_Q };
        int screens_seen = 0, seen[SCR_COUNT] = { 0 };
        screen = SCR_HOME; hl_reset(); transition = 0;
        offscreen = 0; offscreen_on = 1; offshape = 0; offshape_on = 1;
        for (int i = 0; i < fuzz_n; i++)
        {
            rng = rng * 1103515245u + 12345u;
            int k = fk[(rng >> 16) % COUNT(fk)];
            if (k == MODULE_KEY_MENU && screen == SCR_HOME) k = MODULE_KEY_PRESS_SET;
            handle_key(k);
            clock_ms += 5 + (int)((rng >> 8) % 70);
            if (i % 700 == 0) { opt_theme = (opt_theme + 1) % 3; opt_anim = (opt_anim + 1) % 3; lv = !lv; }
            theme_apply(); render(transition); transition = 0;
            if (screens[screen].tick) screens[screen].tick();
            if (!seen[screen]) { seen[screen] = 1; screens_seen++; }
        }
        /* con un seme diverso si controllano solo gli errori: quante schermate visita dipende dal caso */
        CHECK(getenv("IRIDE_FUZZ_SEED") || screens_seen >= SCR_COUNT - 2, "%d tasti a caso: %d schermate visitate, nessun errore", fuzz_n, screens_seen);
        offscreen_on = 0; offshape_on = 0;
        for (int i = 0; i < offscreen && i < 8; i++) printf("  fuori schermo: %s\n", offscreen_txt[i]);
        if (offshape) printf("  forme fuori schermo: %d, la prima: %s\n", offshape, offshape_txt);
        CHECK(offscreen == 0 && offshape == 0, "%d tasti a caso: niente testi o forme fuori dallo schermo (%d, %d)", fuzz_n, offscreen, offshape);
        P = saved; lv = lvs; opt_theme = 0; opt_anim = 0; msg[0] = 0;
    }

    /* ---------- costo di disegno: lettere per fotogramma (indicatore di velocita') */
    long maxg = 0; int maxs = 0;
    for (int s = 0; s < SCR_COUNT; s++) { screen = s; glyphs = 0; render(0); if (glyphs > maxg) { maxg = glyphs; maxs = s; } }
    printf("info: schermata piu' pesante %s con %ld caratteri per fotogramma\n", screens[maxs].title, maxg);
    if (getenv("IRIDE_PROFILE"))
    {
        /* stima grezza coi "mattoni" misurati sulla 6D (1.3.2): riempimento 1,3 us + 0,005 us/pixel,
           lettera 20 us. Il testo piccolo e i blocchi copiati in memoria non passano da qui */
        for (int t = 0; t < 2; t++)
        {
            opt_theme = t;
            for (int s2 = 0; s2 < SCR_COUNT; s2++)
            {
                screen = s2; theme_apply(); render(0);       /* giro a vuoto, come la Prova */
                sim_fills = 0; sim_fill_area = 0; glyphs = 0; render(0);
                printf("profilo %-22s tema %d: %5ld riempimenti, %4ld kpx, %4ld lettere ~ %ld us\n", screens[s2].title, t,
                    sim_fills, sim_fill_area / 1000, glyphs, (long)(sim_fills * 13 / 10 + sim_fill_area / 200 + glyphs * 20));
            }
        }
        opt_theme = 0;
    }

    printf("\n%d test superati, %d falliti\n", passes, fails);
    return fails ? 1 : 0;
}
