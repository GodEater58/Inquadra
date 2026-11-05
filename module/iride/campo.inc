/* campo.inc - la macchina in campo: batteria, scheda, temperatura; registro degli avvisi;
 * sessione per AstroStack (anche per gli scatti comandati dal cavo, per esempio dalla montatura) */

/* ------------------------------------------------------------------ batteria, scheda, temperatura */

/* percentuale della batteria, -1 se la 6D non l'ha ancora detta */
static int cam_battery()
{
    int b = GetBatteryLevel();
    return (b > 0 && b <= 100) ? b : -1;
}

/* secondi di autonomia stimati da Magic Lantern (0: la stima non e' ancora pronta) */
static int cam_battery_left_s()
{
    int s = GetBatteryTimeRemaining();
    return s > 0 && s < 48 * 3600 ? s : 0;
}

/* temperatura interna (EFIC della 6D: segue quella del sensore, non e' la stessa) */
static int cam_temp_c()
{
    return (int)efic_temp * 80 / 100 - 93;
}

/* spazio libero sulla scheda delle foto in MB, -1 se non si sa */
static int cam_free_mb()
{
    struct card_info *c = get_shooting_card();
    if (!c) return -1;
    int k = get_free_space_32k(c);
    return k >= 0 ? k / 32 : -1;
}

/* MB per scatto, larghi: di notte con ISO alti i RAW pesano di piu' */
static int cam_shot_mb(int silent)
{
    if (silent) return 36;                       /* DNG a piena risoluzione, non compresso */
    int raw = (pic_quality & 0x60000) != 0, jpg = (pic_quality & 0x10000) != 0;
    return (raw ? 30 : 0) + (jpg ? 8 : 0) + (!raw && !jpg ? 8 : 0);
}

/* quanti scatti ci stanno ancora sulla scheda (-1 se non si sa) */
static int cam_shots_left(int silent)
{
    int mb = cam_free_mb();
    return mb < 0 ? -1 : mb / MAX(cam_shot_mb(silent), 1);
}

/* ------------------------------------------------------------------ registro degli avvisi */

/* gli avvisi gialli finiscono anche nel registro su scheda, per capire cosa e' successo in campo;
   lo stesso avviso ripetuto entro 10 s si scrive una volta sola */
static void log_warning(const char *text)
{
    static char last[96];
    static int last_ms = -100000;
    int now = get_ms_clock();
    if (!strcmp(last, text) && now - last_ms < 10000) return;
    snprintf(last, sizeof(last), "%s", text);
    last_ms = now;
    if (lens_info.job_state) return;             /* mai mentre Canon salva una foto */
    char line[128];
    snprintf(line, sizeof(line), "AVVISO,%s", text);
    log_line(line);
}

/* ------------------------------------------------------------------ sessione per AstroStack */

static char sess_path[96];
static char sess_counted[96];                        /* file di cui sess_frames conta gia' le righe */
static int sess_frames = 0;
static int sess_ext_fn = -1;                         /* ultimo numero di file visto per gli scatti esterni */

static float sky_now_nelm(float *moon_alt, int *illum);
static int tz_now_min();
static int days_from_civil(int y, int m, int d);
static void civil_from_days(int d70, int *y, int *m, int *d);

/* data della notte: fino a mezzogiorno conta ancora la sera prima (una sessione dalle 22 alle 3
   finisce tutta nello stesso file) */
static void session_night(const struct tm *now, int *y, int *m, int *d)
{
    *y = now->tm_year + 1900; *m = now->tm_mon + 1; *d = now->tm_mday;
    if (now->tm_hour < 12) civil_from_days(days_from_civil(*y, *m, *d) - 1, y, m, d);
}

/* apre (o riprende) il file della notte nella cartella delle foto: ISaammgg.CSV (anno, mese e
   giorno della sera; fino alla 1.4.1 era ISmmddhh, un file per ora) */
