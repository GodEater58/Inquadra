/*
 * Iride 6D - firmware personalizzato per Canon EOS 6D (fw 1.1.6), modulo Magic Lantern.
 *
 * Interfaccia propria a tutto schermo, animata e senza sfarfallio (doppio buffer):
 *   ASTRO    Via Lattea (NPF), intervallometro (Via Lattea, Deep sky, Star trails, Timelapse,
 *            Giorno-notte, Dark, Flat, Bias), fuoco stelle (HFR), allineamento polare,
 *            calendario del cielo, etichetta dei file
 *   FOTO     bracketing HDR, focus stacking, Dual ISO, ETTR, scatto su movimento,
 *            trappola di fuoco, strumenti del Live View, filtri ND con posa B
 *   RULLINI  pellicole con push/pull, pausa e ripresa, contatore nel Live View, registro
 *   PROFILI  5 profili personali
 *   TORNA ALLA 6D  rimette tutte le impostazioni com'erano prima di Iride
 *
 * Il codice e' diviso in parti incluse qui sotto (un'unica unita' di compilazione).
 * Licenza: GPL (come Magic Lantern)
 */

#include <module.h>
#include <dryos.h>
#include <bmp.h>
#include <menu.h>
#include <config.h>
#include <property.h>
#include <propvalues.h>
#include <lens.h>
#include <shoot.h>
#include <beep.h>
#include <fileprefix.h>
#include <fio-ml.h>
#include <powersave.h>
#include <vram.h>
#include <notify_box.h>
#include <focus.h>
#include <picstyle.h>
#include <lvinfo.h>
#include <math.h>
#include <battery.h>

#define IRIDE_VERSION "1.4.1"

/* ------------------------------------------------------------------ compatibilita' API */

/* stile immagine: nomi diversi tra build ufficiale 6D (2025) e ramo di sviluppo 2026 */
#ifdef IRIDE_PICSTYLE_2026
extern void picstyle_set_current_contrast(int value);
extern void picstyle_set_current_saturation(int value);
extern void picstyle_set_current_color_tone(int value);
#define ps_get_contrast   picstyle_get_current_contrast
#define ps_get_saturation picstyle_get_current_saturation
#define ps_get_tone       picstyle_get_current_color_tone
#define ps_set_contrast   picstyle_set_current_contrast
#define ps_set_saturation picstyle_set_current_saturation
#define ps_set_tone       picstyle_set_current_color_tone
#else
#define ps_get_contrast   lens_get_contrast
#define ps_get_saturation lens_get_saturation
#define ps_get_tone       lens_get_color_tone
#define ps_set_contrast   lens_set_contrast
#define ps_set_saturation lens_set_saturation
#define ps_set_tone       lens_set_color_tone
#endif

extern int val2raw_iso(int val);
extern int val2raw_aperture(int val);
extern int menu_redraw_blocked;
extern int hdr_stepsize;                 /* esportata dal core (Advanced Bracket) */
extern void focus_stack_run(int skip_frame);
extern int is_play_mode();
extern int is_camera(const char *model, const char *firmware);
extern uint8_t *bmp_vram_real();
extern uint8_t *bmp_vram_idle();
extern void bmp_draw_to_idle(int value);
extern void bmp_idle_copy(int direction, int fullsize);
extern void gui_open_menu();
extern void menu_redraw();
extern void *memmove(void *dst, const void *src, size_t n);

/* schermo intero dal core di Magic Lantern (patch di Iride 6D, src/menu.c). Con un ML senza
   la patch queste funzioni valgono 0 e Iride lavora come prima, coi suoi giri */
#ifndef WEAK_FUNC
#define WEAK_FUNC(name) __attribute__((weak,alias(#name)))
#endif
extern WEAK_FUNC(ret_0) int menu_fullscreen_begin();
extern WEAK_FUNC(ret_0) void menu_fullscreen_end();
extern WEAK_FUNC(ret_0) int menu_fullscreen_ready();
extern WEAK_FUNC(ret_0) int menu_fullscreen_seq();

