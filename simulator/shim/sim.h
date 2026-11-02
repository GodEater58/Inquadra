/* Simulatore delle API di Magic Lantern usate da Iride 0.2, per eseguirlo su PC */
#ifndef SIM_H
#define SIM_H
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#define PUT_ROW_HOOK sim_put_row   /* righe scritte direttamente nel buffer: il simulatore le registra */
#define COUNT(x) (sizeof(x) / sizeof((x)[0]))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define ABS(a) ((a) > 0 ? (a) : -(a))
#define COERCE(x,lo,hi) MAX(MIN((x),(hi)),(lo))
#define MOD(x,m) ((((int)(x)) % ((int)(m)) + ((int)(m))) % ((int)(m)))
static inline int streq(const char *a, const char *b) { return strcmp(a, b) == 0; }

/* ---- bitmap */
#define COLOR_EMPTY 0x00
#define COLOR_WHITE 0x01
#define COLOR_BLACK 0x02
#define COLOR_CYAN 0x05
#define COLOR_GREEN1 0x06
#define COLOR_GREEN2 0x07
#define COLOR_RED 0x08
#define COLOR_LIGHT_BLUE 0x09
#define COLOR_BLUE 0x0B
#define COLOR_DARK_RED 0x0C
#define COLOR_MAGENTA 0x0E
#define COLOR_YELLOW 0x0F
#define COLOR_ORANGE 0x13
#define COLOR_ALMOST_BLACK 0x26
#define COLOR_ALMOST_WHITE 0x4F
#define COLOR_GRAY(p) (38 + (p) * 41 / 100)
#define NO_BG_ERASE 0xFF
#define FONT(font,fg,bg) (0 | ((font) & 0xFFFF0000) | ((bg) & 0xFF) << 8 | ((fg) & 0xFF))
#define FONT_DYN(id,fg,bg) FONT((id) << 16, fg, bg)
#define FONT_SMALL FONT_DYN(0, COLOR_WHITE, COLOR_BLACK)
#define FONT_MED   FONT_DYN(2, COLOR_WHITE, COLOR_BLACK)
#define FONT_MED_LARGE FONT_DYN(3, COLOR_WHITE, COLOR_BLACK)
#define FONT_LARGE FONT_DYN(4, COLOR_WHITE, COLOR_BLACK)
#define BMPPITCH 960
int bmp_printf(uint32_t fontspec, int x, int y, const char *fmt, ...);
int bmp_string_width(int fontspec, const char *s);
void bmp_fill(uint8_t color, int x, int y, int w, int h);
void bmp_draw_rect(int color, int x, int y, int w, int h);
void fill_circle(int x, int y, int r, int c);
void draw_circle(int x, int y, int r, int c);
void draw_line(int x1, int y1, int x2, int y2, int c);
void clrscr(void);

/* ---- config / modulo / menu */
#define CONFIG_INT(name, var, val) int var = val
#define MODULE_INFO_START()
#define MODULE_INIT(f)
#define MODULE_DEINIT(f)
#define MODULE_INFO_END()
#define MODULE_CBRS_START()
#define MODULE_CBR(a,b,c)
#define MODULE_CBRS_END()
#define MODULE_CONFIGS_START()
#define MODULE_CONFIG(v)
#define MODULE_CONFIGS_END()
#define CBR_RET_CONTINUE 0
struct menu_display_info { char rinfo[32]; };
struct menu_entry {
    const char *name; void (*select)(void*, int); void (*update)(struct menu_entry*, struct menu_display_info*);
    const char *help; const char *help2; int *priv; int max; const char **choices; struct menu_entry *children;
};
#define MENU_EOL { 0 }
#define CHOICES(...) ((const char *[]) { __VA_ARGS__ })
#define MENU_SELECT_FUNC(f) void f(void *priv, int delta)
#define MENU_UPDATE_FUNC(f) void f(struct menu_entry *entry, struct menu_display_info *info)
#define MENU_SET_RINFO(...) snprintf(info->rinfo, sizeof(info->rinfo), __VA_ARGS__)
void menu_add(const char *name, struct menu_entry *m, int n);
int gui_menu_shown(void);
void gui_stop_menu(void);
int menu_get_value_from_script(const char *name, const char *entry);
int menu_set_value_from_script(const char *name, const char *entry, int value);
struct event { int type; int param; void *obj; int arg; };

/* ---- tasti */
#define MODULE_KEY_PRESS_HALFSHUTTER 1
#define MODULE_KEY_UNPRESS_HALFSHUTTER 2
#define MODULE_KEY_PRESS_FULLSHUTTER 3
#define MODULE_KEY_UNPRESS_FULLSHUTTER 4
#define MODULE_KEY_WHEEL_UP 5
#define MODULE_KEY_WHEEL_DOWN 6
#define MODULE_KEY_WHEEL_LEFT 7
#define MODULE_KEY_WHEEL_RIGHT 8
#define MODULE_KEY_PRESS_SET 9
#define MODULE_KEY_UNPRESS_SET 10
#define MODULE_KEY_JOY_CENTER 11
#define MODULE_KEY_PRESS_UP 12
#define MODULE_KEY_PRESS_UP_RIGHT 13
#define MODULE_KEY_PRESS_UP_LEFT 14
#define MODULE_KEY_PRESS_RIGHT 15
#define MODULE_KEY_PRESS_LEFT 16
#define MODULE_KEY_PRESS_DOWN_RIGHT 17
#define MODULE_KEY_PRESS_DOWN_LEFT 18
#define MODULE_KEY_PRESS_DOWN 19
#define MODULE_KEY_UNPRESS_UDLR 20
#define MODULE_KEY_MENU 22
#define MODULE_KEY_INFO 23
#define MODULE_KEY_PLAY 24
#define MODULE_KEY_TRASH 25
#define MODULE_KEY_Q 29
#define MODULE_KEY_REC 27
#define MODULE_KEY_LV 28

