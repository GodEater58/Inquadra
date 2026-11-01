/* photo.inc - bracketing HDR, focus stacking, Dual ISO, ETTR, movimento e trappola di fuoco,
 * strumenti del Live View, filtri ND; lavori a Iride chiusa (sequenze, stack, posa B) */

static void ml_status(int f, char *buf, int len, int *kind)
{
    int v = ml_get(f);
    if (v == INT_MIN) { snprintf(buf, len, "modulo spento"); *kind = VK_WARN; }
    else *kind = v ? VK_ON : VK_OFF;
}

static void ml_toggle(int f, const char *name)
{
    int v = ml_get(f);
    if (v == INT_MIN)
    {
        char buf[80];
        snprintf(buf, sizeof(buf), "%s: attiva il modulo nel menu Modules di ML", name);
        say(C_WARN, buf);
        return;
    }
    if (!ml_set(f, !v)) say(C_WARN, "Magic Lantern non ha accettato il valore");
}

/* ------------------------------------------------------------------ bracketing HDR */

static const int hdr_frame_steps[] = { 3, 5, 7, 9 };
static const int hdr_ev8[] = { 8, 12, 16, 24 };
static const char *const hdr_ev_names[] = { "1 EV", "1,5 EV", "2 EV", "3 EV" };
static const char *const hdr_seq_names[] = { "0 - --", "0 - + -- ++", "0 + ++" };
static const char *const hdr_labels[] = { "Bracketing", "Scatti", "Passo", "Ordine" };

static void hdr_push()
{
    ml_set(MLF_HDR_FRAMES, P.hdr_frames);
    ml_set(MLF_HDR_STEP, hdr_ev8[COERCE(P.hdr_step, 0, 3)]);
    ml_set(MLF_HDR_SEQ, COERCE(P.hdr_seq, 0, 2));
}

static void hdr_value(int row, char *buf, int len, int *kind)
{
    if (row == 0) ml_status(MLF_HDR, buf, len, kind);
    else if (row == 1) snprintf(buf, len, "< %d >", P.hdr_frames);
    else if (row == 2) snprintf(buf, len, "< %s >", hdr_ev_names[COERCE(P.hdr_step, 0, 3)]);
    else snprintf(buf, len, "< %s >", hdr_seq_names[COERCE(P.hdr_seq, 0, 2)]);
}

static void hdr_change(int row, int dir, int big)
{
    if (row == 0) { if (ml_get(MLF_HDR) != INT_MIN && !ml_get(MLF_HDR)) hdr_push(); ml_toggle(MLF_HDR, "HDR"); return; }
    if (row == 1) P.hdr_frames = step_in(hdr_frame_steps, COUNT(hdr_frame_steps), P.hdr_frames, dir);
    else if (row == 2) P.hdr_step = COERCE(P.hdr_step + dir, 0, 3);
    else P.hdr_seq = COERCE(P.hdr_seq + dir, 0, 2);
    persist_touch();
    if (ml_get(MLF_HDR) == 1) hdr_push();
}

static void hdr_activate(int row) { hdr_change(row, 1, 0); }

static void hdr_extra()
{
    char buf[80];
    int span = (P.hdr_frames - 1) * hdr_ev8[COERCE(P.hdr_step, 0, 3)];
    snprintf(buf, sizeof(buf), "Copertura totale: %d.%d EV", span / 8, (span % 8) * 10 / 8);
    txt(FONT_MED, C_TEXT, 30, 252, buf);
    txt(FONT_MED, C_DIM, 30, 290, "Con il bracketing attivo ogni pressione a fondo");
    txt(FONT_MED, C_DIM, 30, 316, "dello scatto diventa una serie. Ghiera su M o Av.");
}

static const struct form hdr_form = { 4, hdr_labels, hdr_value, hdr_change, hdr_activate, hdr_extra,
    "^/v riga   </> cambia   SET attiva   MENU indietro", 58, 46 };

/* ------------------------------------------------------------------ focus stacking */

static const char *const fs_labels[] = { "Foto davanti", "Foto dietro", "Passi tra le foto", "Avvia lo stack" };