#ifndef INT_MIN
#define INT_MIN (-2147483647 - 1)
#endif
#ifndef WB_KELVIN
#define WB_KELVIN 9
#endif
#ifndef PROP_WB_MODE_LV
#define PROP_WB_MODE_LV 0x80050018
#endif
#ifndef PROP_WB_MODE_PH
#define PROP_WB_MODE_PH 0x8000000D
#endif
#ifndef GUISTATE_IDLE
#define GUISTATE_IDLE 0          /* DryOS (gui-common.h); il 4 vale solo per la 5D classic */
#endif
extern int gui_state;
#ifndef BMPPITCH
#define BMPPITCH 960
#endif

#define SCR_W 720
#define SCR_H 480
#define BGMT_LIGHT_6D 0x20   /* tasto luce del display superiore (platform/6D.116/gui.h) */
#define GMT_OLC_INFO_6D 0x69 /* aggiornamento interno del display: non e' un tasto */
#ifndef FAKE_BTN
#define FAKE_BTN -123456     /* tasto "premuto" da Magic Lantern (gui-common.h) */
#endif

/* ------------------------------------------------------------------ opzioni (menu ML) */

static CONFIG_INT("iride.review",   opt_review_off, 1);   /* spegni anteprima col rullino */
static CONFIG_INT("iride.wind",     opt_wind, 1);         /* avviso posa dopo ogni scatto */
static CONFIG_INT("iride.lightkey", opt_light_key, 1);    /* tasto luce LCD apre Iride */
static CONFIG_INT("iride.trash",    opt_trash_open, 1);   /* cestino apre Iride (dentro Iride: menu ML) */
static CONFIG_INT("iride.boot",     opt_boot_open, 1);    /* Iride appare all'accensione */
static CONFIG_INT("iride.anim",     opt_anim, 0);         /* 0 complete, 1 rapide, 2 spente */
static CONFIG_INT("iride.theme",    opt_theme, 2);        /* 0 normale, 1 notte, 2 notte in Astro */
static CONFIG_INT("iride.splash",   opt_splash, 1);
static CONFIG_INT("iride.sqm",      opt_sqm10, 210);      /* cielo buio del luogo, mag/arcsec^2 x10 */
static CONFIG_INT("iride.session",  opt_session, 1);      /* registro della sessione accanto alle foto */
static CONFIG_INT("iride.silent",   opt_silent, 0);       /* intervallometro: otturatore silenzioso (modulo silent) */
static CONFIG_INT("iride.dither.n", opt_dither_n, 0);     /* pausa per il dithering ogni n scatti (0 = no) */
static CONFIG_INT("iride.dither.s", opt_dither_s, 10);    /* durata della pausa per il dithering, s */
static CONFIG_INT("iride.npf.dec",  opt_npf_dec, 0);      /* NPF: punto del cielo (indice della declinazione) */

/* ------------------------------------------------------------------ stato salvato su scheda */

#define NPROF 5
#define NSTYLE 10
#define NMLF 24                          /* funzioni ML della struttura v2 */
#define NMLF2 8                          /* funzioni ML aggiunte dalla 1.2 */

struct profile
{
    int valid, name, mode, iso, shutter, aperture, wb, kelvin, style, c, s, t;
};

struct persist
{
    int magic, version, size;

    /* rullino */
    int roll_loaded, roll_paused, roll_film, roll_num, roll_shots, roll_exp, roll_push;
    int roll_last_fn, roll_first_fn;

    /* "Torna alla 6D": fotografia delle impostazioni prima di Iride */
    int snap_valid, snap_mode, snap_iso, snap_shutter, snap_aperture, snap_wb, snap_kelvin;
    int snap_style, snap_review;
    int ps_touched;                       /* stili immagine modificati (bit per stile) */
    int ps_c[NSTYLE], ps_s[NSTYLE], ps_t[NSTYLE];
    int ml_touched;                       /* funzioni Magic Lantern modificate (bit) */
    int ml_prev[NMLF];

