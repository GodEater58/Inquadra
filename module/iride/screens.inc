/* screens.inc - Home, menu, scaffale dei rullini, dialoghi, profili, opzioni,
 * "Torna alla 6D", tabella delle schermate, disegno e gestione dei tasti */

static int key_dx(int key)
{
    if (key == MODULE_KEY_PRESS_RIGHT || key == MODULE_KEY_WHEEL_RIGHT) return 1;
    if (key == MODULE_KEY_PRESS_LEFT  || key == MODULE_KEY_WHEEL_LEFT)  return -1;
    return 0;
}

static int key_dy(int key)
{
    if (key == MODULE_KEY_PRESS_DOWN || key == MODULE_KEY_WHEEL_DOWN) return 1;
    if (key == MODULE_KEY_PRESS_UP   || key == MODULE_KEY_WHEEL_UP)   return -1;
    return 0;
}

static int key_big(int key) { return key == MODULE_KEY_WHEEL_LEFT || key == MODULE_KEY_WHEEL_RIGHT; }

/* ------------------------------------------------------------------ cornice di selezione elastica */

/* come la barra delle liste: il lato che guida arriva prima, l'altro lo segue */
static int hr_to[4] = { -1 }, hr_lead[4], hr_tail[4], hr_fl[4], hr_ft[4], hr_t0;

static void hr_frame(int x, int y, int w, int h)
{
    int to[4] = { x, y, x + w, y + h };
    int now = get_ms_clock(), d1 = anim_ms(120), d2 = anim_ms(250);
    if (hr_to[0] < 0 || !d1)
    {
        for (int i = 0; i < 4; i++) hr_to[i] = hr_lead[i] = hr_tail[i] = hr_fl[i] = hr_ft[i] = to[i];
        hr_t0 = now - 100000;
    }
    else if (memcmp(to, hr_to, sizeof(to)))
    {
        for (int i = 0; i < 4; i++) { hr_fl[i] = hr_lead[i]; hr_ft[i] = hr_tail[i]; hr_to[i] = to[i]; }
        hr_t0 = now;
    }
    int t = now - hr_t0, e1 = ease(t, d1), e2 = ease(t, d2);
    for (int i = 0; i < 4; i++) { hr_lead[i] = lerp(hr_fl[i], hr_to[i], e1); hr_tail[i] = lerp(hr_ft[i], hr_to[i], e2); }
    if (t < d2) anim_keep();
    int x0 = MIN(hr_lead[0], hr_tail[0]), y0 = MIN(hr_lead[1], hr_tail[1]);
    int x1 = MAX(hr_lead[2], hr_tail[2]), y1 = MAX(hr_lead[3], hr_tail[3]);
    ring_r(C_ACCENT, x0 - 3, y0 - 3, x1 - x0 + 6, y1 - y0 + 6, R_CARD + 3, 3);
}

/* ------------------------------------------------------------------ Home */

/* riquadri: 3 grandi sopra, 3 piccoli sotto */
static void home_tile(int i, int *x, int *y, int *w, int *h)
{
    *x = 24 + (i % 3) * 230; *y = i < 3 ? 62 : 272; *w = 212; *h = i < 3 ? 196 : 84;
}

static int home_intro_t0 = 0;            /* apertura dell'app: i riquadri salgono uno dopo l'altro */
static int icon_t0 = -100000, icon_i = -1;   /* animazione dell'icona appena scelta */

/* spostamento verticale del riquadro i durante l'entrata (HIDDEN = non ancora visibile) */
#define HIDDEN 9999
static int home_intro_dy(int i)
{
    if (!home_intro_t0) return 0;
    static const short delay[6] = { 0, 60, 120, 150, 200, 250 };
    int dur = anim_ms(360), t = get_ms_clock() - home_intro_t0 - anim_ms(delay[i]);
    if (t < 0) { anim_keep(); return HIDDEN; }
    if (t >= dur) return 0;
    anim_keep();
    return (1000 - ease_k(t, dur, EZ_BACK)) * 40 / 1000;
}

static int icon_time(int i)
{
    if (i != icon_i || opt_anim == 2) return -1;
    int t = get_ms_clock() - icon_t0;
    if (t < 0) { anim_keep(); return -1; }
    if (t > 950) return -1;
    anim_keep();
    return opt_anim == 1 ? t * 10 / 6 : t;
}

static void draw_home()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("6D");
    int s = sel[SCR_HOME];
    int intro = 0;
    if (home_intro_t0 && get_ms_clock() - home_intro_t0 > anim_ms(250) + anim_ms(360) + 450) home_intro_t0 = 0;

    const char *big[3] = { "ASTRO", "FOTO", "RULLINI" };
    const char *sub[3] = { "NPF, polare, cielo", "HDR, stack, ND", "" };
    for (int i = 0; i < 3; i++)
    {
        int dy = home_intro_dy(i);
        if (dy == HIDDEN) { intro = 1; continue; }
        if (dy) intro = 1;
        int x = 24 + i * 230, y = 62 + dy;
        int bg = i == s ? C_SEL : C_PANEL;
        fill_r(bg, x, y, 212, 196, R_CARD);
        int t = icon_time(i);
        if (i == 0) icon_sky(x + 11, y + 8, bg, t);
        else if (i == 1) icon_dslr(x + 31, y + 6, bg, t);
        else icon_rolls(P.roll_loaded ? cur_film() : &films[1], x + 6, y + 6, t);
        txt(FONT_LARGE, C_TEXT, x + 16, y + 118, big[i]);
        char line[48];
        if (i == 2)
        {
            if (P.roll_loaded) snprintf(line, sizeof(line), "%s %d/%d%s", cur_film()->name, P.roll_shots, P.roll_exp, P.roll_paused ? " II" : "");
            else snprintf(line, sizeof(line), "Nessun rullino");
            txt_fit(C_DIM, x + 16, y + 158, 212 - 24, line);
        }
        else txt(FONT_MED, C_DIM, x + 16, y + 158, sub[i]);
        if (dy && i == s) ring_r(C_ACCENT, x - 3, y - 3, 218, 202, R_CARD + 3, 3);
    }

    const char *small[3] = { "Torna alla 6D", "Profili", "Opzioni" };
    for (int i = 0; i < 3; i++)
    {
        int dy = home_intro_dy(i + 3);
        if (dy == HIDDEN) { intro = 1; continue; }
        if (dy) intro = 1;
        int x = 24 + i * 230, y = 272 + dy;
        int bg = i + 3 == s ? C_SEL : C_PANEL;
        fill_r(bg, x, y, 212, 84, R_CARD);
        int t = icon_time(i + 3);
        if (i == 0) icon_undo(x + 34, y + 42, t);
        else if (i == 1) icon_faders(x + 34, y + 42, t);
        else icon_gears(x + 32, y + 46, bg, t);
        txt(FONT_MED, C_TEXT, x + 66, y + 14, small[i]);
        char line[40];
        if (i == 0)
        {
            int n = restore_pending_count();
            if (n) snprintf(line, sizeof(line), "%d modifiche", n); else snprintf(line, sizeof(line), "tutto originale");
            txt(FONT_SMALL, n ? C_ACCENT : C_DIM, x + 66, y + 50, line);
        }
        else if (i == 1)
        {
            int n = 0;
            for (int k = 0; k < NPROF; k++) n += P.prof[k].valid;
            snprintf(line, sizeof(line), "%d salvati su %d", n, NPROF);
            txt(FONT_SMALL, C_DIM, x + 66, y + 50, line);
        }
        else txt(FONT_SMALL, C_DIM, x + 66, y + 50, night ? "tema notte" : "tema, tasti, luogo");
        if (dy && i + 3 == s) ring_r(C_ACCENT, x - 3, y - 3, 218, 90, R_CARD + 3, 3);
    }

    int tx, ty, tw, th;
    home_tile(s, &tx, &ty, &tw, &th);
    if (!intro) hr_frame(tx, ty, tw, th);
    else hr_to[0] = -1;

    /* righe dell'obiettivo: si accendono dopo i riquadri */
    int ei = home_intro_t0 ? phase(get_ms_clock() - home_intro_t0, anim_ms(300), anim_ms(260), EZ_OUT) : 1000;
    if (ei < 1000) anim_keep();
    /* striscia di stato: obiettivo e impostazioni come etichette */
    char buf[96], ap[8];
    if (ei > 0)
    {
        int dy = 8 * (1000 - ei) / 1000, y = 366 + dy;
        fill_r(C_PANEL, 24, y, SCR_W - 48, 72, R_CARD);
        /* obiettivo disegnato: anelli e vetro con il riflesso */
        int lx = 58, ly = y + 36;
        disc(lx, ly, 21, C_LINE);
        disc(lx, ly, 18, C_BG);
        disc(lx, ly, 13, C_PANEL2);
        draw_circle(lx, ly, 9, C_ACCENT);
        disc(lx - 5, ly - 5, 3, C_DIM);
        int tx = 94;
        if (lens_info.lens_exists && lens_info.name[0])
        {
            fmt_tenths(ap, sizeof(ap), lens_info.aperture);
            txt(FONT_MED, C_TEXT, tx, y + 4, lens_info.name);
            snprintf(buf, sizeof(buf), "%dmm  f/%s", (int)lens_info.focal_len, ap);
            txt(FONT_MED, C_ACCENT, tx + txt_w(FONT_MED, lens_info.name) + 14, y + 4, buf);
        }
        else txt(FONT_MED, C_DIM, tx, y + 4, "Obiettivo manuale o non rilevato");
        int cx = tx, cy = y + 36, cb = C_SEL;
        cx += chip_kv(cx, cy, "GHIERA", get_shootmode_name_short(shooting_mode), cb, C_ACCENT) + 8;
        cx += chip_kv(cx, cy, "ISO", lens_format_iso(lens_info.raw_iso), cb, C_TEXT) + 8;
        cx += chip_kv(cx, cy, "STILE", style_name(lens_info.raw_picstyle), cb, C_TEXT) + 8;
        snprintf(buf, sizeof(buf), "%s", prefix_cur[0] ? prefix_cur : "IMG_");
        if (cx + 100 < SCR_W - 30) chip_kv(cx, cy, "FILE", buf, cb, C_TEXT);
    }
    draw_footer("joystick scegli  SET apri  INFO aiuto  MENU esci  cestino = menu ML");
}