/* ---- tempo e task */
void msleep(int ms);
int get_ms_clock(void);
extern int ml_shutdown_requested;
#define TASK_LOOP for (int k = 0; !ml_shutdown_requested && k < 1; k++)
void task_create(const char *name, int prio, int stack, void (*f)(void*), void *arg);
struct tm { int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst; long tm_gmtoff; };
void LoadCalendarFromRTC(struct tm *t);

/* ---- obiettivo, esposizione, proprieta' */
struct lens_info {
    char name[32]; unsigned job_state; unsigned focal_len; unsigned aperture; unsigned picstyle; unsigned wb_mode; unsigned kelvin; unsigned iso;
    uint8_t raw_aperture; uint8_t raw_shutter; uint8_t raw_iso; uint8_t raw_picstyle; uint8_t raw_aperture_min; uint8_t lens_exists;
};
extern struct lens_info lens_info;
extern int shooting_mode, lv, lv_dispsize, image_review_time, RECORDING;
extern int pic_quality;
extern int dofpreview;
int display_is_on(void);
extern int sensor_cleaning;
int get_halfshutter_pressed(void);
void menu_redraw(void);
void force_liveview(void);
void close_liveview(void);
void bmp_on(void);
int is_movie_mode(void);
void select_menu_by_name(char *name, const char *entry);
#define PICQ_RAW                 0x4060000
#define PICQ_RAW_JPG_LARGE_FINE  0x3070000
#define PICQ_LARGE_FINE          0x3010000
#define SHOOTMODE_P 0
#define SHOOTMODE_TV 1
#define SHOOTMODE_AV 2
#define SHOOTMODE_M 3
#define SHOOTMODE_BULB 4
#define WB_KELVIN 9
#define AF_DISABLE 0
#define PROP_PICTURE_STYLE 0x80000005
#define PROP_IMAGE_REVIEW_TIME 0x02020006
#define PROP_WB_MODE_LV 0x80050018
#define PROP_WB_MODE_PH 0x8000000D
void prop_request_change(unsigned p, const void *a, size_t l);
#define RAW2VALUE(param,raw) ((int)(raw))
int lens_set_rawaperture(int a); int lens_set_rawiso(int i); int lens_set_rawshutter(int s);
void lens_set_kelvin(int k);
int shutterf_to_raw(float s); float raw2shutterf(int raw);
const char *lens_format_shutter(int r); const char *lens_format_aperture(int r); const char *lens_format_iso(int r);
const char *get_shootmode_name_short(int m);
int lens_take_picture(int wait, int af); int bulb_take_pic(int ms);
int lens_focus(int n, int step, int wait, int extra); int is_manual_focus(void); void set_lv_zoom(int z);
int lens_get_contrast(void); int lens_get_saturation(void); int lens_get_color_tone(void);
void lens_set_contrast(int); void lens_set_saturation(int); void lens_set_color_tone(int);

/* ---- file, avvisi, prefissi */
#define O_RDONLY 0
#define O_SYNC 010000
struct card_info { int file_number; };
struct card_info *get_shooting_card(void);
int get_free_space_32k(const struct card_info *c);
const char *get_dcim_dir(void);
int GetBatteryLevel(void);
int GetBatteryTimeRemaining(void);
extern int efic_temp;
int take_a_pic(int should_af);
int get_us_clock(void);
char *get_config_dir(void);
int FIO_CreateDirectory(const char *d);
FILE *FIO_CreateFileOrAppend(const char *n);
FILE *FIO_CreateFile(const char *n);
FILE *FIO_OpenFile(const char *n, unsigned mode);
int FIO_ReadFile(FILE *f, void *p, size_t n);
int FIO_WriteFile(FILE *f, const void *p, size_t n);
void FIO_CloseFile(FILE *f);
#define fio_malloc malloc
#define fio_free free
#define tmp_malloc malloc
int my_fprintf(FILE *f, const char *fmt, ...);
void NotifyBox(int t, const char *fmt, ...);
void beep(void);
void beep_times(int n);
int file_prefix_set(char *p); int file_prefix_reset(int k);
void display_on(void); void display_off(void);

/* ---- Live View e info bar */
struct vram_info { uint8_t *vram; int width; int pitch; int height; };
struct vram_info *get_yuv422_vram(void);
#define LV2BM_X(x) (x)
#define LV2BM_Y(y) (y)
#define LV2BM_DX(x) (x)
enum lvinfo_bar { LV_TOP_BAR_ONLY, LV_BOTTOM_BAR_ONLY, LV_PREFER_TOP_BAR, LV_PREFER_BOTTOM_BAR, LV_WHEREVER_IT_FITS };
struct lvinfo_item;
typedef void (*lvinfo_update_func)(struct lvinfo_item *item, int can_draw);
struct lvinfo_item { char *name; char *value; lvinfo_update_func update; int preferred_position; int priority; enum lvinfo_bar which_bar; int width; int color_fg; };
#define LVINFO_UPDATE_FUNC(func) void func(struct lvinfo_item *item, int can_draw)
#define LVINFO_BUFFER(n) static char buffer[n]; buffer[0] = '\0'; item->value = buffer;
void lvinfo_add_items(struct lvinfo_item *items, int count);
#endif