static void fs_value(int row, char *buf, int len, int *kind)
{
    if (row == 0) snprintf(buf, len, "< %d >", P.fs_front);
    else if (row == 1) snprintf(buf, len, "< %d >", P.fs_behind);
    else if (row == 2) snprintf(buf, len, "< %d >", P.fs_step);
    else { *kind = VK_ACTION; snprintf(buf, len, "%d foto  SET >", P.fs_front + P.fs_behind + 1); }
}

static void fs_change(int row, int dir, int big)
{
    if (row == 0) P.fs_front = COERCE(P.fs_front + dir * (big ? 5 : 1), 0, 50);
    else if (row == 1) P.fs_behind = COERCE(P.fs_behind + dir * (big ? 5 : 1), 0, 50);
    else if (row == 2) P.fs_step = COERCE(P.fs_step + dir, 1, 10);
    persist_touch();
}

static void fs_activate(int row)
{
    if (row != 3) return;
    if (!lv) { say(C_WARN, "Focus stacking: accendi il Live View"); return; }
    if (is_manual_focus()) { say(C_WARN, "Metti l'obiettivo in AF (lo muove Iride)"); return; }
    if (!ml_set(MLF_FS_FRONT, P.fs_front) || !ml_set(MLF_FS_BEHIND, P.fs_behind) || !ml_set(MLF_FS_STEP, P.fs_step))
    { say(C_WARN, "Focus stacking non disponibile"); return; }
    job_request = JOB_FSTACK;
    gui_stop_menu();
}

static void fs_extra()
{
    txt(FONT_MED, C_DIM, 30, 250, "Treppiede, Live View, obiettivo in AF.");
    txt(FONT_MED, C_DIM, 30, 276, "Metti a fuoco sul punto centrale: Iride scatta le foto");
    txt(FONT_MED, C_DIM, 30, 302, "davanti e dietro spostando il fuoco. MENU per fermare.");
}

static const struct form fs_form = { 4, fs_labels, fs_value, fs_change, fs_activate, fs_extra,
    "^/v riga   </> cambia   SET avvia   MENU indietro", 58, 46 };

/* ------------------------------------------------------------------ Dual ISO */

static const int dual_iso_vals[] = { 3, 4, 5 };
static const char *const dual_iso_names[] = { "800", "1600", "3200" };
static const char *const dual_labels[] = { "Dual ISO", "ISO di recupero" };

static void dual_value(int row, char *buf, int len, int *kind)
{
    if (row == 0) ml_status(MLF_DUAL, buf, len, kind);
    else snprintf(buf, len, "< %s >", dual_iso_names[COERCE(P.dual_rec, 0, 2)]);
}

static void dual_change(int row, int dir, int big)
{
    if (row == 0)
    {
        if (ml_get(MLF_DUAL) == 0) ml_set(MLF_DUAL_REC, dual_iso_vals[COERCE(P.dual_rec, 0, 2)]);
        ml_toggle(MLF_DUAL, "Dual ISO");
        return;
    }
    P.dual_rec = COERCE(P.dual_rec + dir, 0, 2);
    persist_touch();
    if (ml_get(MLF_DUAL) == 1) ml_set(MLF_DUAL_REC, dual_iso_vals[P.dual_rec]);
}

static void dual_activate(int row) { dual_change(row, 1, 0); }

static void dual_extra()
{
    txt(FONT_MED, C_DIM, 30, 170, "Le righe del sensore escono a due ISO diversi:");
    txt(FONT_MED, C_DIM, 30, 196, "fino a 3 stop di gamma dinamica in piu' nel RAW.");
    txt(FONT_MED, C_DIM, 30, 236, "Solo RAW, da convertire sul PC con cr2hdr.");
    txt(FONT_MED, C_DIM, 30, 262, "ISO base basso (100) e ISO di recupero alto.");
}

static const struct form dual_form = { 2, dual_labels, dual_value, dual_change, dual_activate, dual_extra,
    "^/v riga   </> cambia   SET attiva   MENU indietro", 58, 46 };

