/* store.inc - salvataggio su scheda, prefisso dei file, registro, stili immagine,
 * funzioni di Magic Lantern, "Torna alla 6D", profili */

/* ------------------------------------------------------------------ salvataggio */

static int persist_sum(const struct persist *p)
{
    const int *w = (const int *)p;
    int n = (int)(sizeof(struct persist) / sizeof(int)) - 1;   /* escluso il checksum */
    unsigned int h = 2166136261u;
    for (int i = 0; i < n; i++) h = (h ^ (unsigned int)w[i]) * 16777619u;
    return (int)h;
}

static const char *const prof_names[] = {
    "Ritratto", "Paesaggio", "Street", "Notte", "Astro", "Sport", "Macro", "Viaggio", "Studio", "Personale"
};
#define NPROF_NAMES ((int)COUNT(prof_names))

static void persist_defaults()
{
    memset(&P, 0, sizeof(P));
    P.roll_film = 1; P.roll_exp = 36;
    P.int_count = 20; P.int_pause = 2; P.int_screen_off = 1;
    P.npf_focal = 24; P.npf_ap10 = 28;
    P.lat100 = 4190; P.lon100 = 1250;          /* Roma: si cambia in Opzioni */
    P.hdr_frames = 1; P.hdr_step = 2; P.hdr_seq = 1;
    P.fs_front = 5; P.fs_behind = 5; P.fs_step = 3;
    P.dual_rec = 1; P.md_trigger = 1; P.md_level = 8;
    P.nd_idx = 7; P.nd_base = 0;
    for (int i = 0; i < NPROF; i++) P.prof[i].name = i;
}

static void persist_path(char *buf, int len)
{
    snprintf(buf, len, "%sIRIDE.DAT", get_config_dir());
}

static void persist_load()
{
    persist_defaults();
    char path[96];
    persist_path(path, sizeof(path));
    FILE *f = FIO_OpenFile(path, O_RDONLY | O_SYNC);
    if (!f) return;
    struct persist *buf = fio_malloc(sizeof(struct persist));
    if (buf)
    {
        int r = FIO_ReadFile(f, buf, sizeof(struct persist));
        if (r == (int)sizeof(struct persist) && buf->magic == PERSIST_MAGIC &&
            buf->version == PERSIST_VERSION && buf->size == (int)sizeof(struct persist) &&
            buf->checksum == persist_sum(buf))
        {
            memcpy(&P, buf, sizeof(P));
        }
        fio_free(buf);
    }
    FIO_CloseFile(f);
}

static void persist_save()
{
    P.magic = PERSIST_MAGIC;
    P.version = PERSIST_VERSION;
    P.size = sizeof(struct persist);
    P.checksum = persist_sum(&P);
    char path[96];
    persist_path(path, sizeof(path));
    struct persist *buf = fio_malloc(sizeof(struct persist));
    if (!buf) return;
    memcpy(buf, &P, sizeof(P));
    FILE *f = FIO_CreateFile(path);
    if (f)
    {
        FIO_WriteFile(f, buf, sizeof(struct persist));
        FIO_CloseFile(f);
        persist_dirty = 0;
    }
    fio_free(buf);
}

static void persist_save_if_dirty()
{
    if (persist_dirty) persist_save();
}

/* ------------------------------------------------------------------ prefisso dei file */

/* il cambio di prefisso blocca fino a 1 s: solo dai task di Iride, mai dal gestore dei tasti */
static volatile int prefix_pending = 0;
static int prefix_key = 0;
static char prefix_cur[8] = "";
static int seq_tag = 0;

static const char *const tag_names[] = { "OFF", "LGHT", "DARK", "FLAT", "BIAS" };
#define NUM_TAGS ((int)COUNT(tag_names))

static void prefix_apply(const char *p)
{
    if (p && streq(p, prefix_cur)) return;
    if (prefix_key)
    {
        file_prefix_reset(prefix_key);
        prefix_key = 0;
    }
    prefix_cur[0] = 0;
    if (p && p[0])
    {
        char tmp[8];
        snprintf(tmp, sizeof(tmp), "%s", p);
        int k = file_prefix_set(tmp);
        if (k)
        {
            prefix_key = k;
            snprintf(prefix_cur, sizeof(prefix_cur), "%s", p);
        }
    }
}