/* da/verso la Home: lo zoom parte dal riquadro (cornice compresa) */
static void zoom_from_tile(int i)
{
    int x, y, w, h;
    home_tile(i, &x, &y, &w, &h);
    zoom_x = x - 3; zoom_y = y - 3; zoom_w = w + 6; zoom_h = h + 6;
}

static const int home_target[6] = { SCR_ASTRO, SCR_FOTO, SCR_SHELF, SCR_RESTORE, SCR_PROFILES, SCR_OPTIONS };

static int home_tile_of(int scr)
{
    for (int i = 0; i < 6; i++) if (home_target[i] == scr) return i;
    return -1;
}

static void home_key(int key, int dx, int dy)
{
    int s = sel[SCR_HOME];
    if (dx) s = (s / 3) * 3 + MOD(s % 3 + dx, 3);
    else if (dy) s = MOD(s + 3 * dy, 6);
    else if (key == MODULE_KEY_PRESS_SET) { zoom_from_tile(s); go(home_target[s], 3); return; }
    if (s != sel[SCR_HOME]) { icon_t0 = get_ms_clock() + 60; icon_i = s; }
    sel[SCR_HOME] = s;
}

static void home_enter() { hr_to[0] = -1; icon_i = -1; }

/* ------------------------------------------------------------------ menu Astro e Foto */

/* lista a sinistra con le icone, a destra la voce scelta: illustrazione animata,
   a cosa serve e com'e' adesso */
struct md_item { void (*icon)(int cx, int cy, int s, int t); const char *desc; };

static const struct md_item astro_md[] = {
    { ic_milkyway,  "Calcola il tempo piu' lungo prima che le stelle diventino trattini, dalla focale e dal diaframma, e lo imposta con ISO e bianco." },
    { ic_interval,  "Sequenze automatiche con preset per Via Lattea, deep sky, star trails, timelapse, giorno-notte e i frame di calibrazione." },
    { ic_starfocus, "Misura la nitidezza di una stella (HFR) in Live View mentre giri il fuoco: piu' basso il numero, piu' e' a fuoco." },
    { ic_polar,     "Dove mettere la Polare nel cannocchiale polare, con la mappa del cielo a nord per trovarla a occhio." },
    { ic_calendar,  "Luna, buio e Via Lattea per le prossime 7 notti, ora per ora, con la notte migliore della settimana." },
    { ic_mwplan,    "Dove sara' il centro della Via Lattea ora per ora: direzione, altezza e inclinazione dell'arco sull'orizzonte." },
    { ic_moonshot,  "Fase, direzione e altezza della Luna adesso, e il tempo di scatto giusto per la superficie o la luce cinerea." },
    { ic_tag,       "Nomi dei file LGHT, DARK, FLAT o BIAS al posto di IMG_, per ritrovarli nello stacking." },
};
static const struct md_item foto_md[] = {
    { ic_hdr,     "Ogni pressione dello scatto diventa una serie di foto a esposizioni diverse, da unire in HDR." },
    { ic_fstack,  "Piu' foto con il fuoco spostato tra l'una e l'altra, per avere tutto nitido: macro e paesaggi." },
    { ic_dual,    "Due ISO nella stessa foto: piu' gamma dinamica nel RAW, da convertire sul PC con cr2hdr." },
    { ic_ettr,    "Esposizione automatica a destra: la luce piu' alta possibile senza bruciare le alte luci." },
    { ic_motion,  "Scatta da solo quando qualcosa si muove (fulmini, animali) o quando il soggetto entra a fuoco." },
    { ic_lvtools, "Peaking, zebra, istogramma, falsi colori, zoom di fuoco ed esposimetro spot nel Live View." },
    { ic_nd,      "Il tempo giusto con un filtro ND, e lo scatto in posa B quando si superano i 30 secondi." },
};

static int md_t0 = -100000, md_txt_t0 = -100000;   /* icona grande e testi della voce scelta */
static int md_stat_t = -100000;                      /* stato delle voci: ricalcolato ogni 300 ms */
static void md_enter() { md_t0 = get_ms_clock(); md_txt_t0 = -100000; md_stat_t = -100000; }
static void md_select() { md_t0 = md_txt_t0 = get_ms_clock(); md_stat_t = -100000; }

static void astro_status(int i, char *buf, int len, int *on)
{
    char a[12];
    *on = 0;
    int d70, mins, off, wax;
    switch (astro_rows[i].target)
    {
        case SCR_NPF:
        {
            fmt_tenths(a, sizeof(a), npf_tenths_dec(P.npf_focal, P.npf_ap10, npf_dec()));
            char b[12];
            fmt_tenths(b, sizeof(b), P.npf_ap10);
            snprintf(buf, len, "%s s con %d mm f/%s", a, P.npf_focal, b);
            break;
        }
        case SCR_INTERVAL: snprintf(buf, len, "%s, %d scatti", presets[COERCE(P.int_preset, 0, NUM_PRESETS - 1)].name, P.int_count); break;
        case SCR_STARS: *on = lv; snprintf(buf, len, lv ? "Live View acceso: pronta" : "START/STOP accende il Live View"); break;
        case SCR_POLAR:
        {
            int h, m;
            polar_clock(&h, &m);
            snprintf(buf, len, "Polare alle ore %d:%02d", h, m);
            break;
        }
        case SCR_SKY:
        {
            struct jtime j = jt_now(&d70, &mins, &off);
            int ill = moon_illum(j, &wax);
            snprintf(buf, len, "%s, %d%%", moon_phase_name(ill, wax), (ill + 5) / 10);
            break;
        }
        case SCR_MWPLAN:
        {
            struct jtime j = jt_now(&d70, &mins, &off);
            float al, az;
            alt_az(GC_RA, GC_DEC, lst_deg(j, lon_deg()), lat_deg(), &al, &az);
            *on = al > 10.0f;
            if (al > 0.0f) snprintf(buf, len, "Adesso verso %s, alto %d gradi", compass16(az), (int)(al + 0.5f));
            else snprintf(buf, len, "Adesso sotto l'orizzonte");
            break;
        }
        case SCR_MOON:
        {
            struct jtime j = jt_now(&d70, &mins, &off);
            float ra, dec, al, az;
            moon_pos(j, &ra, &dec, 0);
            alt_az(ra, dec, lst_deg(j, lon_deg()), lat_deg(), &al, &az);
            int ill = moon_illum(j, &wax);
            *on = al > 0.0f;
            if (al > 0.0f) snprintf(buf, len, "Verso %s, alta %d gradi, %d%%", compass16(az), (int)(al + 0.5f), (ill + 5) / 10);
            else snprintf(buf, len, "Sotto l'orizzonte, %d%%", (ill + 5) / 10);
            break;
        }
        default:
            *on = P.astro_tag != 0;
            snprintf(buf, len, P.astro_tag ? "Nomi dei file: %s_" : "Nomi dei file normali (IMG_)", tag_names[COERCE(P.astro_tag, 0, NUM_TAGS - 1)]);
            break;
    }
}

static void foto_status(int i, char *buf, int len, int *on)
{
    *on = 0;
    int v;
    switch (i)
    {
        case 0:
            v = ml_get(MLF_HDR);
            *on = v == 1;
            if (v == INT_MIN) snprintf(buf, len, "modulo di Magic Lantern spento");
            else if (v == 1) snprintf(buf, len, "attivo: %d scatti da %s", P.hdr_frames, hdr_ev_names[COERCE(P.hdr_step, 0, 3)]);
            else snprintf(buf, len, "spento");
            break;
        case 1: snprintf(buf, len, "%d foto, passo di fuoco %d", P.fs_front + P.fs_behind + 1, P.fs_step); break;
        case 2:
            v = ml_get(MLF_DUAL);
            *on = v == 1;
            if (v == INT_MIN) snprintf(buf, len, "modulo dual_iso spento");
            else snprintf(buf, len, v == 1 ? "attivo, recupero ISO %s" : "spento", dual_iso_names[COERCE(P.dual_rec, 0, 2)]);
            break;
        case 3:
            v = ml_get(MLF_ETTR);
            *on = v == 1;
            if (v == INT_MIN) snprintf(buf, len, "modulo ettr spento");
            else snprintf(buf, len, v == 1 ? "attivo: %s" : "spento", ettr_mode_names[COERCE(P.ettr_mode, 0, 3)]);
            break;
        case 4:
        {
            int md = ml_get(MLF_MD) == 1, tr = ml_get(MLF_TRAP);
            *on = md || tr > 0;
            snprintf(buf, len, md ? "scatto su movimento attivo" : tr > 0 && tr != INT_MIN ? "trappola di fuoco attiva" : "spenti");
            break;
        }
        case 5:
        {
            int n = 0;
            for (int k = 0; k < 6; k++) n += ml_get(lv_feats[k]) == 1;
            *on = n > 0;
            if (n) snprintf(buf, len, "%d strument%s acces%s", n, n == 1 ? "o" : "i", n == 1 ? "o" : "i");
            else snprintf(buf, len, "tutti spenti");
            break;
        }
        default:
        {
            char d[24];
            fmt_duration(d, sizeof(d), nd_result_tenths());
            snprintf(buf, len, "%s: %s diventa %s", nd_names[COERCE(P.nd_idx, 0, NUM_ND - 1)], lens_format_shutter(P.nd_base), d);
        }
    }
}

/* icone piccole delle liste: immagini pronte (icons_small.inc), scritte a righe */
#include "icons_small.inc"
static int ics_index(void (*f)(int, int, int, int))
{
    static void (*const fn[])(int, int, int, int) = { ic_milkyway, ic_interval, ic_starfocus, ic_polar, ic_calendar,
        ic_mwplan, ic_moonshot, ic_tag, ic_hdr, ic_fstack, ic_dual, ic_ettr, ic_motion, ic_lvtools, ic_nd };
    for (int i = 0; i < (int)COUNT(fn); i++) if (fn[i] == f) return i;
    return -1;
}