    /* astro */
    int astro_tag, int_preset, int_count, int_bulb, int_pause, int_delay_idx, int_screen_off;
    int npf_focal, npf_ap10;
    int lat100, lon100, tz_mode, polar_view;

    /* foto */
    int hdr_frames, hdr_step, hdr_seq;
    int fs_front, fs_behind, fs_step;
    int dual_rec, ettr_mode, md_trigger, md_level, trap_mode;
    int nd_idx, nd_base;

    struct profile prof[NPROF];

    /* dalla versione 3 (Iride 1.2): tutto quello che segue e' nuovo; un file v2 si carica
       copiando la parte sopra e lasciando questa ai valori di partenza */
    int ml_prev2[NMLF2];
    int reserved[24];                     /* spazio per le versioni future senza perdere i dati */
    int checksum;
};

static struct persist P;
static volatile int persist_dirty = 0;

#define PERSIST_MAGIC 0x45444952   /* "RIDE" */
#define PERSIST_VERSION 3
#define PERSIST_V2_WORDS ((int)(__builtin_offsetof(struct persist, ml_prev2) / sizeof(int)) + 1)   /* v2: fino a prof, poi il checksum */

/* ------------------------------------------------------------------ stato dell'interfaccia */

enum
{
    SCR_HOME, SCR_ASTRO, SCR_FOTO, SCR_NPF, SCR_INTERVAL, SCR_STARS, SCR_POLAR, SCR_SKY,
    SCR_SHELF, SCR_CONFIRM, SCR_HDR, SCR_FSTACK, SCR_DUAL, SCR_ETTR, SCR_MOTION, SCR_LVTOOLS,
    SCR_ND, SCR_PROFILES, SCR_OPTIONS, SCR_RESTORE, SCR_KEYS, SCR_BENCH, SCR_MOON, SCR_MWPLAN, SCR_COUNT
};

static volatile int app_running = 0;
static volatile int screen = SCR_HOME;
static volatile int dirty = 1;
static int sel[SCR_COUNT];
static int transition = 0;               /* +1 avanti (da destra), -1 indietro */

static char msg[96] = "";
static int msg_until = 0;
static int msg_t0 = 0;                   /* quando e' comparso l'avviso (animazione) */
static int msg_color = 0;

/* coda dei tasti: il gestore li accoda, il task di Iride li esegue (puo' bloccare senza danni) */
#define KQ 16
static volatile int kq[KQ];
static volatile int kq_head = 0, kq_tail = 0;

static void kq_push(int k)
{
    int n = (kq_head + 1) % KQ;
    if (n != kq_tail) { kq[kq_head] = k; kq_head = n; }
}

static int kq_pop()
{
    if (kq_tail == kq_head) return -1;
    int k = kq[kq_tail];
    kq_tail = (kq_tail + 1) % KQ;
    return k;
}

static int key_pending() { return kq_tail != kq_head; }

/* un tasto premuto durante un'animazione la salta e viene consumato (non fa altro) */
static int anim_skip()
{
    if (!key_pending()) return 0;
    kq_pop();
    return 1;
}

/* cestino dentro Iride: si passa al menu di Magic Lantern invece di chiudere tutto */
static volatile int to_ml_menu = 0;
static volatile int open_settle = 0;   /* aperta dalla schermata di scatto: Canon apre il suo menu sotto */
static volatile int lv_toggle = 0;     /* START/STOP dentro Iride: Live View acceso/spento e ritorno qui */
static int lv_resume_screen = 0;
static int lv_reopen_ms = -100000;      /* riaperta dopo il Live View: un LV doppio subito dopo si ignora */
#define LV_DEBOUNCE_MS 1500
static volatile int skip_splash = 0;    /* riapertura dopo il Live View: subito al lavoro */
static volatile int fullscreen = 0;     /* ML di Iride: schermo intero dal core (niente giri) */
static volatile int key_fake = 0;       /* l'evento in corso e' un tasto finto di ML */