static int roll_active() { return P.roll_loaded && !P.roll_paused; }

/* priorita': sequenza astro > etichetta manuale > rullino attivo */
static void prefix_refresh()
{
    char p[8] = "";
    if (job_running == JOB_SEQ && seq_tag > 0)
        snprintf(p, sizeof(p), "%s", tag_names[seq_tag]);
    else if (P.astro_tag > 0 && P.astro_tag < NUM_TAGS)
        snprintf(p, sizeof(p), "%s", tag_names[P.astro_tag]);
    else if (roll_active())
        snprintf(p, sizeof(p), "R%03d", P.roll_num % 1000);
    prefix_apply(p);
}

/* ------------------------------------------------------------------ registro su scheda */

static void log_line(const char *line)
{
    FIO_CreateDirectory("ML/LOGS");
    FILE *f = FIO_CreateFileOrAppend("ML/LOGS/IRIDE.CSV");
    if (!f) return;
    struct tm now;
    LoadCalendarFromRTC(&now);
    my_fprintf(f, "%04d-%02d-%02d %02d:%02d,%s\n",
        now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, line);
    FIO_CloseFile(f);
}

/* ------------------------------------------------------------------ stili immagine */

#define PS_STD       0x81
#define PS_PORTRAIT  0x82
#define PS_LANDSCAPE 0x83
#define PS_NEUTRAL   0x84
#define PS_FAITHFUL  0x85
#define PS_MONO      0x86

static int style_index(int id)
{
    if (id >= 0x81 && id <= 0x87) return id - 0x81;
    if (id >= 0x21 && id <= 0x23) return 7 + id - 0x21;
    return -1;
}

static int style_id(int index) { return index < 7 ? 0x81 + index : 0x21 + index - 7; }
static int valid_picstyle(int id) { return style_index(id) >= 0; }
static int valid_review(int r) { return r == 0 || r == 2 || r == 4 || r == 8 || r == 0xff; }
static int valid_slider(int v) { return v >= -4 && v <= 4; }

static const char *style_name(int id)
{
    switch (id)
    {
        case PS_STD:       return "Standard";
        case PS_PORTRAIT:  return "Ritratto";
        case PS_LANDSCAPE: return "Paesaggio";
        case PS_NEUTRAL:   return "Neutro";
        case PS_FAITHFUL:  return "Fedele";
        case PS_MONO:      return "Monocromo";
        case 0x87:         return "Auto";
        case 0x21: case 0x22: case 0x23: return "Utente";
    }
    return "?";
}

/* un valore sbagliato non va mai mandato a Canon */
static void set_picstyle(int id)
{
    if (!valid_picstyle(id)) return;
    if (lens_info.raw_picstyle == (unsigned)id) return;
    prop_request_change(PROP_PICTURE_STYLE, &id, 4);
    for (int i = 0; i < 20 && lens_info.raw_picstyle != (unsigned)id; i++) msleep(25);
}

/* cambia i cursori di uno stile ricordando una sola volta i valori originali */
static void style_set_sliders(int id, int c, int s, int t)
{
    set_picstyle(id);
    if (lens_info.raw_picstyle != (unsigned)id) return;
    int i = style_index(id);
    if (!(P.ps_touched & (1 << i)))
    {
        P.ps_c[i] = ps_get_contrast();
        P.ps_s[i] = ps_get_saturation();
        P.ps_t[i] = ps_get_tone();
        P.ps_touched |= 1 << i;
        persist_touch();
    }
    if (valid_slider(c)) ps_set_contrast(c);
    if (id != PS_MONO)
    {
        msleep(80);
        if (valid_slider(s)) ps_set_saturation(s);
        msleep(80);
        if (valid_slider(t)) ps_set_tone(t);
    }
    msleep(80);
}