static void draw_sprite(int x, int y, int idx)
{
    unsigned char row[ICS_N];
    const unsigned char *px = ics_px[idx];
    for (int r = 0; r < ICS_N; r++)
    {
        for (int i = 0; i < ICS_N; i++)
        {
            int v = px[r * ICS_N + i];
            if (v && night)
            {
                /* tema notte: solo rossi, le sfumature scure spariscono */
                int l = gray_level(v);
                v = l < 0 ? COLOR_RED : l < 70 ? 0 : l < 150 ? COLOR_DARK_RED : COLOR_RED;
            }
            row[i] = (unsigned char)v;
        }
        put_row(x, y + r, row, ICS_N);
    }
}

static void draw_menu_md(const struct menu_row *rows, const struct md_item *it, int n, int s, int rh,
                         void (*status)(int, char *, int, int *), int value_row)
{
    const int lx = 24, lw = 332, y0 = 52, ph = rh - 6;
    const int box = ph - 12;
    int now = get_ms_clock();
    char buf[96];
    /* lo stato chiede valori a Magic Lantern e fa conti astronomici: non a ogni fotogramma */
    static char st_txt[8][72];
    static unsigned char st_on[8];
    if (now - md_stat_t > 300)
    {
        md_stat_t = now;
        for (int i = 0; i < n && i < 8; i++) { int on; status(i, st_txt[i], sizeof(st_txt[i]), &on); st_on[i] = on; }
    }
    for (int i = 0; i < n; i++) fill_r(C_PANEL, lx, y0 + i * rh, lw, ph, R_ROW);
    hl_bar(lx, lw, y0 + s * rh, ph);
    for (int i = 0; i < n; i++)
    {
        int y = y0 + i * rh, mine = i == s;
        int bx = lx + 14, by = y + 6;
        fill_r(C_BG, bx, by, box, box, R_CHIP + 1);
        ring_r(mine ? C_ACCENT : C_LINE, bx, by, box, box, R_CHIP + 1, 1);
        int si = ics_index(it[i].icon);
        if (si >= 0 && box >= ICS_N + 2) draw_sprite(bx + (box - ICS_N) / 2, by + (box - ICS_N) / 2, si);
        else it[i].icon(bx + box / 2, by + box / 2, box - 6, -1);
        int tx = bx + box + 14;
        txt(FONT_MED, mine ? C_TEXT : C_TEXT, tx, y + (ph - 23) / 2, rows[i].title);
        int on = st_on[MIN(i, 7)];
        if (i == value_row)
        {
            /* l'etichetta si cambia qui con </> */
            snprintf(buf, sizeof(buf), "%s", tag_names[COERCE(P.astro_tag, 0, NUM_TAGS - 1)]);
            int vw = txt_w(FONT_MED, buf), xr = lx + lw - 22, mid = y + ph / 2;
            arrow(xr, mid, 1, mine ? C_ACCENT : C_LINE);
            txt(FONT_MED, P.astro_tag ? C_ACCENT : C_DIM, xr - 12 - vw, y + (ph - 23) / 2, buf);
            arrow(xr - 24 - vw, mid, -1, mine ? C_ACCENT : C_LINE);
        }
        else if (on) disc(lx + lw - 20, y + ph / 2, 4, C_OK);
    }

    /* pannello della voce scelta */
    const int px = 368, pw = SCR_W - 24 - px;
    info_card(px, y0, pw, 384, 0);
    int t = now - md_t0;
    if (t > 1300 || opt_anim == 2) t = -1;
    else { anim_keep(); if (opt_anim == 1) t = t * 10 / 6; }
    int ex = px + pw / 2, ey = y0 + 72, es = 104;
    fill_r(C_BG, ex - es / 2 - 6, ey - es / 2 - 6, es + 12, es + 12, R_CARD);
    ring_r(C_LINE, ex - es / 2 - 6, ey - es / 2 - 6, es + 12, es + 12, R_CARD, 1);
    it[s].icon(ex, ey, es, t < 0 ? -1 : t);
    int e0 = phase(now - md_txt_t0, 0, anim_ms(240), EZ_OUT), e1 = phase(now - md_txt_t0, anim_ms(60), anim_ms(240), EZ_OUT);
    int e2 = phase(now - md_txt_t0, anim_ms(120), anim_ms(240), EZ_OUT);
    if (e2 < 1000) anim_keep();
    txt_in(FONT_MED_LARGE, C_TEXT, px + 18, y0 + 140, rows[s].title, e0, 0, 8);
    if (e1 >= 1000) txt_wrap(FONT_MED, C_DIM, px + 18, y0 + 174, pw - 34, it[s].desc, 24);
    else if (e1 > 0)
    {
        /* il testo lungo entra tutto insieme: una riga alla volta sarebbe troppo */
        int dy = (1000 - e1) * 8 / 1000;
        txt_wrap(FONT_MED, fade_c(C_DIM, e1), px + 18, y0 + 174 + dy, pw - 34, it[s].desc, 24);
    }
    int on = st_on[MIN(s, 7)];
    snprintf(buf, sizeof(buf), "%s", st_txt[MIN(s, 7)]);
    fill(C_LINE, px + 18, y0 + 324, pw - 36, 1);
    txt_in(FONT_SMALL, C_DIM, px + 18, y0 + 332, "ADESSO", e2, 0, 6);
    if (on && e2 >= 1000) disc(px + 24, y0 + 361, 4, C_OK);
    if (e2 >= 1000) txt_fit(on ? C_OK : C_TEXT, px + (on ? 36 : 18), y0 + 350, pw - (on ? 52 : 34), buf);
    else if (txt_w(FONT_MED, buf) <= pw - (on ? 52 : 34)) txt_in(FONT_MED, on ? C_OK : C_TEXT, px + (on ? 36 : 18), y0 + 350, buf, e2, 0, 6);
}

static void draw_astro()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Astro");
    draw_menu_md(astro_rows, astro_md, NUM_ASTRO_ROWS, sel[SCR_ASTRO], 49, astro_status, ASTRO_TAG_ROW);
    draw_footer(sel[SCR_ASTRO] == ASTRO_TAG_ROW ? "</> cambia etichetta   MENU indietro" : "^/v scegli   SET apri   INFO aiuto   MENU indietro");
}

static void astro_key(int key, int dx, int dy)
{
    int s = sel[SCR_ASTRO];
    if (dy) { sel[SCR_ASTRO] = COERCE(s + dy, 0, NUM_ASTRO_ROWS - 1); if (sel[SCR_ASTRO] != s) md_select(); }
    else if ((dx || key == MODULE_KEY_PRESS_SET) && s == ASTRO_TAG_ROW)
    {
        P.astro_tag = MOD(P.astro_tag + (dx ? dx : 1), NUM_TAGS);
        persist_touch();
        prefix_pending = 1;
        md_t0 = get_ms_clock();
        md_stat_t = -100000;
    }
    else if (key == MODULE_KEY_PRESS_SET)
    {
        if (astro_rows[s].target == SCR_MWPLAN) { mw_back_sky = 0; mw_night = 0; }
        go(astro_rows[s].target, 1);
    }
}

static void draw_foto()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Foto");
    draw_menu_md(foto_rows, foto_md, NUM_FOTO_ROWS, sel[SCR_FOTO], 55, foto_status, -1);
    draw_footer("^/v scegli   SET apri   MENU indietro");
}

static void foto_key(int key, int dx, int dy)
{
    int s = sel[SCR_FOTO];
    if (dy) { sel[SCR_FOTO] = COERCE(s + dy, 0, NUM_FOTO_ROWS - 1); if (sel[SCR_FOTO] != s) md_select(); }
    else if (key == MODULE_KEY_PRESS_SET) go(foto_rows[s].target, 1);
}

/* ------------------------------------------------------------------ scaffale */

static int shelf_cat = CAT_COLOR;
static int shelf_pos[CAT_COUNT] = { 1, 0, 0, 0 };
static struct glider shelf_g = { -1, 0, 0, 0 }, tab_g = { -1, 0, 0, 0 };
static int pop_t0 = -100000;                                /* cambio categoria: i rullini spuntano */
static int bump_t0 = -100000, bump_dir = 0;                 /* fine dello scaffale: piccolo urto */
static int info_t0 = -100000, info_dir = 0;                 /* testi della pellicola che entrano */
static int cat_films[NUM_FILMS];

static int cat_list(int cat)
{
    int n = 0;
    for (int i = 0; i < NUM_FILMS; i++) if (films[i].cat == cat) cat_films[n++] = i;
    return n;
}

static int shelf_film()
{
    int n = cat_list(shelf_cat);
    shelf_pos[shelf_cat] = COERCE(shelf_pos[shelf_cat], 0, MAX(n - 1, 0));
    return cat_films[shelf_pos[shelf_cat]];
}

