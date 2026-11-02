/* bench.inc - Prova della 6D: quanto ci mette la macchina vera a disegnare e a calcolare,
 * con batteria, temperatura e scheda; il risultato resta in ML/LOGS/IRIDEBEN.TXT */

static void draw_home();
static void draw_astro();
static void draw_shelf();

enum { BT_HOME, BT_ASTRO, BT_INTERVAL, BT_SHELF, BT_NIGHT, BT_GCSCAN, BT_SKY, BT_MOON, BT_POLAR, BT_N };
static const char *const bench_names[BT_N] = {
    "Home, un fotogramma", "Menu Astro, un fotogramma", "Intervallometro, un fotogramma", "Scaffale dei rullini",
    "Calendario: una notte calcolata", "Calendario: quando torna la Via Lattea", "Calendario, un fotogramma",
    "Luna grande, prima volta", "Allineamento polare, un fotogramma",
};
static int bench_us[BT_N];                       /* microsecondi (media), -1 = non ancora */
static int bench_step = -1;                      /* -1 ferma, 0..BT_N-1 in corso, BT_N finita */
static int bench_t0 = 0;

static void bench_enter()
{
    if (bench_step < BT_N && bench_step >= 0) return;
    if (bench_step < 0) for (int i = 0; i < BT_N; i++) bench_us[i] = -1;
}

/* tempo medio di n chiamate, disegnando nel buffer nascosto */
static int bench_time(void (*fn)(void), int n)
{
    bmp_draw_to_idle(1);
    int t0 = get_us_clock();
    for (int i = 0; i < n; i++) fn();
    int dt = get_us_clock() - t0;
    bmp_draw_to_idle(0);
    return dt / MAX(n, 1);
}

static void bench_interval() { draw_form(&int_form, 0); int_extra(); }
static void bench_moon()
{
    static int il = 100;
    ms_big.ok = 0;                               /* senza la memoria: il caso peggiore */
    il = (il + 137) % 1000;
    draw_moon(66, 106, 44, il, 1, 0);
}

static void bench_save()
{
    FIO_CreateDirectory("ML/LOGS");
    FILE *f = FIO_CreateFile("ML/LOGS/IRIDEBEN.TXT");
    if (!f) return;
    struct tm now;
    LoadCalendarFromRTC(&now);
    my_fprintf(f, "Iride 6D %s - prova del %04d-%02d-%02d %02d:%02d\n", IRIDE_VERSION,
               now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min);
    my_fprintf(f, "batteria %d%%, autonomia %d s, temperatura %d gradi, scheda %d MB liberi, live view %d\n",
               cam_battery(), cam_battery_left_s(), cam_temp_c(), cam_free_mb(), lv);
    for (int i = 0; i < BT_N; i++)
        my_fprintf(f, "%s;%d.%d ms\n", bench_names[i], bench_us[i] / 1000, bench_us[i] % 1000 / 100);
    FIO_CloseFile(f);
}

/* una prova per giro: la schermata mostra l'avanzamento */
static int bench_tick()
{
    if (bench_step < 0 || bench_step >= BT_N) return 0;
    int s = bench_step, t0;
    switch (s)
    {
        case BT_HOME: bench_us[s] = bench_time(draw_home, 3); break;
        case BT_ASTRO: bench_us[s] = bench_time(draw_astro, 3); break;
        case BT_INTERVAL: bench_us[s] = bench_time(bench_interval, 3); break;
        case BT_SHELF: bench_us[s] = bench_time(draw_shelf, 3); break;
        case BT_NIGHT:
        {
            int d70, mins, off;
            jt_now(&d70, &mins, &off);
            t0 = get_us_clock();
            sky_compute_night(0, d70);
            bench_us[s] = get_us_clock() - t0;
            break;
        }
        case BT_GCSCAN:
        {
            /* da domani in avanti, finche' non lo trova (o un anno) */
            int d70, mins, off;
            jt_now(&d70, &mins, &off);
            gcn_state = 1; gcn_from = gcn_day = d70 + 1;
            t0 = get_us_clock();
            while (gcn_state == 1) gc_scan_step();
            bench_us[s] = get_us_clock() - t0;
            break;
        }
        case BT_SKY:
            sky_enter();
            while (sky_tick()) ;
            bench_us[s] = bench_time(draw_sky, 3);
            break;
        case BT_MOON: bench_us[s] = bench_time(bench_moon, 3); break;
        case BT_POLAR: bench_us[s] = bench_time(draw_polar, 3); break;
    }
    bench_step++;
    if (bench_step >= BT_N) { bench_save(); say(C_OK, "Prova finita: salvata in ML/LOGS/IRIDEBEN.TXT"); }
    return 1;
}