/* diagnostica: ultimi eventi dei tasti visti da Iride, per capire cosa manda la 6D */
#define KLOG_N 12
struct klog_entry { int param, arg, gs, ms; char app, menu, play, opened, fake; };
static struct klog_entry klog[KLOG_N];
static volatile int klog_count = 0;

static void klog_add(int param, int arg, int opened)
{
    struct klog_entry *k = &klog[klog_count % KLOG_N];
    k->param = param; k->arg = arg; k->gs = gui_state; k->ms = get_ms_clock();
    k->app = app_running; k->menu = gui_menu_shown(); k->play = is_play_mode(); k->opened = opened;
    k->fake = arg == FAKE_BTN;
    klog_count++;
}

/* lavori lanciati a Iride chiusa (sequenze, focus stacking, posa B) */
enum { JOB_NONE, JOB_SEQ, JOB_FSTACK, JOB_BULB };
static volatile int job_request = JOB_NONE;
static volatile int job_running = JOB_NONE;
static volatile int job_abort = 0;

static void say(int color, const char *text);
static const char *parent_title(const char *section);
static void go(int s, int dir);
static void lv_task(void *unused);
static void session_watch_external(int fn);
static void log_warning(const char *text);
static int persist_touch_ms = 0;
static void persist_touch() { persist_dirty = 1; persist_touch_ms = get_ms_clock(); }

#include "ui.inc"
#include "moon.inc"
#include "icons.inc"
#include "store.inc"
#include "film.inc"
#include "campo.inc"
#include "astro.inc"
#include "sky2.inc"
#include "photo.inc"
#include "bench.inc"
#include "screens.inc"

/* ------------------------------------------------------------------ ciclo dell'app */

static void say(int color, const char *text)
{
    snprintf(msg, sizeof(msg), "%s", text);
    if (color == C_WARN) log_warning(text);     /* gli avvisi restano nel registro su scheda */
    msg_color = color;
    msg_t0 = get_ms_clock();
    msg_until = msg_t0 + 3500;
    dirty = 1;
}

static void go(int s, int dir)
{
    if (s == screen) return;
    if (screen == SCR_STARS) stars_leave();
    /* indietro: il dialogo riscende; verso la Home la schermata rientra nel suo riquadro */
    if (dir < 0 && screen == SCR_CONFIRM) dir = -2;
    if (dir < 0 && s == SCR_HOME)
    {
        int i = home_tile_of(screen);
        if (i >= 0) { sel[SCR_HOME] = i; zoom_from_tile(i); dir = -3; }
    }
    screen = s;
    transition = dir;
    msg[0] = 0;
    msg_until = 0;
    dirty = 1;
    hl_reset();
    if (screens[s].enter) screens[s].enter();
}