static void draw_shelf()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Rullini");
    int now = get_ms_clock();
    for (int c = 0; c < CAT_COUNT; c++)
        txt_c(FONT_MED, c == shelf_cat ? C_TEXT : C_DIM, 120 + c * 160, 52, cat_names[c]);
    int tv = glide(&tab_g, shelf_cat * 1000, 220);
    fill(C_ACCENT, 120 + tv * 160 / 1000 - 34, 80, 68, 3);

    int n = cat_list(shelf_cat);
    int pos = COERCE(shelf_pos[shelf_cat], 0, MAX(n - 1, 0));
    int vis = glide(&shelf_g, pos * 1000, 260);

    /* urto alla fine dello scaffale: oscillazione smorzata */
    int bump = 0, tb = now - bump_t0, db = anim_ms(300);
    if (tb >= 0 && tb < db)
    {
        bump = (int)((float)(bump_dir * 12) * sinf((float)tb * 6.2832f / (float)db) * (float)(db - tb) / (float)db);
        anim_keep();
    }

    /* posizione continua: ogni rullino scala e si sposta in base alla distanza dal centro */
    int order[NUM_FILMS], cnt = 0;
    for (int far = 3; far >= 0; far--)
        for (int j = 0; j < n; j++)
        {
            int d = ABS(j * 1000 - vis);
            if (d >= far * 1000 && d < (far + 1) * 1000) order[cnt++] = j;
        }
    /* mensola: tutti i rullini ci appoggiano sopra, piu' piccoli man mano che si allontanano */
    const int shelf_y = 254;
    fill(cc(COLOR_GRAY(16)), 40, shelf_y - 4, SCR_W - 80, 8);         /* piano in prospettiva */
    fill(cc(COLOR_GRAY(36)), 40, shelf_y + 4, SCR_W - 80, 2);         /* bordo davanti */
    fill(cc(COLOR_GRAY(8)), 40, shelf_y + 6, SCR_W - 80, 3);
    for (int k = 0; k < cnt; k++)
    {
        int j = order[k];
        int d = j * 1000 - vis;
        int ad = ABS(d);
        if (ad > 2600) continue;
        /* cambio categoria: spuntano dal piano, prima quello al centro */
        int ep = phase(now - pop_t0, anim_ms(ABS(j - pos) * 55), anim_ms(340), EZ_BACK);
        if (ep < 1000) anim_keep();
        if (ep <= 0) continue;
        int scale = 100 - MIN(ad, 2000) * 25 / 1000;          /* 100 al centro, 50 a due posti */
        scale = scale * lerp(25, 100, ep) / 100;
        int cx = 360 + d * 140 / 1000 - (d > 0 ? 1 : -1) * MAX(ad - 1000, 0) * 20 / 1000 + bump;
        int lift = MAX(0, 700 - ad) * 8 / 700;               /* quello scelto si solleva un poco */
        int top = shelf_y - canister_height(scale) - lift;
        int leader = ep >= 1000 ? MAX(0, 600 - ad) * 44 / 600 : 0;   /* la linguetta esce piano */
        int sw = 76 * scale / 100 + 10 - lift;                /* ombra sul piano */
        fill(cc(COLOR_GRAY(6)), cx - sw / 2 + 3, shelf_y - 3, sw, 6);
        draw_canister(&films[cat_films[j]], cx, top, scale, leader);
    }

    /* testi: entrano dal lato da cui arriva il rullino, uno dopo l'altro */
    const struct film *fm = &films[cat_films[pos]];
    int ddx = info_dir * 28, ddy = info_dir ? 0 : 10;
    int e0 = phase(now - info_t0, 0, anim_ms(240), EZ_OUT), e1 = phase(now - info_t0, anim_ms(50), anim_ms(240), EZ_OUT);
    int e2 = phase(now - info_t0, anim_ms(100), anim_ms(240), EZ_OUT), e3 = phase(now - info_t0, anim_ms(150), anim_ms(240), EZ_OUT);
    if (e3 < 1000) anim_keep();
    int bw = txt_w(FONT_MED, fm->brand), nw = txt_w(FONT_LARGE, fm->name);
    int gx = 360 - (bw + 12 + nw) / 2;
    txt_in(FONT_MED, C_DIM, gx, 278, fm->brand, e0, ddx, ddy);
    txt_in(FONT_LARGE, C_TEXT, gx + bw + 12, 270, fm->name, e0, ddx, ddy);
    txt_c_in(FONT_MED, C_TEXT, 360, 314, fm->desc, e1, ddx, ddy);
    txt_c_in(FONT_MED, C_DIM, 360, 340, fm->use, e2, ddx, ddy);
    char buf[96];
    snprintf(buf, sizeof(buf), "ISO %d   %s   stile %s   %dK", fm->iso, fm->process, style_name(fm->style), fm->kelvin);
    txt_c_in(FONT_MED, C_ACCENT, 360, 376, buf, e3, ddx, ddy);
    int raw_warn = P.roll_loaded && !P.roll_paused && picq_raw_only();
    if (P.roll_loaded)
        snprintf(buf, sizeof(buf), "Caricato: R%03d %s, %d/%d%s", P.roll_num % 1000, cur_film()->name, P.roll_shots, P.roll_exp,
            P.roll_paused ? " - in pausa" : raw_warn ? " - solo RAW" : "");
    else snprintf(buf, sizeof(buf), "Nessun rullino caricato");
    txt_c(FONT_MED, raw_warn ? C_WARN : C_DIM, 360, 410, buf);
    draw_footer(P.roll_loaded ? "</> pellicola  ^/v tipo  SET carica  Q rullino attuale" : "</> pellicola  ^/v tipo  SET carica");
}

/* dialogo sopra lo scaffale: caricamento o rullino attuale */
enum { CONF_LOAD, CONF_ROLL };
static int conf_mode = CONF_LOAD;
static int load_exp = 36, load_push = 0;

static void shelf_key(int key, int dx, int dy)
{
    int n = cat_list(shelf_cat);
    int now = get_ms_clock();
    if (dx)
    {
        int np = COERCE(shelf_pos[shelf_cat] + dx, 0, MAX(n - 1, 0));
        if (np == shelf_pos[shelf_cat]) { bump_t0 = now; bump_dir = -dx; }
        else { shelf_pos[shelf_cat] = np; info_t0 = now; info_dir = dx; }
    }
    else if (dy) { shelf_cat = MOD(shelf_cat + dy, CAT_COUNT); shelf_g.vis = -1; pop_t0 = now; info_t0 = now; info_dir = 0; }
    else if (key == MODULE_KEY_PRESS_SET) { conf_mode = CONF_LOAD; sel[SCR_CONFIRM] = 2; go(SCR_CONFIRM, 2); }
    else if (key == MODULE_KEY_Q && P.roll_loaded) { conf_mode = CONF_ROLL; sel[SCR_CONFIRM] = 1; go(SCR_CONFIRM, 2); }
}

static void shelf_enter()
{
    if (P.roll_loaded)
    {
        shelf_cat = cur_film()->cat;
        int n = cat_list(shelf_cat);
        for (int i = 0; i < n; i++) if (cat_films[i] == P.roll_film) shelf_pos[shelf_cat] = i;
    }
    shelf_g.vis = -1;
    tab_g.vis = -1;
}

static const char *const load_labels[] = { "Pose", "Push / pull", "Carica il rullino" };
static const char *const roll_labels[] = { "Stato", "", "Riavvolgi" };

static void conf_value(int row, char *buf, int len, int *kind)
{
    if (conf_mode == CONF_LOAD)
    {
        const struct film *fm = &films[shelf_film()];
        if (row == 0) snprintf(buf, len, "< %d >", load_exp);
        else if (row == 1)
        {
            if (load_push) snprintf(buf, len, "< %s%d stop, ISO %d >", load_push > 0 ? "+" : "", load_push, film_iso_exposed(fm, load_push));
            else snprintf(buf, len, "< normale, ISO %d >", film_iso_exposed(fm, 0));
        }
        else { *kind = VK_ACTION; snprintf(buf, len, P.roll_loaded ? "riavvolge l'attuale  SET >" : "SET >"); }
    }
    else
    {
        if (row == 0) { snprintf(buf, len, P.roll_paused ? "in pausa (impostazioni 6D)" : "attivo"); *kind = VK_INFO; }
        else *kind = VK_ACTION;
    }
}

static void conf_change(int row, int dir, int big)
{
    if (conf_mode != CONF_LOAD) return;
    if (row == 0) load_exp = load_exp == 36 ? (dir > 0 ? 12 : 24) : load_exp == 24 ? (dir > 0 ? 36 : 12) : (dir > 0 ? 24 : 36);
    else if (row == 1) load_push = COERCE(load_push + dir, -2, 3);
}

static void conf_activate(int row)
{
    if (conf_mode == CONF_LOAD)
    {
        if (row != 2) { conf_change(row, 1, 0); return; }
        int fi = shelf_film();
        if (P.roll_loaded)
        {
            int shots = P.roll_shots;
            anim_rewind(cur_film(), shots);
            roll_rewind();
        }
        roll_load(fi, load_exp, load_push);
        anim_load(&films[fi]);
        go(SCR_SHELF, -2);          /* la scena riscende e scopre lo scaffale */
        if (picq_raw_only()) say(C_WARN, "Caricato. " RAW_HINT);
        else say(C_OK, "Rullino caricato");
        return;
    }
    if (row == 1)
    {
        if (P.roll_paused)
        {
            roll_resume();
            if (picq_raw_only()) say(C_WARN, "Ripreso. " RAW_HINT);
            else say(C_OK, "Rullino ripreso: carattere della pellicola attivo");
        }
        else { roll_pause(); say(C_OK, "In pausa: impostazioni normali della 6D"); }
        go(SCR_SHELF, -1);
    }
    else if (row == 2)
    {
        int shots = P.roll_shots;
        const struct film *fm = cur_film();
        anim_rewind(fm, shots);
        roll_rewind();
        go(SCR_SHELF, -2);          /* la scena riscende e scopre lo scaffale */
        say(C_OK, "Rullino riavvolto, impostazioni ripristinate");
    }
}

static void draw_confirm()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Rullini");
    char buf[96];
    struct form f;
    memset(&f, 0, sizeof(f));
    f.n = 3; f.value = conf_value; f.y0 = 200; f.row_h = 46;
    static const char *labels[3];
    if (conf_mode == CONF_LOAD)
    {
        const struct film *fm = &films[shelf_film()];
        draw_canister(fm, 110, 60, 80, 0);
        snprintf(buf, sizeof(buf), "%s %s", fm->brand, fm->name);
        txt(FONT_LARGE, C_TEXT, 190, 68, buf);
        txt(FONT_MED, C_TEXT, 190, 110, fm->desc);
        txt(FONT_MED, C_DIM, 190, 136, fm->use);
        if (P.roll_loaded)
        {
            snprintf(buf, sizeof(buf), "Il rullino %03d (%d/%d) verra' riavvolto prima", P.roll_num % 1000, P.roll_shots, P.roll_exp);
            txt(FONT_MED, C_WARN, 190, 166, buf);
        }
        else if (picq_raw_only()) txt(FONT_MED, C_WARN, 190, 166, "Solo RAW: il look va solo nei JPEG");
        for (int i = 0; i < 3; i++) labels[i] = load_labels[i];
    }
    else
    {
        const struct film *fm = cur_film();
        draw_canister(fm, 110, 60, 80, 26);
        snprintf(buf, sizeof(buf), "Rullino %03d  %s", P.roll_num % 1000, fm->name);
        txt(FONT_LARGE, C_TEXT, 190, 68, buf);
        snprintf(buf, sizeof(buf), "%d pose su %d%s", P.roll_shots, P.roll_exp,
            P.roll_push ? (P.roll_push > 0 ? ", tirata" : ", tirata indietro") : "");
        txt(FONT_MED, C_TEXT, 190, 110, buf);
        txt(FONT_MED, C_DIM, 190, 136, fm->desc);
        if (P.roll_shots < P.roll_exp)
        {
            snprintf(buf, sizeof(buf), "Riavvolgendo, le %d pose rimaste vanno perse", P.roll_exp - P.roll_shots);
            txt(FONT_MED, C_DIM, 190, 166, buf);
        }
        labels[0] = roll_labels[0];
        labels[1] = P.roll_paused ? "Riprendi il rullino" : "Pausa: torna alla 6D normale";
        labels[2] = roll_labels[2];
    }
    f.labels = labels;
    draw_form(&f, sel[SCR_CONFIRM]);
    draw_footer(conf_mode == CONF_LOAD ? "^/v riga   </> cambia   SET carica   MENU annulla" : "^/v riga   SET conferma   MENU annulla");
}