/* ------------------------------------------------------------------ ETTR */

static const char *const ettr_mode_names[] = { "Sempre", "Auto Snap", "Premi SET", "Doppio mezzo scatto" };
static const char *const ettr_labels[] = { "Auto ETTR", "Quando" };

static void ettr_value(int row, char *buf, int len, int *kind)
{
    if (row == 0) ml_status(MLF_ETTR, buf, len, kind);
    else snprintf(buf, len, "< %s >", ettr_mode_names[COERCE(P.ettr_mode, 0, 3)]);
}

static void ettr_change(int row, int dir, int big)
{
    if (row == 0)
    {
        if (ml_get(MLF_ETTR) == 0) ml_set(MLF_ETTR_TRIG, P.ettr_mode);
        ml_toggle(MLF_ETTR, "ETTR");
        return;
    }
    P.ettr_mode = COERCE(P.ettr_mode + dir, 0, 3);
    persist_touch();
    if (ml_get(MLF_ETTR) == 1) ml_set(MLF_ETTR_TRIG, P.ettr_mode);
}

static void ettr_activate(int row) { ettr_change(row, 1, 0); }

static void ettr_extra()
{
    txt(FONT_MED, C_DIM, 30, 170, "Esposizione automatica \"a destra\": la luce piu' alta");
    txt(FONT_MED, C_DIM, 30, 196, "possibile senza bruciare le alte luci del RAW.");
    txt(FONT_MED, C_DIM, 30, 236, "Ghiera su M. Ideale con paesaggi e notturni.");
}

static const struct form ettr_form = { 2, ettr_labels, ettr_value, ettr_change, ettr_activate, ettr_extra,
    "^/v riga   </> cambia   SET attiva   MENU indietro", 58, 46 };

/* ------------------------------------------------------------------ movimento e trappola di fuoco */

static const char *const md_trig_names[] = { "Cambio di luce", "Differenza immagine", "Mani ferme" };
static const char *const trap_names[] = { "OFF", "Tieni premuto AF", "Continua" };
static const char *const motion_labels[] = { "Scatto su movimento", "Rileva", "Sensibilita'", "Trappola di fuoco" };

static void motion_value(int row, char *buf, int len, int *kind)
{
    if (row == 0) ml_status(MLF_MD, buf, len, kind);
    else if (row == 1) snprintf(buf, len, "< %s >", md_trig_names[COERCE(P.md_trigger, 0, 2)]);
    else if (row == 2) snprintf(buf, len, "< %d >", P.md_level);
    else
    {
        int v = ml_get(MLF_TRAP);
        if (v == INT_MIN) { snprintf(buf, len, "non disponibile"); *kind = VK_WARN; }
        else snprintf(buf, len, "< %s >", trap_names[COERCE(v, 0, 2)]);
    }
}

static void motion_change(int row, int dir, int big)
{
    if (row == 0)
    {
        if (ml_get(MLF_MD) == 0) { ml_set(MLF_MD_TRIG, P.md_trigger); ml_set(MLF_MD_LEVEL, P.md_level); }
        ml_toggle(MLF_MD, "Movimento");
        return;
    }
    if (row == 3)
    {
        int v = ml_get(MLF_TRAP);
        if (v != INT_MIN) ml_set(MLF_TRAP, COERCE(v + dir, 0, 2));
        return;
    }
    if (row == 1) P.md_trigger = COERCE(P.md_trigger + dir, 0, 2);
    else P.md_level = COERCE(P.md_level + dir * (big ? 5 : 1), 1, 30);
    persist_touch();
    if (ml_get(MLF_MD) == 1) { ml_set(MLF_MD_TRIG, P.md_trigger); ml_set(MLF_MD_LEVEL, P.md_level); }
}

static void motion_activate(int row) { motion_change(row, 1, 0); }