static void app_task(void *unused)
{
    app_running = 1;

    /* aperto dal tasto luce: aspetta che il menu sia davvero aperto */
    for (int i = 0; i < 30 && !gui_menu_shown(); i++) msleep(50);
    if (!gui_menu_shown()) { menu_redraw_blocked = 0; app_running = 0; return; }

    menu_redraw_blocked = 1;
    fullscreen = menu_fullscreen_begin();
    to_ml_menu = 0;
    /* ML apre il menu di Canon sotto il suo (circa 200 ms) e poi ferma i disegni di Canon:
       aspettiamo che abbia finito, altrimenti Canon disegna sopra Iride. Col core di Iride
       ML dice quando ha finito; altrimenti si aspetta un tempo fisso */
    if (open_settle)
    {
        if (fullscreen) { for (int i = 0; i < 30 && !menu_fullscreen_ready(); i++) msleep(20); }
        else msleep(350);
        open_settle = 0;
    }
    /* in Live View ML spegne la grafica finche' non ridisegna il suo menu (che Iride blocca) */
    bmp_on();
    int open_ms = get_ms_clock(), last_full = 0, last_poke = 0;
    int last_seq = menu_fullscreen_seq(), resync_until = 0;
    kq_tail = kq_head;
    theme_apply();
    clrscr();
    if (opt_splash && opt_anim != 2 && !skip_splash) splash();
    if (screen == SCR_HOME && opt_anim != 2 && !skip_splash)
    {
        /* i riquadri della Home entrano uno dopo l'altro, poi l'icona scelta si anima */
        home_intro_t0 = get_ms_clock() | 1;
        icon_i = sel[SCR_HOME];
        icon_t0 = home_intro_t0 + anim_ms(520);
    }
    skip_splash = 0;
    dirty = 1;
    transition = 0;
    int last_tick = 0;

    TASK_LOOP
    {
        if (!gui_menu_shown() || to_ml_menu || lv_toggle) break;

        int key = kq_pop();                  /* non "k": e' gia' il contatore di TASK_LOOP */
        if (key >= 0)
        {
            handle_key(key);
            continue;
        }

        if (prefix_pending)
        {
            prefix_pending = 0;
            prefix_refresh();
            dirty = 1;
        }

        int now = get_ms_clock();
        if (msg[0] && now >= msg_until) { msg[0] = 0; dirty = 1; }
        if (msg[0] && msg_until - now <= anim_ms(240)) dirty = 1;     /* l'avviso si spegne piano */
        if (screens[screen].tick && now - last_tick >= screens[screen].tick_ms)
        {
            last_tick = now;
            if (screens[screen].tick()) dirty = 1;
        }

        int period;
        if (fullscreen)
        {
            /* col core di Iride ML tiene sveglio da solo il dialogo Canon e Canon non disegna
               sullo schermo: basta ridisegnare quando ML segnala che Canon potrebbe averlo
               fatto (risveglio in Live View), piu' un controllo ogni 2 s */
            int seq = menu_fullscreen_seq();
            if (seq != last_seq) { last_seq = seq; resync_until = now + 300; dirty = 1; }
            period = now < resync_until ? 100 : 2000;
        }
        else
        {
            /* in Live View la 6D chiude il menu Canon sotto se nessuno lo risveglia: ci pensa ML
               a ogni ridisegno (che resta bloccato, ma il controllo del dialogo gira lo stesso) */
            if (lv && now - last_poke >= 500) { last_poke = now; menu_redraw(); }
            /* ridisegno completo periodico (invisibile, doppio buffer): cancella qualsiasi disegno
               arrivato da fuori (Canon, ML) - spesso nei primi secondi e in Live View, dove ogni
               2 s ML risveglia il menu Canon sotto, altrimenti ogni secondo */
            period = now - open_ms < 3000 ? 150 : lv ? 250 : 1000;
        }
        if (now - last_full >= period) dirty = 1;

        if (dirty || anim_active())
        {
            dirty = 0;
            anim_again = 0;
            last_full = now;
            theme_apply();
            render(transition);
            transition = 0;
            msleep(1);
        }
        else msleep(25);
    }

    if (screen == SCR_STARS) stars_leave();
    bmp_draw_to_idle(0);
    clrscr();
    if (fullscreen) { menu_fullscreen_end(); fullscreen = 0; }
    menu_redraw_blocked = 0;
    app_running = 0;
    if (to_ml_menu)
    {
        /* il menu di Magic Lantern e' ancora aperto: lo ridisegniamo */
        to_ml_menu = 0;
        menu_redraw();
    }
    if (lv_toggle)
    {
        lv_toggle = 0;
        gui_stop_menu();
        task_create("iride_lv", 0x1e, 0x1000, lv_task, (void *)0);
    }
    persist_save_if_dirty();

    if (job_request != JOB_NONE && job_running == JOB_NONE)
    {
        job_running = job_request;
        job_request = JOB_NONE;
        task_create("iride_job", 0x1c, 0x2000, job_task, (void *)0);
    }
}