static void confirm_key(int key, int dx, int dy)
{
    int s = sel[SCR_CONFIRM];
    if (dy) sel[SCR_CONFIRM] = COERCE(s + dy, conf_mode == CONF_ROLL ? 1 : 0, 2);
    else if (dx) conf_change(s, dx, 0);
    else if (key == MODULE_KEY_PRESS_SET) conf_activate(s);
}

/* ------------------------------------------------------------------ profili */

static void prof_summary(const struct profile *p, char *buf, int len)
{
    char ap[8];
    int av = p->aperture ? RAW2VALUE(aperture, p->aperture) : 0;
    fmt_tenths(ap, sizeof(ap), av);
    snprintf(buf, len, "%s %s f/%s ISO %s %s", get_shootmode_name_short(p->mode), lens_format_shutter(p->shutter),
        ap, p->iso ? lens_format_iso(p->iso) : "Auto", style_name(p->style));
}

static void draw_profiles()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Profili");
    int s = sel[SCR_PROFILES];
    const int y0 = 56, rh = 76, ph = rh - 6;
    for (int i = 0; i < NPROF; i++) fill_r(C_PANEL, 24, y0 + i * rh, SCR_W - 48, ph, R_ROW);
    hl_bar(24, SCR_W - 48, y0 + s * rh, ph);
    for (int i = 0; i < NPROF; i++)
    {
        struct profile *p = &P.prof[i];
        int y = y0 + i * rh, mine = i == s;
        char buf[48];
        /* tondo con il modo di scatto (o il numero se il posto e' vuoto) */
        int cx = 70, cy = y + ph / 2;
        disc(cx, cy, 25, p->valid ? (mine ? C_ACCENT : C_LINE) : C_PANEL2);
        disc(cx, cy, 22, C_BG);
        if (p->valid) txt_c(FONT_MED_LARGE, mine ? C_ACCENT : C_TEXT, cx, cy - 15, get_shootmode_name_short(p->mode));
        snprintf(buf, sizeof(buf), "%d", i + 1);
        if (!p->valid) txt_c(FONT_MED, C_DIM, cx, cy - 12, buf);
        else txt(FONT_SMALL, C_DIM, 112, y + 15, buf);
        txt(FONT_MED_LARGE, p->valid ? C_TEXT : C_DIM, p->valid ? 128 : 112, y + 8, prof_names[COERCE(p->name, 0, NPROF_NAMES - 1)]);
        int x = 112, cyy = y + 42;
        int bg = mine ? C_BG : C_PANEL2;
        if (p->valid)
        {
            char ap[8];
            fmt_tenths(ap, sizeof(ap), p->aperture ? RAW2VALUE(aperture, p->aperture) : 0);
            x += chip(x, cyy, lens_format_shutter(p->shutter), bg, C_TEXT) + 6;
            snprintf(buf, sizeof(buf), "f/%s", ap);
            x += chip(x, cyy, buf, bg, C_TEXT) + 6;
            snprintf(buf, sizeof(buf), "ISO %s", p->iso ? lens_format_iso(p->iso) : "Auto");
            x += chip(x, cyy, buf, bg, C_TEXT) + 6;
            chip(x, cyy, style_name(p->style), bg, C_ACCENT);
        }
        else
        {
            /* posto libero: etichetta tratteggiata con il "+", in evidenza quando e' scelto */
            const char *e = "INFO salva qui le impostazioni di adesso";
            int ew = txt_w(FONT_SMALL, e) + 36, ec = mine ? C_ACCENT : C_LINE;
            for (int k = 0; k < ew; k += 6) { fill(ec, x + k, cyy, MIN(3, ew - k), 1); fill(ec, x + k, cyy + 19, MIN(3, ew - k), 1); }
            for (int k = 0; k < 20; k += 6) { fill(ec, x, cyy + k, 1, MIN(3, 20 - k)); fill(ec, x + ew - 1, cyy + k, 1, MIN(3, 20 - k)); }
            fill(ec, x + 9, cyy + 9, 9, 2);
            fill(ec, x + 12, cyy + 6, 2, 8);
            fill(ec, x + 13, cyy + 6, 1, 8);
            txt(FONT_SMALL, mine ? C_TEXT : C_DIM, x + 26, cyy + 3, e);
        }
    }
    draw_footer("SET carica   INFO salva   Q cancella   </> nome   MENU");
}

static void profiles_key(int key, int dx, int dy)
{
    int s = sel[SCR_PROFILES];
    struct profile *p = &P.prof[s];
    if (dy) sel[SCR_PROFILES] = COERCE(s + dy, 0, NPROF - 1);
    else if (dx) { p->name = MOD(p->name + dx, NPROF_NAMES); persist_touch(); }
    else if (key == MODULE_KEY_INFO)
    {
        profile_save(s);
        char buf[64];
        snprintf(buf, sizeof(buf), "Salvato in %s", prof_names[COERCE(p->name, 0, NPROF_NAMES - 1)]);
        say(C_OK, buf);
    }
    else if (key == MODULE_KEY_Q) { if (p->valid) { p->valid = 0; persist_touch(); say(C_OK, "Profilo cancellato"); } }
    else if (key == MODULE_KEY_PRESS_SET)
    {
        if (!p->valid) { say(C_DIM, "Profilo vuoto: INFO per salvarlo"); return; }
        int full = profile_load(s);
        if (full && prof_ap_limited)
        {
            char buf[80], ap[8];
            fmt_tenths(ap, sizeof(ap), RAW2VALUE(aperture, lens_info.raw_aperture));
            snprintf(buf, sizeof(buf), "Caricato con f/%s: %s dell'obiettivo", ap,
                prof_ap_limited == 1 ? "il piu' aperto" : "il piu' chiuso");
            say(C_OK, buf);
        }
        else say(full ? C_OK : C_ACCENT, full ? "Profilo caricato" : "Caricato in parte: tempo e diaframma vogliono la stessa ghiera");
    }
}

/* ------------------------------------------------------------------ opzioni */

static const char *const opt_labels[] = { "Tema", "Animazioni", "Schermata iniziale", "Apri all'accensione",
    "Cestino apre Iride", "Tasto luce LCD", "Anteprima col rullino", "Avviso posa", "Luogo", "Latitudine", "Longitudine",
    "Fuso orario", "Cielo del luogo", "Diagnostica tasti", "Prova della 6D" };

/* buio del cielo di casa (mag/arcsec^2 come un SQM): serve a stimare le stelle visibili con la Luna */
static const short sqm_steps[] = { 220, 215, 210, 205, 200, 195, 190, 185 };
static const char *const sqm_names[] = { "molto buio", "buio", "campagna", "campagna vicino citta'",
    "periferia", "periferia luminosa", "citta'", "centro citta'" };
static int sqm_index()
{
    int best = 0;
    for (int i = 0; i < (int)COUNT(sqm_steps); i++)
        if (ABS(sqm_steps[i] - opt_sqm10) < ABS(sqm_steps[best] - opt_sqm10)) best = i;
    return best;
}

static void opt_value(int row, char *buf, int len, int *kind)
{
    static const char *const theme[] = { "normale", "notte (rosso)", "notte in Astro" };
    static const char *const anim[] = { "complete", "rapide", "spente" };
    switch (row)
    {
        case 0: snprintf(buf, len, "< %s >", theme[COERCE(opt_theme, 0, 2)]); break;
        case 1: snprintf(buf, len, "< %s >", anim[COERCE(opt_anim, 0, 2)]); break;
        case 2: *kind = opt_splash ? VK_ON : VK_OFF; break;
        case 3: *kind = opt_boot_open ? VK_ON : VK_OFF; break;
        case 4: *kind = opt_trash_open ? VK_ON : VK_OFF; break;
        case 5: *kind = opt_light_key ? VK_ON : VK_OFF; break;
        case 6: snprintf(buf, len, "< %s >", opt_review_off ? "spegni" : "lascia com'e'"); break;
        case 7: *kind = opt_wind ? VK_ON : VK_OFF; break;
        case 8:
        {
            int ci = city_find();
            if (ci >= 0) snprintf(buf, len, "< %s >", cities[ci].name);
            else snprintf(buf, len, "< vicino a %s >", cities[city_nearest()].name);
            break;
        }
        case 9: snprintf(buf, len, "< %d.%02d %c >", ABS(P.lat100) / 100, ABS(P.lat100) % 100, P.lat100 < 0 ? 'S' : 'N'); break;
        case 10: snprintf(buf, len, "< %d.%02d %c >", ABS(P.lon100) / 100, ABS(P.lon100) % 100, P.lon100 < 0 ? 'O' : 'E'); break;
        case 11:
        {
            char t[48];
            tz_name(tz_valid(P.tz_mode) ? P.tz_mode : 0, t, sizeof(t));
            snprintf(buf, len, "< %s >", t);
            break;
        }
        case 12:
        {
            int i = sqm_index();
            snprintf(buf, len, "< %d,%d %s >", sqm_steps[i] / 10, sqm_steps[i] % 10, sqm_names[i]);
            break;
        }
        case 13: case 14: *kind = VK_ACTION; snprintf(buf, len, ">"); break;
    }
}