static void motion_extra()
{
    txt(FONT_MED, C_DIM, 30, 260, "Movimento: scatta da solo quando qualcosa cambia");
    txt(FONT_MED, C_DIM, 30, 286, "nel Live View (fulmini, animali). Livello basso = piu' sensibile.");
    txt(FONT_MED, C_DIM, 30, 324, "Trappola di fuoco: scatta quando il soggetto");
    txt(FONT_MED, C_DIM, 30, 350, "entra a fuoco (obiettivo in MF).");
}

static const struct form motion_form = { 4, motion_labels, motion_value, motion_change, motion_activate, motion_extra,
    "^/v riga   </> cambia   SET attiva   MENU indietro", 58, 46 };

/* ------------------------------------------------------------------ strumenti del Live View */

static const int lv_feats[] = { MLF_PEAK, MLF_ZEBRA, MLF_HIST, MLF_FALSE, MLF_ZOOM, MLF_SPOT };
static const char *const lv_names[] = { "Focus peaking", "Zebra", "Istogramma", "Falsi colori", "Zoom di fuoco", "Esposimetro spot" };
static const char *const lv_labels[] = { "Focus peaking", "Zebra", "Istogramma", "Falsi colori", "Zoom di fuoco", "Esposimetro spot", "Spegni tutto" };

static void lv_value(int row, char *buf, int len, int *kind)
{
    if (row < 6) ml_status(lv_feats[row], buf, len, kind);
    else { *kind = VK_ACTION; snprintf(buf, len, "SET"); }
}

static void lv_change(int row, int dir, int big)
{
    if (row >= 6) return;
    int f = lv_feats[row];
    int v = ml_get(f);
    if (v == INT_MIN) { say(C_WARN, "Funzione non disponibile"); return; }
    if (!v && ml_get(MLF_GDRAW) == 0 && !ml_set(MLF_GDRAW, 3)) ml_set(MLF_GDRAW, 1);   /* serve il disegno globale */
    ml_toggle(f, lv_names[row]);
}

static void lv_activate(int row)
{
    if (row < 6) { lv_change(row, 1, 0); return; }
    for (int i = 0; i < 6; i++) if (ml_get(lv_feats[i]) > 0) ml_set(lv_feats[i], 0);
    say(C_OK, "Strumenti del Live View spenti");
}

static const struct form lv_form = { 7, lv_labels, lv_value, lv_change, lv_activate, 0,
    "^/v riga   SET o </> accendi/spegni   MENU indietro", 58, 46 };

/* ------------------------------------------------------------------ filtri ND */

static const int nd_thirds[] = { 3, 6, 9, 12, 15, 18, 26, 30, 45 };   /* stop x 3 */
static const char *const nd_names[] = { "ND2", "ND4", "ND8", "ND16", "ND32", "ND64", "ND400", "ND1000", "ND32000" };
#define NUM_ND ((int)COUNT(nd_thirds))
static volatile int job_bulb_s = 0;

static int valid_third(int raw) { return raw % 8 == 0 || raw % 8 == 3 || raw % 8 == 5; }

static void nd_enter()
{
    int raw = lens_info.raw_shutter;
    if (raw >= 16 && raw <= 152 && valid_third(raw)) P.nd_base = raw;
    if (P.nd_base < 16 || P.nd_base > 152) P.nd_base = 104;   /* 1/60 */
}

/* tempo col filtro in decimi di secondo */
static int nd_result_tenths()
{
    float base = raw2shutterf(P.nd_base);
    float stops = (float)nd_thirds[COERCE(P.nd_idx, 0, NUM_ND - 1)] / 3.0f;
    return (int)(base * powf(2.0f, stops) * 10.0f + 0.5f);
}

static void fmt_duration(char *buf, int len, int t10)
{
    if (t10 < 10) { snprintf(buf, len, "1/%d s", (int)(10.0f / (float)MAX(t10, 1) + 0.5f)); return; }
    int s = t10 / 10;
    if (s < 60) { snprintf(buf, len, "%d.%d s", s, t10 % 10); return; }
    if (s < 3600) { snprintf(buf, len, "%d min %02d s", s / 60, s % 60); return; }
    snprintf(buf, len, "%d h %02d min", s / 3600, (s % 3600) / 60);
}