static void iride_start()
{
    if (app_running) return;
    if (job_running)
    {
        NotifyBox(3000, "Iride: lavoro in corso, cestino per fermarlo");
        return;
    }
    task_create("iride_app", 0x1c, 0x3000, app_task, (void *)0);
}

/* ------------------------------------------------------------------ tasti */

/* si apre solo dove Magic Lantern apre il suo menu col cestino (menu.c, handle_ml_menu_erase):
   schermata di scatto a riposo, niente riproduzione, niente filmato, niente anteprima profondita' */
static int can_open_from_shooting()
{
    return !app_running && !job_running && !gui_menu_shown() && gui_state == GUISTATE_IDLE &&
        !is_play_mode() && !RECORDING && !dofpreview && is_camera("6D", "1.1.6");
}

static void open_on(int scr)
{
    menu_redraw_blocked = 1;          /* niente lampo del menu ML prima di Iride */
    menu_fullscreen_begin();          /* col core di Iride: Canon fermato appena finito il flood */
    /* menu ML sulla scheda Shoot, voce Iride: niente indicatori audio (scheda Audio) sotto Iride,
       e il cestino da Iride riporta proprio qui */
    select_menu_by_name("Shoot", "Iride");
    screen = COERCE(scr, 0, SCR_COUNT - 1);
    hl_reset();
    if (screens[screen].enter) screens[screen].enter();
    open_settle = 1;
    gui_open_menu();
    iride_start();
}

static void open_from_shooting() { open_on(SCR_HOME); }

/* all'accensione: aspetta che Canon e ML abbiano finito (pulizia del sensore, schermata di
   scatto accesa) e apre la Home. Se nel frattempo si usa un tasto o si tiene premuto a meta'
   il pulsante di scatto (risveglio per scattare subito) non apre nulla. */
static volatile int boot_done = 0;
static int boot_ready()
{
    return can_open_from_shooting() && display_is_on() && !sensor_cleaning;
}

/* START/STOP dentro Iride: Iride si chiude, la 6D accende (o spegne) il Live View come col suo
   tasto, poi Iride si riapre sulla stessa schermata */
static void lv_task(void *unused)
{
    int was = lv;
    msleep(300);                                /* ML chiude il suo menu e quello di Canon */
    if (!was) force_liveview(); else close_liveview();
    /* ML accende e spegne il Live View "premendo" un tasto LV finto. Se Canon e' occupata
       (per esempio subito dopo un cambio di rullino, mentre salva stile, ISO e bianco) il tasto
       finto arriva in ritardo: se Iride fosse gia' riaperta lo prenderebbe per un nuovo
       START/STOP e lo toglierebbe a Canon, e si andrebbe avanti e indietro senza fine.
       Quindi Iride si riapre solo quando il Live View e' cambiato davvero. */
    for (int i = 0; i < 80 && lv == was; i++) msleep(100);
    if (lv == was) return;                      /* il tasto finto arrivera' a Canon, non a Iride */
    msleep(400);
    for (int i = 0; i < 30 && !can_open_from_shooting(); i++) msleep(100);
    if (can_open_from_shooting())
    {
        lv_reopen_ms = get_ms_clock();
        skip_splash = 1;
        open_on(lv_resume_screen);
    }
}

static void boot_task(void *unused)
{
    int keys0 = klog_count;
    msleep(2500);
    for (int i = 0; i < 40 && !boot_done; i++)
    {
        if (!opt_boot_open || klog_count != keys0 || get_halfshutter_pressed() || app_running) break;
        if (boot_ready())
        {
            open_from_shooting();
            break;
        }
        msleep(250);
    }
    boot_done = 1;
}