static void opt_change(int row, int dir, int big)
{
    switch (row)
    {
        case 0: opt_theme = MOD(opt_theme + dir, 3); break;
        case 1: opt_anim = MOD(opt_anim + dir, 3); break;
        case 2: opt_splash = !opt_splash; break;
        case 3: opt_boot_open = !opt_boot_open; break;
        case 4: opt_trash_open = !opt_trash_open; break;
        case 5: opt_light_key = !opt_light_key; break;
        case 6: opt_review_off = !opt_review_off; break;
        case 7: opt_wind = !opt_wind; break;
        case 8:
        {
            /* citta' successiva; da un luogo personalizzato si parte dalla piu' vicina */
            int ci = city_find();
            city_set(ci >= 0 ? ci + dir * (big ? 5 : 1) : city_nearest());
            break;
        }
        case 9: P.lat100 = COERCE(P.lat100 + dir * (big ? 100 : 1), -9000, 9000); persist_touch(); break;
        case 10: P.lon100 = COERCE(P.lon100 + dir * (big ? 100 : 1), -18000, 18000); persist_touch(); break;
        case 11: P.tz_mode = tz_next(tz_valid(P.tz_mode) ? P.tz_mode : 0, dir * (big ? 4 : 1)); persist_touch(); break;
        case 12: opt_sqm10 = sqm_steps[COERCE(sqm_index() - dir, 0, (int)COUNT(sqm_steps) - 1)]; break;   /* destra = piu' buio */
    }
}

static void opt_activate(int row)
{
    if (row == 13) { go(SCR_KEYS, 1); return; }
    if (row == 14) { go(SCR_BENCH, 1); return; }
    opt_change(row, 1, 0);
}

static const char *const opt_heads[] = { "ASPETTO", 0, 0, "APERTURA DI IRIDE", 0, 0, "SCATTO", 0, "LUOGO E ORA", 0, 0, 0, 0, "STRUMENTI", 0 };
static const struct form opt_form = { 15, opt_labels, opt_value, opt_change, opt_activate, 0,
    "^/v riga   </> cambia (ghiera superiore x100)   MENU indietro", 54, 42, opt_heads, 0, 0 };

/* ------------------------------------------------------------------ diagnostica tasti */

static const char *key_name(int p)
{
    /* codici della 6D (platform/6D.116/gui.h) */
    switch (p)
    {
        case 0x20: return "LUCE LCD";
        case 0x0d: return "CESTINO";
        case 0x06: return "MENU";
        case 0x07: return "INFO";
        case 0x0b: return "PLAY";
        case 0x1d: return "Q";
        case 0x1e: return "LV";
        case 0x04: return "SET";
        case 0x05: return "SET rilascio";
        case 0x00: case 0x01: case 0x02: case 0x03: return "ghiera";
        case 0x12: return "zoom";
        case 0x13: return "zoom rilascio";
        case 0x50: return "mezzo scatto";
        case 0x36: return "croce rilascio";
        case 0x54: return "tasto non gestito";
    }
    if (p >= 0x2e && p <= 0x35) return "croce";
    if (p == 0x1c) return "croce";
    return "?";
}

static void klog_save()
{
    FIO_CreateDirectory("ML/LOGS");
    FILE *f = FIO_CreateFile("ML/LOGS/IRIDEKEY.TXT");
    if (!f) return;
    my_fprintf(f, "Iride 6D %s - ultimi tasti (nuovo in alto), ora %d ms, ML con schermo intero: %s\n",
        IRIDE_VERSION, get_ms_clock(), fullscreen ? "si" : "no");
    my_fprintf(f, "codice,arg,nome,gui_state,iride,menu_ml,play,aperto,ms\n");
    int n = MIN(klog_count, KLOG_N);
    for (int i = 0; i < n; i++)
    {
        struct klog_entry *k = &klog[(klog_count - 1 - i) % KLOG_N];
        my_fprintf(f, "0x%02x,0x%x,%s,%d,%d,%d,%d,%d,%d\n", k->param, k->arg, key_name(k->param),
            k->gs, k->app, k->menu, k->play, k->opened, k->ms);
    }
    FIO_CloseFile(f);
}

static int keys_seen = -1;
static void keys_enter() { keys_seen = -1; klog_save(); }
static int keys_tick()
{
    static int last_s = -1;
    int s = get_ms_clock() / 1000;
    if (klog_count != keys_seen || s != last_s) { keys_seen = klog_count; last_s = s; return 1; }
    return 0;
}

static void draw_keys()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Diagnostica tasti");
    txt(FONT_MED, C_DIM, 30, 54, "Chiudi Iride, premi il tasto luce 2-3 volte, poi riaprila qui.");
    txt(FONT_SMALL, C_DIM, 30, 84, "TASTO");
    txt(FONT_SMALL, C_DIM, 230, 84, "CODICE");
    txt(FONT_SMALL, C_DIM, 330, 84, "DOVE");
    txt_r(FONT_SMALL, C_DIM, SCR_W - 30, 84, "QUANDO");
    fill(C_LINE, 30, 100, SCR_W - 60, 1);
    int n = MIN(klog_count, 10);
    int now = get_ms_clock();
    char buf[96];
    if (!n) txt(FONT_MED, C_DIM, 30, 112, "Nessun tasto registrato finora.");
    for (int i = 0; i < n; i++)
    {
        struct klog_entry *k = &klog[(klog_count - 1 - i) % KLOG_N];
        int y = 108 + i * 28;
        int light = k->param == BGMT_LIGHT_6D;
        const char *where = k->app ? "dentro Iride" : k->menu ? "menu ML" : k->play ? "riproduzione" : "scatto";
        int ago = (now - k->ms) / 1000;
        txt(FONT_MED, light ? C_ACCENT : C_TEXT, 30, y, key_name(k->param));
        snprintf(buf, sizeof(buf), "0x%02x", k->param);
        txt(FONT_MED, C_DIM, 230, y, buf);
        snprintf(buf, sizeof(buf), "%s, stato %d%s%s", where, k->gs, k->opened ? ", aperta" : "", k->fake ? ", da ML" : "");
        txt(FONT_MED, C_TEXT, 330, y, buf);
        if (ago < 60) snprintf(buf, sizeof(buf), "%d s fa", ago);
        else snprintf(buf, sizeof(buf), "%d min fa", ago / 60);
        txt_r(FONT_MED, C_DIM, SCR_W - 30, y, buf);
    }
    txt(FONT_SMALL, C_DIM, 30, 396, "Salvato anche in ML/LOGS/IRIDEKEY.TXT (all'apertura di questa pagina)");
    txt(FONT_SMALL, C_DIM, 30, 414, "Iride 6D " IRIDE_VERSION " su Magic Lantern - Canon EOS 6D 1.1.6");
    draw_footer("MENU indietro");
}

/* ------------------------------------------------------------------ Torna alla 6D */

/* iconcine dell'elenco "Torna alla 6D" (quadrato di 26 px centrato in cx, cy) */
enum { RI_EXPO, RI_STYLE, RI_SLIDERS, RI_ML, RI_FILE, RI_ROLL };
static void restore_icon(int kind, int cx, int cy)
{
    switch (kind)
    {
        case RI_EXPO:       /* ghiera dei modi */
            disc(cx, cy, 11, C_DIM); disc(cx, cy, 8, C_BG);
            for (int i = 0; i < 8; i++) { float a = (float)i * 0.785f; fill(C_DIM, cx + (int)(9.5f * cosf(a)), cy + (int)(9.5f * sinf(a)), 2, 2); }
            fill(C_ACCENT, cx - 1, cy - 8, 3, 6);
            break;
        case RI_STYLE:      /* tavolozza */
            disc(cx, cy, 11, C_DIM); disc(cx + 5, cy + 5, 4, C_BG);
            disc(cx - 4, cy - 4, 2, tc(COLOR_RED)); disc(cx + 3, cy - 5, 2, tc(COLOR_YELLOW)); disc(cx - 5, cy + 3, 2, tc(COLOR_LIGHT_BLUE));
            break;
        case RI_SLIDERS:    /* cursori */
            for (int i = 0; i < 3; i++) { int x = cx - 8 + i * 8; fill(C_LINE, x, cy - 10, 2, 20); fill(C_ACCENT, x - 3, cy - 4 + (i == 1 ? -5 : i * 4), 8, 4); }
            break;
        case RI_ML:         /* Magic Lantern */
            frame_rect(C_DIM, cx - 12, cy - 9, 24, 18, 1);
            txt_c(FONT_SMALL, C_TEXT, cx, cy - 6, "ML");
            break;
        case RI_FILE:       /* nome del file */
            fill(C_DIM, cx - 8, cy - 11, 12, 22); fill(C_DIM, cx - 8, cy - 7, 16, 18);
            fill_tri(cx + 4, cy - 11, cx + 4, cy - 7, cx + 8, cy - 7, C_LINE);
            fill(C_BG, cx - 5, cy - 2, 10, 2); fill(C_BG, cx - 5, cy + 3, 8, 2);
            break;
        default:            /* rullino */
            fill(C_DIM, cx - 3, cy - 12, 6, 3);
            fill(C_ACCENT, cx - 7, cy - 9, 14, 20);
            fill(C_BG, cx - 7, cy - 3, 14, 6);
            fill(C_LINE, cx + 7, cy - 4, 6, 8);
            break;
    }
}