static const char *const nd_labels[] = { "Tempo senza filtro", "Filtro", "Tempo col filtro", "Applica / scatta" };

static void nd_value(int row, char *buf, int len, int *kind)
{
    if (row == 0) snprintf(buf, len, "< %s >", lens_format_shutter(P.nd_base));
    else if (row == 1) snprintf(buf, len, "< %s  (%d stop%s) >", nd_names[COERCE(P.nd_idx, 0, NUM_ND - 1)],
        nd_thirds[COERCE(P.nd_idx, 0, NUM_ND - 1)] / 3, nd_thirds[COERCE(P.nd_idx, 0, NUM_ND - 1)] % 3 ? "+" : "");
    else if (row == 2) { fmt_duration(buf, len, nd_result_tenths()); *kind = VK_TEXT; }
    else { *kind = VK_ACTION; snprintf(buf, len, nd_result_tenths() > 300 ? "posa B  SET >" : "imposta  SET >"); }
}

static void nd_change(int row, int dir, int big)
{
    if (row == 0)
    {
        /* passi di 1/3 di stop: tempo piu' lungo = codice piu' basso */
        int raw = P.nd_base;
        do raw -= dir; while (raw > 16 && raw < 152 && !valid_third(raw));
        P.nd_base = COERCE(raw, 16, 152);
    }
    else if (row == 1) P.nd_idx = COERCE(P.nd_idx + dir, 0, NUM_ND - 1);
    persist_touch();
}

static void nd_activate(int row)
{
    if (row != 3) return;
    int t10 = nd_result_tenths();
    if (t10 <= 300)
    {
        if (shooting_mode != SHOOTMODE_M && shooting_mode != SHOOTMODE_TV) { say(C_WARN, "Metti la ghiera su M o Tv"); return; }
        snap_take();
        int raw = shutterf_to_raw((float)t10 / 10.0f);
        while (raw < 152 && !valid_third(raw)) raw++;
        lens_set_rawshutter(MAX(raw, 16));
        char buf[64];
        snprintf(buf, sizeof(buf), "Tempo impostato: %s", lens_format_shutter(lens_info.raw_shutter));
        say(C_OK, buf);
        return;
    }
    if (shooting_mode != SHOOTMODE_M && shooting_mode != SHOOTMODE_BULB) { say(C_WARN, "Posa B: metti la ghiera su M o B"); return; }
    job_bulb_s = (t10 + 5) / 10;
    job_request = JOB_BULB;
    gui_stop_menu();
}

static void nd_extra()
{
    txt(FONT_MED, C_DIM, 30, 260, "Misura senza filtro, poi monta il filtro:");
    txt(FONT_MED, C_DIM, 30, 286, "fino a 30 s Iride imposta il tempo, oltre scatta");
    txt(FONT_MED, C_DIM, 30, 312, "in posa B col conto alla rovescia. Cestino = stop.");
}

static const struct form nd_form = { 4, nd_labels, nd_value, nd_change, nd_activate, nd_extra,
    "^/v riga   </> cambia   SET applica   MENU indietro", 58, 46 };

/* ------------------------------------------------------------------ menu Foto */

static const struct menu_row foto_rows[] = {
    { "Bracketing HDR", "Serie di scatti a esposizioni diverse", SCR_HDR },
    { "Focus stacking", "Fuoco spostato tra uno scatto e l'altro", SCR_FSTACK },
    { "Dual ISO", "Piu' gamma dinamica nel RAW", SCR_DUAL },
    { "ETTR", "Esposizione automatica a destra", SCR_ETTR },
    { "Movimento e trappola di fuoco", "Scatta da solo: fulmini, animali, fuoco", SCR_MOTION },
    { "Strumenti Live View", "Peaking, zebra, istogramma, falsi colori", SCR_LVTOOLS },
    { "Filtri ND", "Tempo col filtro e posa B automatica", SCR_ND },
};
#define NUM_FOTO_ROWS ((int)COUNT(foto_rows))

/* ------------------------------------------------------------------ lavori a Iride chiusa */

static int job_index = 0, job_total = 0;
static const char *job_name = "";