static unsigned int iride_keypress(unsigned int key)
{
    /* durante un lavoro (sequenza, stack, posa B) il cestino lo ferma */
    if (job_running && !app_running)
    {
        if (key == MODULE_KEY_TRASH) { job_abort = 1; return 0; }
        return 1;
    }
    if (!app_running)
    {
        if (key == MODULE_KEY_TRASH && opt_trash_open && can_open_from_shooting())
        {
            if (klog_count) klog[(klog_count - 1) % KLOG_N].opened = 1;
            open_from_shooting();
            return 0;
        }
        return 1;
    }

    /* 0 = evento che non e' un tasto (aggiornamenti interni di Canon quando cambiano ISO, stile,
       bianco...): passa a Canon e non entra nella coda, altrimenti interrompe le animazioni */
    if (key <= 0) return 1;

    switch (key)
    {
        /* cestino: da Iride al menu di Magic Lantern (un altro cestino lo chiude) */
        case MODULE_KEY_TRASH:
            to_ml_menu = 1;
            return 0;
        /* START/STOP (sulla 6D Live View e REC sono lo stesso tasto): in foto accende/spegne
           il Live View e torna qui; in filmato lo lasciamo a Canon (registrazione) */
        case MODULE_KEY_LV:
        case MODULE_KEY_REC:
            if (is_movie_mode() || RECORDING) return 1;
            /* tasto LV "premuto" da ML (force_liveview, close_liveview) arrivato in ritardo:
               e' per Canon, non un nuovo START/STOP */
            if (key_fake) return 1;
            /* appena riaperta dopo il Live View: un LV ritardato (o doppio) non rilancia tutto */
            if (get_ms_clock() - lv_reopen_ms < LV_DEBOUNCE_MS) return 0;
            if (lv_toggle) return 0;
            lv_resume_screen = screen;
            lv_toggle = 1;
            return 0;
        /* lasciamo passare: chiudono il menu e quindi Iride */
        case MODULE_KEY_PRESS_HALFSHUTTER:
        case MODULE_KEY_UNPRESS_HALFSHUTTER:
        case MODULE_KEY_PRESS_FULLSHUTTER:
        case MODULE_KEY_UNPRESS_FULLSHUTTER:
            return 1;
        /* rilasci e diagonali: non servono, ma non devono arrivare a Canon */
        case MODULE_KEY_UNPRESS_SET:
        case MODULE_KEY_UNPRESS_UDLR:
        case MODULE_KEY_PRESS_UP_LEFT:
        case MODULE_KEY_PRESS_UP_RIGHT:
        case MODULE_KEY_PRESS_DOWN_LEFT:
        case MODULE_KEY_PRESS_DOWN_RIGHT:
        case MODULE_KEY_JOY_CENTER:
            return 0;
    }
    kq_push(key);
    return 0;
}

/* tasto luce del display superiore: apre Iride dalla schermata di scatto */
static unsigned int iride_keypress_raw(unsigned int ctx)
{
    struct event *e = (struct event *)(uintptr_t)ctx;
    key_fake = e && e->arg == FAKE_BTN;   /* letto subito dopo da iride_keypress, stesso evento */
    if (!e || e->param < 0 || e->param == GMT_OLC_INFO_6D) return 1;
    int open = opt_light_key && e->param == BGMT_LIGHT_6D && can_open_from_shooting();
    klog_add(e->param, e->arg, open);
    if (!open) return 1;
    open_from_shooting();
    return 0;
}

/* ------------------------------------------------------------------ menu di Magic Lantern */

static MENU_SELECT_FUNC(iride_open)
{
    screen = SCR_HOME;
    iride_start();
}

static MENU_UPDATE_FUNC(iride_open_upd)
{
    if (P.roll_loaded)
        MENU_SET_RINFO("R%03d %d/%d%s", P.roll_num % 1000, P.roll_shots, P.roll_exp, P.roll_paused ? " pausa" : "");
}