static void draw_restore()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Torna alla 6D");
    int n = restore_pending_count();
    if (!n)
    {
        draw_check(360, 170, 40, 1000);
        txt_c(FONT_LARGE, C_TEXT, 360, 230, "Tutto e' gia' originale");
        txt_c(FONT_MED, C_DIM, 360, 276, "Iride non ha cambiato nessuna impostazione della 6D.");
        draw_footer("MENU indietro");
        return;
    }
    char buf[96];
    /* riepilogo */
    info_card(24, 54, SCR_W - 48, 62, 0);
    snprintf(buf, sizeof(buf), "%d", n);
    txt(FONT_LARGE, C_ACCENT, 44, 68, buf);
    int nx = 44 + txt_w(FONT_LARGE, buf) + 14;
    txt(FONT_MED, C_TEXT, nx, 64, n == 1 ? "modifica fatta da Iride" : "modifiche fatte da Iride");
    txt(FONT_SMALL, C_DIM, nx, 92, "Rimetto tutto com'era prima, in un colpo solo:");
    /* elenco */
    int y = 126;
    const int rh = 38;
    #define RITEM(icon, text) do { if (y < 366) { fill_r(C_PANEL, 24, y, SCR_W - 48, rh - 6, R_ROW); \
        fill_r(C_BG, 32, y + 3, 26, 26, R_CHIP); restore_icon(icon, 45, y + 16); txt(FONT_MED, C_TEXT, 72, y + 4, text); y += rh; } } while (0)
    if (P.snap_valid)
    {
        snprintf(buf, sizeof(buf), "ISO, tempo, diaframma, bianco e anteprima (ghiera %s)", get_shootmode_name_short(P.snap_mode));
        RITEM(RI_EXPO, buf);
        snprintf(buf, sizeof(buf), "Stile immagine %s", style_name(P.snap_style));
        RITEM(RI_STYLE, buf);
    }
    for (int i = 0; i < NSTYLE; i++)
        if (P.ps_touched & (1 << i))
        {
            snprintf(buf, sizeof(buf), "Cursori dello stile %s", style_name(style_id(i)));
            RITEM(RI_SLIDERS, buf);
        }
    int nml = 0;
    for (int f = 0; f < MLF_COUNT; f++) if (P.ml_touched & (1 << f)) nml++;
    if (nml)
    {
        snprintf(buf, sizeof(buf), "%d impostazion%s di Magic Lantern (HDR, overlay...)", nml, nml == 1 ? "e" : "i");
        RITEM(RI_ML, buf);
    }
    if (P.astro_tag) RITEM(RI_FILE, "Nomi dei file: di nuovo IMG_");
    if (roll_active()) RITEM(RI_ROLL, "Rullino in pausa (non si perde)");
    #undef RITEM
    /* pulsante */
    fill_r(C_ACCENT, 24, 384, SCR_W - 48, 48, R_CARD);
    txt_c(FONT_MED_LARGE, C_BG, 360, 394, "SET   ripristina tutto");
    draw_footer("SET conferma   MENU annulla");
}

static void restore_frame(int p, int ok_t, const char *text)
{
    frame_begin();
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Torna alla 6D");
    draw_check(360, 168, 32, ok_t);
    txt_c(FONT_MED, ok_t > 0 ? C_TEXT : C_DIM, 360, 222, text);
    draw_progress(160, 262, 400, 10, p);
    draw_footer("");
    frame_show(0);
}

static void restore_key(int key, int dx, int dy)
{
    if (key != MODULE_KEY_PRESS_SET || !restore_pending_count()) return;
    /* la barra corre mentre Canon applica le proprieta', poi compare la spunta */
    int dur = anim_ms(260), t0 = get_ms_clock();
    for (int t = 0; ; t = get_ms_clock() - t0)
    {
        restore_frame(600 * ease(t, dur) / 1000, 0, "Ripristino in corso...");
        if (t >= dur || key_pending()) break;
        msleep(10);
    }
    restore_all();
    persist_save();
    dur = anim_ms(800); t0 = get_ms_clock();
    for (int t = 0; ; t = get_ms_clock() - t0)
    {
        int tv = dur ? t * 800 / dur : 800;
        restore_frame(600 + 400 * ease(tv, 200) / 1000, tv, "Fatto: la 6D e' com'era prima di Iride");
        if (t >= dur || anim_skip()) break;
        msleep(10);
    }
    go(SCR_HOME, -1);
    say(C_OK, "Fatto: la 6D e' com'era prima di Iride");
}

/* ------------------------------------------------------------------ tabella delle schermate */

struct screen_def
{
    const char *title;
    int parent;
    const struct form *form;
    void (*draw)(void);
    void (*key)(int key, int dx, int dy);
    void (*enter)(void);
    int (*tick)(void);
    int tick_ms;
};

static const struct screen_def screens[SCR_COUNT] = {
    [SCR_HOME]     = { "6D",                  SCR_HOME,  0,            draw_home,     home_key,     home_enter,  0, 0 },
    [SCR_ASTRO]    = { "Astro",               SCR_HOME,  0,            draw_astro,    astro_key,    md_enter,    0, 0 },
    [SCR_FOTO]     = { "Foto",                SCR_HOME,  0,            draw_foto,     foto_key,     md_enter,    0, 0 },
    [SCR_NPF]      = { "Via Lattea",          SCR_ASTRO, &npf_form,    0,             0,            npf_enter,   0, 0 },
    [SCR_INTERVAL] = { "Intervallometro",     SCR_ASTRO, &int_form,    0,             0,            0,           0, 0 },
    [SCR_STARS]    = { "Fuoco stelle",        SCR_ASTRO, 0,            draw_stars,    stars_key,    stars_enter, stars_tick, 250 },
    [SCR_POLAR]    = { "Allineamento polare", SCR_ASTRO, 0,            draw_polar,    polar_key,    polar_enter, polar_tick, 1000 },
    [SCR_SKY]      = { "Calendario del cielo",SCR_ASTRO, 0,            draw_sky,      sky_key,      sky_enter,   sky_tick, 1 },
    [SCR_SHELF]    = { "Rullini",             SCR_HOME,  0,            draw_shelf,    shelf_key,    shelf_enter, 0, 0 },
    [SCR_CONFIRM]  = { "Rullini",             SCR_SHELF, 0,            draw_confirm,  confirm_key,  0,           0, 0 },
    [SCR_HDR]      = { "Bracketing HDR",      SCR_FOTO,  &hdr_form,    0,             0,            0,           0, 0 },
    [SCR_FSTACK]   = { "Focus stacking",      SCR_FOTO,  &fs_form,     0,             0,            0,           0, 0 },
    [SCR_DUAL]     = { "Dual ISO",            SCR_FOTO,  &dual_form,   0,             0,            0,           0, 0 },
    [SCR_ETTR]     = { "ETTR",                SCR_FOTO,  &ettr_form,   0,             0,            0,           0, 0 },
    [SCR_MOTION]   = { "Movimento",           SCR_FOTO,  &motion_form, 0,             0,            0,           0, 0 },
    [SCR_LVTOOLS]  = { "Strumenti Live View", SCR_FOTO,  &lv_form,     0,             0,            0,           0, 0 },
    [SCR_ND]       = { "Filtri ND",           SCR_FOTO,  &nd_form,     0,             0,            nd_enter,    0, 0 },
    [SCR_PROFILES] = { "Profili",             SCR_HOME,  0,            draw_profiles, profiles_key, 0,           0, 0 },
    [SCR_OPTIONS]  = { "Opzioni",             SCR_HOME,  &opt_form,    0,             0,            0,           0, 0 },
    [SCR_RESTORE]  = { "Torna alla 6D",       SCR_HOME,  0,            draw_restore,  restore_key,  0,           0, 0 },
    [SCR_KEYS]     = { "Diagnostica tasti",   SCR_OPTIONS, 0,          draw_keys,     0,            keys_enter,  keys_tick, 250 },
    [SCR_BENCH]    = { "Prova della 6D",      SCR_OPTIONS, 0,          draw_bench,    bench_key,    bench_enter, bench_tick, 1 },
    [SCR_MOON]     = { "Luna",                SCR_ASTRO, &moon_form,   0,             0,            moonshot_enter, moonshot_tick, 1000 },
    [SCR_MWPLAN]   = { "Dove puntare",        SCR_ASTRO, 0,            draw_mwplan,   mwplan_key,   mwplan_enter, mwplan_tick, 1 },
};

/* titolo della schermata madre per il percorso nell'intestazione (NULL se non serve) */
static const char *parent_title(const char *section)
{
    if (screen == SCR_HOME) return 0;
    int p = screens[screen].parent;
    if (p == SCR_HOME || p == screen) return 0;
    const char *t = screens[p].title;
    return streq(t, section) ? 0 : t;
}

/* ------------------------------------------------------------------ aiuto (tasto INFO) */