static void styles_restore()
{
    for (int i = 0; i < NSTYLE; i++)
    {
        if (!(P.ps_touched & (1 << i))) continue;
        int id = style_id(i);
        set_picstyle(id);
        if (lens_info.raw_picstyle == (unsigned)id)
        {
            if (valid_slider(P.ps_c[i])) ps_set_contrast(P.ps_c[i]);
            if (id != PS_MONO)
            {
                msleep(80);
                if (valid_slider(P.ps_s[i])) ps_set_saturation(P.ps_s[i]);
                msleep(80);
                if (valid_slider(P.ps_t[i])) ps_set_tone(P.ps_t[i]);
            }
            msleep(80);
        }
        P.ps_touched &= ~(1 << i);
    }
    persist_touch();
}

/* ------------------------------------------------------------------ "Torna alla 6D" */

/* prima di cambiare qualcosa: fotografa le impostazioni della macchina (una volta) */
static void snap_take()
{
    if (P.snap_valid) return;
    P.snap_mode = shooting_mode;
    P.snap_iso = lens_info.raw_iso;
    P.snap_shutter = lens_info.raw_shutter;
    P.snap_aperture = lens_info.raw_aperture;
    P.snap_wb = lens_info.wb_mode;
    P.snap_kelvin = lens_info.kelvin;
    P.snap_style = lens_info.raw_picstyle;
    P.snap_review = image_review_time;
    P.snap_valid = 1;
    persist_touch();
}

static void set_wb(int mode, int kelvin)
{
    if (mode == WB_KELVIN)
    {
        if (kelvin >= 2500 && kelvin <= 10000) lens_set_kelvin(kelvin);
        return;
    }
    if (mode < 0 || mode > 9 || mode == 7) return;
    prop_request_change(PROP_WB_MODE_LV, &mode, 4);
    prop_request_change(PROP_WB_MODE_PH, &mode, 4);
}

/* rimette stile, ISO, tempo, diaframma, bianco e anteprima della fotografia */
static void restore_camera()
{
    styles_restore();
    if (!P.snap_valid) return;
    set_picstyle(P.snap_style);
    if (shooting_mode == P.snap_mode)
    {
        if (P.snap_iso == 0 || (P.snap_iso >= 72 && P.snap_iso <= 136)) lens_set_rawiso(P.snap_iso);
        if ((shooting_mode == SHOOTMODE_M || shooting_mode == SHOOTMODE_TV) && P.snap_shutter >= 12)
            lens_set_rawshutter(P.snap_shutter);
        if ((shooting_mode == SHOOTMODE_M || shooting_mode == SHOOTMODE_AV) && P.snap_aperture)
            lens_set_rawaperture(P.snap_aperture);
    }
    set_wb(P.snap_wb, P.snap_kelvin);
    if (valid_review(P.snap_review))
    {
        int r = P.snap_review;
        prop_request_change(PROP_IMAGE_REVIEW_TIME, &r, 4);
    }
    P.snap_valid = 0;
    persist_touch();
}

/* ------------------------------------------------------------------ funzioni di Magic Lantern */

enum
{
    MLF_HDR, MLF_HDR_FRAMES, MLF_HDR_SEQ, MLF_HDR_STEP,
    MLF_FS_FRONT, MLF_FS_BEHIND, MLF_FS_STEP,
    MLF_DUAL, MLF_DUAL_REC, MLF_ETTR, MLF_ETTR_TRIG,
    MLF_MD, MLF_MD_TRIG, MLF_MD_LEVEL, MLF_TRAP,
    MLF_GDRAW, MLF_ZEBRA, MLF_PEAK, MLF_ZOOM, MLF_HIST, MLF_FALSE, MLF_SPOT,
    MLF_AUTOEXPO, MLF_DEFLICK, MLF_COUNT
};