static void job_overlay(const char *state)
{
    bmp_printf(FONT(FONT_MED, COLOR_WHITE, COLOR_BLACK), 10, 10, " IRIDE  %s  %d/%d  %s   [cestino = stop] ",
        job_name, job_index, job_total, state);
}

static int job_wait(int seconds, const char *what)
{
    for (int s = seconds; s > 0; s--)
    {
        char st[32];
        snprintf(st, sizeof(st), "%s %ds", what, s);
        job_overlay(st);
        for (int k = 0; k < 10; k++)
        {
            if (job_abort || ml_shutdown_requested) return 1;
            msleep(100);
        }
    }
    return 0;
}

static void seq_run()
{
    int preset = COERCE(P.int_preset, 0, NUM_PRESETS - 1);
    const struct preset *ps = &presets[preset];
    job_name = ps->name;
    job_total = P.int_count;
    job_index = 0;
    seq_tag = ps->tag;
    int bulb = P.int_bulb;
    int is_bias = (preset == PRESET_BIAS), is_hg = (preset == PRESET_HG);
    int done = 0;
    prefix_refresh();

    if (is_hg) { ml_set(MLF_AUTOEXPO, 1); ml_set(MLF_DEFLICK, 1); }

    if (!job_wait(delay_steps[COERCE(P.int_delay_idx, 0, (int)COUNT(delay_steps) - 1)], "inizio tra"))
    {
        int saved_shutter = lens_info.raw_shutter;
        if (is_bias) { lens_set_rawshutter(shutterf_to_raw(1.0f / 4000.0f)); msleep(200); }
        int screen_off = P.int_screen_off && !is_hg;   /* il giorno-notte ha bisogno del Live View */
        if (screen_off) display_off();

        for (int i = 0; i < job_total; i++)
        {
            if (job_abort || ml_shutdown_requested) break;
            job_index = i + 1;
            job_overlay("scatto");
            if (bulb > 0)
            {
                if (bulb_take_pic(bulb * 1000)) { job_abort = 1; break; }
            }
            else
            {
                float t = raw2shutterf(lens_info.raw_shutter);
                lens_take_picture(64 + 2 * (int)t, AF_DISABLE);
            }
            done++;
            if (i < job_total - 1 && job_wait(P.int_pause, "pausa")) break;
        }
        if (screen_off) display_on();
        if (is_bias) lens_set_rawshutter(saved_shutter);
    }
    if (is_hg) { ml_restore(MLF_AUTOEXPO); ml_restore(MLF_DEFLICK); }

    char line[96];
    snprintf(line, sizeof(line), "ASTRO,%s,%s,%d,%d,%d", ps->name, tag_names[seq_tag], done, job_total, bulb);
    log_line(line);
    beep_times(2);
    NotifyBox(6000, "Iride: %s %s, %d/%d scatti", ps->name, job_abort ? "interrotta" : "finita", done, job_total);
}

static void bulb_run()
{
    job_name = "Posa ND";
    job_total = 1;
    job_index = 1;
    if (job_wait(3, "inizio tra")) return;
    snap_take();
    int canceled = bulb_take_pic(job_bulb_s * 1000);
    beep_times(2);
    char d[24];
    fmt_duration(d, sizeof(d), job_bulb_s * 10);
    NotifyBox(5000, "Iride: posa B %s %s", d, canceled ? "interrotta" : "finita");
}

static void job_task(void *unused)
{
    job_abort = 0;
    switch (job_running)
    {
        case JOB_SEQ:    seq_run(); break;
        case JOB_FSTACK: NotifyBox(2000, "Iride: focus stack, MENU per fermare"); msleep(1500); focus_stack_run(0); break;
        case JOB_BULB:   bulb_run(); break;
    }
    /* l'ultimo file puo' arrivare dopo l'ultimo controllo del contatore: riallinea */
    msleep(500);
    if (P.roll_loaded) P.roll_last_fn = current_file_number();
    job_running = JOB_NONE;
    persist_touch();
    prefix_refresh();
    persist_save_if_dirty();
}