static struct menu_entry iride_menu[] =
{
    {
        .name    = "Iride",
        .select  = iride_open,
        .update  = iride_open_upd,
        .help    = "Apri Iride: Astro, Foto, Rullini, Profili. Anche col cestino o il tasto luce.",
        .help2   = "Q: opzioni rapide.",
        .children = (struct menu_entry[])
        {
            {
                .name    = "All'accensione",
                .priv    = &opt_boot_open,
                .max     = 1,
                .choices = CHOICES("niente", "apre Iride"),
                .help    = "Accendendo la 6D compare la Home di Iride (mezzo scatto per scattare).",
            },
            {
                .name    = "Cestino",
                .priv    = &opt_trash_open,
                .max     = 1,
                .choices = CHOICES("menu ML", "apre Iride"),
                .help    = "Il cestino apre Iride; dentro Iride il cestino porta al menu ML.",
            },
            {
                .name    = "Tasto luce LCD",
                .priv    = &opt_light_key,
                .max     = 1,
                .choices = CHOICES("normale", "apre Iride"),
                .help    = "Il tasto della luce del display superiore apre Iride.",
            },
            {
                .name    = "Animazioni",
                .priv    = &opt_anim,
                .max     = 2,
                .choices = CHOICES("complete", "rapide", "spente"),
            },
            {
                .name    = "Tema",
                .priv    = &opt_theme,
                .max     = 2,
                .choices = CHOICES("normale", "notte (rosso)", "notte in Astro"),
            },
            MENU_EOL,
        },
    },
};

/* quando Magic Lantern salva le sue impostazioni (anche allo spegnimento) salviamo anche le nostre */
static unsigned int iride_config_save(unsigned int ctx)
{
    persist_save_if_dirty();
    return CBR_RET_CONTINUE;
}

static unsigned int iride_init()
{
    persist_sem = create_named_semaphore("iride_dat", SEM_CREATE_UNLOCKED);
    persist_load();
    film_init();
    menu_add("Shoot", iride_menu, COUNT(iride_menu));
    lvinfo_add_items(roll_info_items, COUNT(roll_info_items));
    prefix_pending = 1;
    if (opt_boot_open) task_create("iride_boot", 0x1e, 0x1000, boot_task, (void *)0);
    return 0;
}

static unsigned int iride_deinit()
{
    persist_save_if_dirty();
    prefix_apply("");
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(iride_init)
    MODULE_DEINIT(iride_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    /* prima il gestore grezzo: vede ogni evento (diagnostica) e passa quasi sempre oltre */
    MODULE_CBR(CBR_KEYPRESS_RAW, iride_keypress_raw, 0)
    MODULE_CBR(CBR_KEYPRESS, iride_keypress, 0)
    MODULE_CBR(CBR_SECONDS_CLOCK, iride_clock, 0)
    MODULE_CBR(CBR_CONFIG_SAVE, iride_config_save, 0)
MODULE_CBRS_END()

MODULE_CONFIGS_START()
    MODULE_CONFIG(opt_review_off)
    MODULE_CONFIG(opt_wind)
    MODULE_CONFIG(opt_light_key)
    MODULE_CONFIG(opt_trash_open)
    MODULE_CONFIG(opt_boot_open)
    MODULE_CONFIG(opt_anim)
    MODULE_CONFIG(opt_theme)
    MODULE_CONFIG(opt_splash)
    MODULE_CONFIG(opt_sqm10)
    MODULE_CONFIG(opt_session)
    MODULE_CONFIG(opt_silent)
    MODULE_CONFIG(opt_dither_n)
    MODULE_CONFIG(opt_dither_s)
    MODULE_CONFIG(opt_npf_dec)
    MODULE_CONFIG(opt_moon_iso)
    MODULE_CONFIG(opt_moon_ap)
    MODULE_CONFIG(opt_moon_es)
MODULE_CONFIGS_END()