static FILE *session_open()
{
    struct tm now;
    LoadCalendarFromRTC(&now);
    const char *dir = get_dcim_dir();
    if (!dir || !dir[0]) return 0;
    int n = (int)strlen(dir), ny, nm, nd;
    session_night(&now, &ny, &nm, &nd);
    snprintf(sess_path, sizeof(sess_path), "%s%sIS%02d%02d%02d.CSV", dir, dir[n - 1] == '/' ? "" : "/",
             ny % 100, nm, nd);
    FILE *f = FIO_OpenFile(sess_path, O_RDONLY | O_SYNC);
    int fresh = !f;
    if (f)
    {
        /* file gia' iniziato (per esempio prima di un cambio di batteria): la numerazione
           riprende dall'ultima riga invece di ricominciare da 1 */
        if (strcmp(sess_counted, sess_path))
        {
            char chunk[256];
            int lines = 0, r;
            while ((r = FIO_ReadFile(f, chunk, sizeof(chunk))) > 0)
                for (int i = 0; i < r; i++) if (chunk[i] == '\n') lines++;
            sess_frames = MAX(lines - 4, 0);     /* 3 righe di commento + intestazione */
        }
        FIO_CloseFile(f);
    }
    snprintf(sess_counted, sizeof(sess_counted), "%s", sess_path);
    f = FIO_CreateFileOrAppend(sess_path);
    if (!f) return 0;
    if (fresh)
    {
        char la[12], lo[12];
        snprintf(la, sizeof(la), "%d.%02d %c", ABS(P.lat100) / 100, ABS(P.lat100) % 100, P.lat100 < 0 ? 'S' : 'N');
        snprintf(lo, sizeof(lo), "%d.%02d %c", ABS(P.lon100) / 100, ABS(P.lon100) % 100, P.lon100 < 0 ? 'W' : 'E');
        sess_frames = 0;                             /* file nuovo: si conta da 1 */
        my_fprintf(f, "# Iride 6D %s - sessione del %04d-%02d-%02d\n", IRIDE_VERSION, ny, nm, nd);
        my_fprintf(f, "# luogo %s %s, cielo del luogo %d.%d mag/arcsec2, fuso %+d min\n", la, lo, opt_sqm10 / 10, opt_sqm10 % 10, tz_now_min());
        my_fprintf(f, "# obiettivo %s\n", lens_info.name[0] ? lens_info.name : "manuale");
        my_fprintf(f, "numero,file,tipo,ora,posa_s,iso,diaframma,focale_mm,temperatura_c,batteria,luna_pct,luna_alt,mag_limite,otturatore,origine\n");
    }
    return f;
}

/* una riga per scatto. fn: numero del file Canon (0 per gli scatti silenziosi, che hanno nomi loro) */
static void session_frame(int fn, int tag, float expo_s, int silent, const char *origin)
{
    if (!opt_session) return;
    FILE *f = session_open();
    if (!f) return;
    struct tm now;
    LoadCalendarFromRTC(&now);
    char name[24], ex[16], ap[12];
    const char *pre = tag > 0 && tag < NUM_TAGS ? tag_names[tag] : "IMG_";
    int raw = (pic_quality & 0x60000) != 0;
    if (silent) (snprintf)(name, sizeof(name), "%s", "silenzioso");
    else snprintf(name, sizeof(name), "%s%04d.%s", pre, fn % 10000, raw ? "CR2" : "JPG");
    int e1000 = (int)(expo_s * 1000.0f + 0.5f);
    snprintf(ex, sizeof(ex), "%d.%03d", e1000 / 1000, e1000 % 1000);
    if (lens_info.raw_aperture) { int a10 = RAW2VALUE(aperture, lens_info.raw_aperture); snprintf(ap, sizeof(ap), "%d.%d", a10 / 10, a10 % 10); }
    else snprintf(ap, sizeof(ap), "-");
    float malt = 0.0f;
    int ill = 0;
    float ne = sky_now_nelm(&malt, &ill);
    int ne10 = (int)(ne * 10.0f + 0.5f), ma10 = (int)(malt * 10.0f + (malt >= 0 ? 0.5f : -0.5f));
    sess_frames++;
    my_fprintf(f, "%d,%s,%s,%02d:%02d:%02d,%s,%d,%s,%d,%d,%d,%d,%s%d.%d,%d.%d,%s,%s\n",
        sess_frames, name, tag > 0 && tag < NUM_TAGS ? tag_names[tag] : "LGHT", now.tm_hour, now.tm_min, now.tm_sec,
        ex, lens_info.iso, ap, lens_info.focal_len, cam_temp_c(), cam_battery(), (ill + 5) / 10,
        ma10 < 0 ? "-" : "", ABS(ma10) / 10, ABS(ma10) % 10, ne10 / 10, ne10 % 10,
        silent ? "silenzioso" : "meccanico", origin);
    FIO_CloseFile(f);
}

/* scatti comandati da fuori (cavo, montatura): con l'etichetta astro attiva ogni nuovo file va nel registro.
   Chiamata ogni secondo, solo a macchina ferma */
static void session_watch_external(int fn)
{
    if (!opt_session || P.astro_tag <= 0 || job_running != JOB_NONE || lens_info.job_state) { sess_ext_fn = fn; return; }
    if (sess_ext_fn < 0) { sess_ext_fn = fn; return; }
    if (fn == sess_ext_fn) return;
    int n = MOD(fn - sess_ext_fn, 9999);             /* i numeri dei file vanno da 0001 a 9999 */
    if (n > 0 && n < 50)
    {
        float t = lens_info.raw_shutter > 0 ? raw2shutterf(lens_info.raw_shutter) : 0.0f;
        for (int k = 1; k <= n; k++) session_frame(MOD(sess_ext_fn + k - 1, 9999) + 1, P.astro_tag, t, 0, "cavo");
    }
    sess_ext_fn = fn;
}