/* scheda del menu ML e voce: le sottovoci stanno in un menu che ha il nome della voce madre */
static const char *const mlf_menu[MLF_COUNT] = {
    "Shoot", "Advanced Bracket", "Advanced Bracket", "",
    "Focus Stacking", "Focus Stacking", "Focus Stacking",
    "Expo", "Dual ISO", "Expo", "Auto ETTR",
    "Shoot", "Motion Detect", "Motion Detect", "Focus",
    "Overlay", "Overlay", "Overlay", "Overlay", "Overlay", "Overlay", "Overlay",
    "Expo", "Shoot",
};
static const char *const mlf_entry[MLF_COUNT] = {
    "Advanced Bracket", "Frames", "Sequence", "",
    "Num. pics in front", "Num. pics behind", "Focus steps / picture",
    "Dual ISO", "Recovery ISO", "Auto ETTR", "Trigger mode",
    "Motion Detect", "Trigger by", "Trigger level", "Trap Focus",
    "Global Draw", "Zebras", "Focus Peak", "Magic Zoom", "Histogram", "False color", "Spotmeter",
    "Auto exposure", "Post Deflicker",
};

static int ml_get(int f)
{
    if (f == MLF_HDR_STEP) return hdr_stepsize;
    return menu_get_value_from_script(mlf_menu[f], mlf_entry[f]);
}

static int ml_available(int f) { return ml_get(f) != INT_MIN; }

static int ml_set(int f, int v)
{
    int cur = ml_get(f);
    if (cur == INT_MIN) return 0;
    if (!(P.ml_touched & (1 << f)))
    {
        P.ml_prev[f] = cur;
        P.ml_touched |= 1 << f;
        persist_touch();
    }
    if (cur == v) return 1;
    if (f == MLF_HDR_STEP) { hdr_stepsize = v; return 1; }
    return menu_set_value_from_script(mlf_menu[f], mlf_entry[f], v) == 1;
}

static void ml_restore(int f)
{
    if (!(P.ml_touched & (1 << f))) return;
    if (f == MLF_HDR_STEP) hdr_stepsize = P.ml_prev[f];
    else menu_set_value_from_script(mlf_menu[f], mlf_entry[f], P.ml_prev[f]);
    P.ml_touched &= ~(1 << f);
    persist_touch();
}

static void ml_restore_all()
{
    for (int f = 0; f < MLF_COUNT; f++) ml_restore(f);
}

/* "Torna alla 6D": tutto come prima di Iride */
static void restore_all()
{
    restore_camera();
    ml_restore_all();
    P.astro_tag = 0;
    if (P.roll_loaded) P.roll_paused = 1;
    persist_touch();
    prefix_refresh();
}

static int restore_pending_count()
{
    int n = 0;
    if (P.snap_valid) n++;
    for (int i = 0; i < NSTYLE; i++) if (P.ps_touched & (1 << i)) n++;
    for (int f = 0; f < MLF_COUNT; f++) if (P.ml_touched & (1 << f)) n++;
    if (P.astro_tag) n++;
    if (roll_active()) n++;
    return n;
}

/* ------------------------------------------------------------------ profili */

static void profile_save(int i)
{
    struct profile *p = &P.prof[i];
    p->valid = 1;
    p->mode = shooting_mode;
    p->iso = lens_info.raw_iso;
    p->shutter = lens_info.raw_shutter;
    p->aperture = lens_info.raw_aperture;
    p->wb = lens_info.wb_mode;
    p->kelvin = lens_info.kelvin;
    p->style = lens_info.raw_picstyle;
    p->c = ps_get_contrast();
    p->s = ps_get_saturation();
    p->t = ps_get_tone();
    persist_touch();
}

/* ritorna 1 se anche tempo e diaframma sono stati applicati (stessa ghiera) */
static int profile_load(int i)
{
    struct profile *p = &P.prof[i];
    if (!p->valid) return 0;
    snap_take();
    if (valid_picstyle(p->style)) style_set_sliders(p->style, p->c, p->s, p->t);
    if (p->iso == 0 || (p->iso >= 72 && p->iso <= 136)) lens_set_rawiso(p->iso);
    set_wb(p->wb, p->kelvin);
    if (shooting_mode != p->mode) return 0;
    if ((shooting_mode == SHOOTMODE_M || shooting_mode == SHOOTMODE_TV) && p->shutter >= 12) lens_set_rawshutter(p->shutter);
    if ((shooting_mode == SHOOTMODE_M || shooting_mode == SHOOTMODE_AV) && p->aperture) lens_set_rawaperture(p->aperture);
    return 1;
}