static const char *help_text(int scr)
{
    switch (scr)
    {
        case SCR_HOME: return
            "Iride riunisce le funzioni di Magic Lantern per la 6D, in italiano.\n"
            "ASTRO: strumenti per la notte. FOTO: HDR, focus stacking, ND e altro. RULLINI: pellicole simulate con le pose contate.\n"
            "Profili: impostazioni salvate. Opzioni: tema, luogo, fuso orario.\n"
            "Torna alla 6D: rimette tutto com'era prima di Iride.\n"
            "In ogni schermata INFO apre l'aiuto; un tasto qualsiasi lo chiude.";
        case SCR_ASTRO: return
            "Via Lattea: la posa piu' lunga senza stelle mosse (regola NPF).\n"
            "Intervallometro: sequenze di luci, dark, flat e bias, anche silenziose e col dithering.\n"
            "Fuoco sulle stelle: misura la stella e mette a fuoco, anche da solo.\n"
            "Allineamento polare: dove va la Polare nel cannocchiale.\n"
            "Calendario: Luna, buio e Via Lattea nelle prossime notti.\n"
            "Dove puntare: direzione e altezza del centro della Via Lattea, ora per ora.\n"
            "Luna da fotografare: l'esposizione giusta per fase e altezza.\n"
            "Etichetta file: LGHT, DARK, FLAT o BIAS al posto di IMG_.";
        case SCR_FOTO: return
            "Funzioni di Magic Lantern spiegate semplici: Iride le accende e le regola.\n"
            "HDR: piu' scatti a esposizioni diverse. Focus stacking: il fuoco si sposta tra uno scatto e l'altro.\n"
            "Dual ISO ed ETTR: piu' gamma dinamica nel RAW.\n"
            "Movimento: scatta da sola quando qualcosa cambia.\n"
            "Filtri ND: il tempo giusto col filtro.\n"
            "Torna alla 6D rimette i valori di prima.";
        case SCR_NPF: return
            "La regola NPF dice quanto puo' durare la posa prima che la rotazione della Terra allunghi le stelle in trattini.\n"
            "Dipende da focale, diaframma e pixel del sensore (6,54 micron sulla 6D).\n"
            "Punto del cielo: verso il polo le stelle si muovono meno e la posa si allunga; sull'equatore celeste e' la piu' corta.\n"
            "Applica: imposta il tempo sulla 6D (ghiera su M).";
        case SCR_INTERVAL: return
            "Scatta da sola una serie di foto. Il preset sceglie scatti, posa e pausa (Via Lattea, Deep sky, Star trails...).\n"
            "Prima di partire stima durata, ora di fine, batteria e spazio sulla scheda.\n"
            "Otturatore silenzioso: foto senza alzare lo specchio, da 0,3 a 30 secondi. Serve silent.mo; i file non hanno i dati EXIF.\n"
            "Dithering: ogni tot scatti una pausa per spostare di poco la montatura.\n"
            "Registro: un file CSV con ogni scatto, per AstroStack.\n"
            "Si ferma da sola con la batteria quasi scarica o la scheda piena.";
        case SCR_STARS: return
            "Metti una stella luminosa nel riquadro; SET ingrandisce x5 e x10.\n"
            "HFR: il raggio che contiene meta' della luce della stella. Piu' e' piccolo, meglio e' a fuoco.\n"
            "</> gira il fuoco, ^/v cambia il passo.\n"
            "Q: fuoco automatico. Iride gira il fuoco, disegna la curva a V e si ferma sul minimo, arrivandoci sempre dallo stesso verso. L'obiettivo deve essere su AF.";
        case SCR_POLAR: return
            "Dove mettere la Polare nel cannocchiale polare della montatura, per il luogo e l'ora impostati.\n"
            "Occhio nudo: come la vedi guardando a nord. Cannocchiale: l'immagine capovolta.\n"
            "Luogo e fuso orario si scelgono in Opzioni.";
        case SCR_SKY: return
            "Una riga per notte: data, Luna, la striscia della notte, le ore di buio senza Luna e quando si vede il centro della Via Lattea.\n"
            "La striscia va dalla sera alla mattina: nero e' cielo buio, grigio e' cielo schiarito dalla Luna, i puntini sono le stelle che si vedono.\n"
            "La stellina segna la notte migliore. SET: dove puntare quella notte.";
        case SCR_SHELF: return
            "Ogni pellicola imposta ISO, bilanciamento del bianco e stile della 6D per somigliare a quel film nel JPEG (il RAW resta neutro), e conta le pose come un rullino vero.\n"
            "Push / pull: la pellicola tirata o frenata.\n"
            "Torna alla 6D mette in pausa il rullino.";
        case SCR_HDR: return
            "La 6D scatta piu' foto di fila, piu' chiare e piu' scure, da unire poi in HDR al computer.\n"
            "Scatti: quante foto. Passo: quanti stop tra una e l'altra. Ordine: in che sequenza.\n"
            "Col treppiede; parte col normale pulsante di scatto.";
        case SCR_FSTACK: return
            "Piu' foto col fuoco spostato un poco tra una e l'altra, da unire al computer per avere tutto a fuoco (macro, paesaggi).\n"
            "Metti a fuoco il punto di partenza in Live View, scegli quante foto davanti e dietro e il passo, poi avvia. L'obiettivo deve essere su AF: lo muove Iride.";
        case SCR_DUAL: return
            "Dual ISO legge meta' delle righe del sensore a un ISO e meta' a un altro: nel RAW c'e' piu' gamma dinamica.\n"
            "Il file va convertito al computer (cr2hdr).\n"
            "ISO di recupero: il secondo ISO, quello per le ombre.";
        case SCR_ETTR: return
            "Auto ETTR sceglie l'esposizione piu' chiara possibile senza bruciare le luci: meno rumore nelle ombre del RAW.\n"
            "Quando: sempre, dopo ogni scatto (Auto Snap), premendo SET o con un doppio mezzo scatto.";
        case SCR_MOTION: return
            "Scatto su movimento: la 6D scatta da sola quando nell'inquadratura cambia la luce o l'immagine (fulmini, animali). Serve il Live View.\n"
            "Sensibilita': quanto deve cambiare l'immagine per scattare.\n"
            "Trappola di fuoco: scatta quando il soggetto entra a fuoco (obiettivo in MF).";
        case SCR_LVTOOLS: return
            "Aiuti che si vedono nel Live View.\n"
            "Focus peaking: colora i bordi a fuoco. Zebra: righe sulle zone bruciate. Istogramma e falsi colori: per l'esposizione. Zoom di fuoco: un riquadro ingrandito. Esposimetro spot: la luce al centro.\n"
            "Live View al buio: schiarisce lo schermo da 1 a 6 stop per inquadrare di notte; le foto non cambiano.";
        case SCR_ND: return
            "Metti il tempo giusto senza filtro e scegli il filtro: Iride calcola il tempo col filtro.\n"
            "Oltre 30 s usa la posa B e chiude lo scatto da sola.";
        case SCR_OPTIONS: return
            "Tema: normale o rosso per la notte.\n"
            "Luogo: scegli una citta' o regola latitudine e longitudine; serve per Luna, Sole, Polare e Via Lattea.\n"
            "Fuso orario: Europa, USA, Australia e Nuova Zelanda con l'ora legale automatica, oppure uno scarto fisso da UTC.\n"
            "Cielo del luogo: quanto e' buio il tuo cielo, per stimare le stelle che si vedono.\n"
            "Prova della 6D: controlla Iride su questa macchina.";
        case SCR_RESTORE: return
            "Rimette la 6D come prima di Iride: stile, bilanciamento, funzioni di Magic Lantern accese da Iride e nomi dei file. Il rullino va in pausa.";
        case SCR_BENCH: return
            "Prova le parti di Iride su questa 6D e misura i tempi. SET avvia.\n"
            "Il risultato resta nella scheda, in ML/LOGS/IRIDEBEN.TXT: allegalo se qualcosa non va.";
        case SCR_MOON: return
            "Esposizione per la Luna con la regola Looney 11 (f/11 a 1/ISO con la Luna piena), corretta per la fase e per l'altezza sull'orizzonte.\n"
            "Soggetto: la parte illuminata, o la luce cinerea (la parte scura rischiarata dalla Terra).\n"
            "Applica: imposta tempo, ISO e diaframma (ghiera su M). Oltre 30 s serve la posa B.";
        case SCR_MWPLAN: return
            "Il cielo da est a ovest, con l'altezza in gradi: la fascia e' la Via Lattea, il punto il suo centro (verde quando si vede).\n"
            "Sotto: la direzione da guardare con la bussola e l'altezza.\n"
            "</> cambia l'ora, ^/v la notte.";
    }
    return 0;
}

static int help_scr = -1;                  /* schermata con l'aiuto aperto, -1 = chiuso */

/* righe che servono per un paragrafo (stesso taglio di txt_wrap) */
static int wrap_lines(int font, int w, const char *s, int n)
{
    char line[96];
    int lines = 0, i = 0;
    while (i < n)
    {
        while (i < n && s[i] == ' ') i++;
        if (i >= n) break;
        int k = 0, cut = 0;
        while (i + k < n && k < (int)sizeof(line) - 1)
        {
            line[k] = s[i + k]; line[k + 1] = 0;
            if (txt_w(font, line) > w) break;
            if (s[i + k] == ' ') cut = k;
            k++;
        }
        if (i + k < n && cut > 0) k = cut;
        if (k == 0) k = 1;
        i += k;
        lines++;
    }
    return lines;
}

static void draw_help()
{
    const char *t = help_text(screen);
    if (!t) return;
    int x = 10, y = 52, w = SCR_W - 20, h = 386;
    fill_r(C_BG, x, y, w, h, R_CARD);
    ring_r(C_ACCENT, x, y, w, h, R_CARD, 2);
    char buf[64];
    snprintf(buf, sizeof(buf), "Aiuto: %s", screens[screen].title);
    txt(FONT_MED, C_ACCENT, x + 18, y + 12, buf);
    /* testo medio se ci sta, altrimenti piccolo */
    int tw = w - 36, font = FONT_MED, lh = 25, need = 0;
    for (int pass = 0; pass < 2; pass++)
    {
        need = 0;
        for (const char *p = t; *p; )
        {
            const char *e = strchr(p, '\n');
            int n = e ? (int)(e - p) : (int)strlen(p);
            need += wrap_lines(font, tw, p, n) * lh + lh / 3;
            p += n + (e ? 1 : 0);
        }
        if (need <= h - 60) break;
        font = FONT_SMALL; lh = 20;
    }
    int ty = y + 48;
    char para[400];
    for (const char *p = t; *p; )
    {
        const char *e = strchr(p, '\n');
        int n = e ? (int)(e - p) : (int)strlen(p);
        n = MIN(n, (int)sizeof(para) - 1);
        memcpy(para, p, n); para[n] = 0;
        ty = txt_wrap(font, C_TEXT, x + 18, ty, tw, para, lh) + lh / 3;
        p += (e ? (int)(e - p) + 1 : (int)strlen(p));
        if (ty > y + h - lh) break;
    }
    draw_footer("un tasto qualsiasi chiude l'aiuto");
}

static void render(int dir)
{
    const struct screen_def *sd = &screens[screen];
    frame_begin();
    if (sd->form)
    {
        fill(C_BG, 0, 46, SCR_W, 398);
        draw_form(sd->form, sel[screen]);
        if (sd->form->extra) sd->form->extra();
        draw_header(sd->title);          /* dopo: copre le righe che scorrono fuori */
        draw_footer(sd->form->hint);
    }
    else if (sd->draw) sd->draw();
    if (help_scr >= 0 && help_scr != screen) help_scr = -1;   /* cambiata schermata: l'aiuto si chiude */
    if (help_scr == screen) draw_help();
    frame_show(dir);
}

static void handle_key(int key)
{
    const struct screen_def *sd = &screens[screen];
    if (help_scr >= 0) { help_scr = -1; dirty = 1; return; }            /* un tasto qualsiasi chiude l'aiuto */
    if (key == MODULE_KEY_INFO && help_text(screen)) { help_scr = screen; dirty = 1; return; }
    int dx = key_dx(key), dy = key_dy(key);
    if (dx || key == MODULE_KEY_PRESS_SET) val_kick(dx);
    if (key == MODULE_KEY_MENU)
    {
        if (screen == SCR_HOME) gui_stop_menu();
        else if (screen == SCR_MWPLAN && mw_back_sky) { mw_back_sky = 0; sky_keep = 1; sky_sel = mw_night; go(SCR_SKY, -1); }
        else go(sd->parent, -1);
        return;
    }
    if (sd->form)
    {
        const struct form *f = sd->form;
        int s = sel[screen];
        if (dy) sel[screen] = COERCE(s + dy, 0, f->n - 1);
        else if (dx && f->change) f->change(s, dx, key_big(key));
        else if (key == MODULE_KEY_PRESS_SET && f->activate) f->activate(s);
    }
    else if (sd->key) sd->key(key, dx, dy);
    dirty = 1;
}