static void bench_key(int key, int dx, int dy)
{
    if (key == MODULE_KEY_PRESS_SET && (bench_step < 0 || bench_step >= BT_N))
    {
        for (int i = 0; i < BT_N; i++) bench_us[i] = -1;
        bench_step = 0;
        bench_t0 = get_ms_clock();
    }
}

static void draw_bench()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Prova della 6D");
    char buf[96], v[32];
    /* la macchina adesso */
    info_card(24, 52, SCR_W - 48, 76, "LA MACCHINA ADESSO");
    int b = cam_battery(), left = cam_battery_left_s(), mb = cam_free_mb();
    if (b < 0) snprintf(v, sizeof(v), "-");
    else if (left > 0) snprintf(v, sizeof(v), "%d%%  ~%dh%02d", b, left / 3600, left % 3600 / 60);
    else snprintf(v, sizeof(v), "%d%%", b);
    txt(FONT_SMALL, C_DIM, 44, 80, "BATTERIA");
    txt(FONT_MED, C_TEXT, 44, 96, v);
    txt(FONT_SMALL, C_DIM, 250, 80, "TEMPERATURA");
    snprintf(v, sizeof(v), "%d gradi", cam_temp_c());
    txt(FONT_MED, C_TEXT, 250, 96, v);
    txt(FONT_SMALL, C_DIM, 450, 80, "SCHEDA");
    if (mb >= 0) snprintf(v, sizeof(v), "%d,%d GB liberi", mb / 1024, mb % 1024 * 10 / 1024);
    else snprintf(v, sizeof(v), "-");
    txt(FONT_MED, C_TEXT, 450, 96, v);

    /* le prove */
    const int y0 = 140, rh = 30, bx = 470, bw = 140;
    for (int i = 0; i < BT_N; i++)
    {
        int y = y0 + i * rh;
        if (i % 2 == 0) fill(C_PANEL, 24, y, SCR_W - 48, rh - 2);
        int running = bench_step == i;
        txt(FONT_MED, running ? C_ACCENT : C_TEXT, 40, y + 2, bench_names[i]);
        int us = bench_us[i];
        if (us < 0) { txt_r(FONT_MED, C_DIM, SCR_W - 40, y + 2, running ? "..." : "-"); continue; }
        /* sotto i 100 ms un fotogramma e' fluido; oltre i 400 si sente */
        int ms10 = us / 100;
        int c = us <= 100000 ? C_OK : us <= 400000 ? C_ACCENT : C_WARN;
        fill(C_PANEL2, bx, y + 10, bw, 8);
        fill(c, bx, y + 10, MIN(bw, bw * us / 500000), 8);
        snprintf(v, sizeof(v), "%d,%d ms", ms10 / 10, ms10 % 10);
        txt_r(FONT_MED, c, SCR_W - 40, y + 2, v);
    }
    if (bench_step >= 0 && bench_step < BT_N) anim_keep();
    if (bench_step < 0) snprintf(buf, sizeof(buf), "SET avvia la prova (qualche secondo)   MENU indietro");
    else if (bench_step < BT_N) snprintf(buf, sizeof(buf), "Prova in corso: %d di %d", bench_step + 1, BT_N);
    else snprintf(buf, sizeof(buf), "SET ripete   salvata in ML/LOGS/IRIDEBEN.TXT");
    draw_footer(buf);
}
