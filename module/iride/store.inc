/* store.inc - salvataggio su scheda, prefisso dei file, registro, stili immagine,
 * funzioni di Magic Lantern, "Torna alla 6D", profili */

/* ------------------------------------------------------------------ salvataggio */

/* somma di controllo delle prime n parole (il checksum e' la parola successiva) */
static int persist_sum_n(const void *p, int n)
{
    const int *w = (const int *)p;
    unsigned int h = 2166136261u;
    for (int i = 0; i < n; i++) h = (h ^ (unsigned int)w[i]) * 16777619u;
    return (int)h;
}

static int persist_sum(const struct persist *p)
{
    return persist_sum_n(p, (int)(sizeof(struct persist) / sizeof(int)) - 1);
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
    P.lat100 = 4189; P.lon100 = 1251;          /* Roma (come nell'elenco delle citta'): si cambia in Opzioni */
    P.hdr_frames = 1; P.hdr_step = 2; P.hdr_seq = 1;
    P.fs_front = 5; P.fs_behind = 5; P.fs_step = 3;
    P.dual_rec = 1; P.md_trigger = 1; P.md_level = 8;
    P.nd_idx = 7; P.nd_base = 0;
    for (int i = 0; i < NPROF; i++) P.prof[i].name = i;
}

/* due copie sulla scheda, scritte a turno (IRIDE.DAT e IRIDE.BAK) col numero del salvataggio:
   se la batteria si stacca o la scheda esce mentre si scrive, l'altra copia e' ancora buona e
   non si perdono rullino e "Torna alla 6D". Una 1.3 o precedente legge solo IRIDE.DAT. */
#define persist_seq reserved[0]
#define persist_prof_seeded reserved[1]       /* profili di partenza gia' messi (una volta sola) */
static int persist_next_slot = 0;
static struct semaphore *persist_sem = 0;

static void persist_path(char *buf, int len, int slot)
{
    snprintf(buf, len, "%s%s", get_config_dir(), slot ? "IRIDE.BAK" : "IRIDE.DAT");
}

/* 2 = file v3 valido, 1 = file v2 (1.0/1.1) valido, 0 = niente */
static int persist_read(int slot, struct persist *buf)
{
    char path[96];
    persist_path(path, sizeof(path), slot);
    FILE *f = FIO_OpenFile(path, O_RDONLY | O_SYNC);
    if (!f) return 0;
    int r = FIO_ReadFile(f, buf, sizeof(struct persist));
    FIO_CloseFile(f);
    const int *w = (const int *)buf;
    int v2_bytes = PERSIST_V2_WORDS * (int)sizeof(int);
    if (r == (int)sizeof(struct persist) && buf->magic == PERSIST_MAGIC &&
        buf->version == PERSIST_VERSION && buf->size == (int)sizeof(struct persist) &&
        buf->checksum == persist_sum(buf))
        return 2;
    if (r == v2_bytes && buf->magic == PERSIST_MAGIC && buf->version == 2 && buf->size == v2_bytes &&
        w[PERSIST_V2_WORDS - 1] == persist_sum_n(buf, PERSIST_V2_WORDS - 1))
        return 1;
    return 0;
}

static void persist_sanitize();
static void profiles_seed();

static void persist_load()
{
    persist_defaults();
    struct persist *a = fio_malloc(sizeof(struct persist));
    struct persist *b = fio_malloc(sizeof(struct persist));
    if (a && b)
    {
        int ka = persist_read(0, a), kb = persist_read(1, b);
        /* la copia v3 col salvataggio piu' recente; il file v2 vale solo se e' l'unico */
        struct persist *best = 0;
        int slot = 0;
        if (ka == 2 && (kb != 2 || a->persist_seq - b->persist_seq >= 0)) { best = a; slot = 0; }
        else if (kb == 2) { best = b; slot = 1; }
        if (best)
        {
            memcpy(&P, best, sizeof(P));
            persist_next_slot = !slot;
        }
        else if (ka == 1)
        {
            /* file della 1.0/1.1: stessa struttura fino ai profili, il resto parte da zero */
            memcpy(&P, a, __builtin_offsetof(struct persist, ml_prev2));
            P.version = PERSIST_VERSION;
            P.size = sizeof(struct persist);
            persist_next_slot = 1;               /* l'originale resta finche' non c'e' una copia nuova */
            persist_touch();
        }
    }
    if (a) fio_free(a);
    if (b) fio_free(b);
    persist_sanitize();
    profiles_seed();
}

static void persist_save()
{
    /* si salva da piu' task (contatore, chiusura di Iride, ML allo spegnimento): uno alla volta */
    if (persist_sem && take_semaphore(persist_sem, 2000) != 0) return;
    struct persist *buf = fio_malloc(sizeof(struct persist));
    if (buf)
    {
        /* prima si azzera il "da salvare", poi si copia: una modifica che arriva durante la
           scrittura resta da salvare. La somma si calcola sulla copia, non su P che puo'
           cambiare nel frattempo (prima un rullino che avanzava poteva rendere il file illeggibile) */
        persist_dirty = 0;
        P.persist_seq++;
        memcpy(buf, &P, sizeof(P));
        buf->magic = PERSIST_MAGIC;
        buf->version = PERSIST_VERSION;
        buf->size = sizeof(struct persist);
        buf->checksum = persist_sum(buf);
        char path[96];
        persist_path(path, sizeof(path), persist_next_slot);
        FILE *f = FIO_CreateFile(path);
        int ok = 0;
        if (f)
        {
            ok = FIO_WriteFile(f, buf, sizeof(struct persist)) == (int)sizeof(struct persist);
            FIO_CloseFile(f);
        }
        if (ok) persist_next_slot = !persist_next_slot;
        else
        {
            /* scheda piena o protetta: si riprova piu' tardi, non ogni secondo */
            persist_dirty = 1;
            persist_touch_ms = get_ms_clock() + 30000;
        }
        fio_free(buf);
    }
    if (persist_sem) give_semaphore(persist_sem);
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

/* valori fuori posto (file di un'altra versione, scheda rovinata ma somma giusta): si rimettono
   nei limiti, cosi' nessun indice esce dalle tabelle */
static void persist_sanitize()
{
    P.roll_exp = COERCE(P.roll_exp, 1, 99);
    P.roll_push = COERCE(P.roll_push, -2, 3);
    P.roll_shots = COERCE(P.roll_shots, 0, 999);
    P.roll_num = MAX(P.roll_num, 0);
    if (P.astro_tag < 0 || P.astro_tag >= NUM_TAGS) P.astro_tag = 0;
    P.dual_rec = COERCE(P.dual_rec, 0, 2);
    P.nd_idx = COERCE(P.nd_idx, 0, 8);
    P.lat100 = COERCE(P.lat100, -9000, 9000);
    P.lon100 = COERCE(P.lon100, -18000, 18000);
    for (int i = 0; i < NPROF; i++) if (P.prof[i].name < 0 || P.prof[i].name >= NPROF_NAMES) P.prof[i].name = i % NPROF_NAMES;
}


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
    MLF_AUTOEXPO, MLF_DEFLICK,
    MLF_DISPGAIN, MLF_SILENT, MLF_SILENT_MODE,          /* dalla 1.2: valori di prima in ml_prev2 */
    MLF_COUNT
};

/* scheda del menu ML e voce: le sottovoci stanno in un menu che ha il nome della voce madre */
static const char *const mlf_menu[MLF_COUNT] = {
    "Shoot", "Advanced Bracket", "Advanced Bracket", "",
    "Focus Stacking", "Focus Stacking", "Focus Stacking",
    "Expo", "Dual ISO", "Expo", "Auto ETTR",
    "Shoot", "Motion Detect", "Motion Detect", "Focus",
    "Overlay", "Overlay", "Overlay", "Overlay", "Overlay", "Overlay", "Overlay",
    "Expo", "Shoot",
    "Display", "Shoot", "Silent Picture",
};
static const char *const mlf_entry[MLF_COUNT] = {
    "Advanced Bracket", "Frames", "Sequence", "",
    "Num. pics in front", "Num. pics behind", "Focus steps / picture",
    "Dual ISO", "Recovery ISO", "Auto ETTR", "Trigger mode",
    "Motion Detect", "Trigger by", "Trigger level", "Trap Focus",
    "Global Draw", "Zebras", "Focus Peak", "Magic Zoom", "Histogram", "False color", "Spotmeter",
    "Auto exposure", "Post Deflicker",
    "LV display gain", "Silent Picture", "Silent Mode",
};

/* dove sta il valore di prima di una funzione (le prime NMLF nella parte v2 del salvataggio) */
static int *ml_prev_slot(int f) { return f < NMLF ? &P.ml_prev[f] : &P.ml_prev2[f - NMLF]; }

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
        *ml_prev_slot(f) = cur;
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
    if (f == MLF_HDR_STEP) hdr_stepsize = *ml_prev_slot(f);
    else menu_set_value_from_script(mlf_menu[f], mlf_entry[f], *ml_prev_slot(f));
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

/* profili di partenza: una base da cui ritoccare (si sovrascrivono con INFO). Il modo e' quello
   consigliato per la ghiera: con la ghiera su un altro modo si caricano stile, ISO e bianco.
   Codici di Magic Lantern (lens.h): ISO 72 = 100, 80 = 200, 112 = 3200, 0 = Auto;
   tempi 96 = 1/30, 117 = 1/200, 120 = 1/250, 136 = 1/1000; diaframmi 32 = f/2.8, 40 = f/4,
   48 = f/5.6, 56 = f/8. Cursori a 0 = quelli di fabbrica dello stile */
static const struct profile prof_presets[] = {
    /* valid name mode          iso  tempo diaf wb       kelvin stile         c  s  t */
    { 1, 0, SHOOTMODE_AV, 80,  117, 32, WB_AUTO, 5200, PS_PORTRAIT,  0, 0, 0 },   /* Ritratto: sfondo sfocato, pelle morbida */
    { 1, 1, SHOOTMODE_AV, 72,  117, 56, WB_AUTO, 5200, PS_LANDSCAPE, 0, 0, 0 },   /* Paesaggio: tutto a fuoco, ISO basso (cavalletto se serve) */
    { 1, 2, SHOOTMODE_AV, 0,   120, 48, WB_AUTO, 5200, PS_STD,       0, 0, 0 },   /* Street: f/5.6 e ISO Auto, sempre pronta */
    { 1, 5, SHOOTMODE_TV, 0,   136, 40, WB_AUTO, 5200, PS_STD,       0, 0, 0 },   /* Sport: 1/1000 per fermare l'azione */
    { 1, 3, SHOOTMODE_M,  112, 96,  40, WB_AUTO, 5200, PS_STD,       0, 0, 0 },   /* Notte in citta' a mano: 1/30, f/4, ISO 3200 */
};

/* la prima volta riempie i posti vuoti con i profili di partenza, senza toccare quelli salvati
   (e senza doppioni dello stesso nome) */
static void profiles_seed()
{
    if (P.persist_prof_seeded) return;
    int k = 0;
    for (int i = 0; i < NPROF; i++)
    {
        if (P.prof[i].valid) continue;
        while (k < (int)COUNT(prof_presets))
        {
            int dup = 0;
            for (int j = 0; j < NPROF; j++) if (P.prof[j].valid && P.prof[j].name == prof_presets[k].name) dup = 1;
            if (!dup) break;
            k++;
        }
        if (k >= (int)COUNT(prof_presets)) break;
        P.prof[i] = prof_presets[k++];
    }
    P.persist_prof_seeded = 1;
    persist_touch();
}

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
static int prof_ap_limited = 0;          /* 1: aperto al massimo dell'obiettivo, 2: chiuso al minimo */
static int profile_load(int i)
{
    prof_ap_limited = 0;
    struct profile *p = &P.prof[i];
    if (!p->valid) return 0;
    snap_take();
    if (valid_picstyle(p->style)) style_set_sliders(p->style, p->c, p->s, p->t);
    if (p->iso == 0 || (p->iso >= 72 && p->iso <= 136)) lens_set_rawiso(p->iso);
    set_wb(p->wb, p->kelvin);
    if (shooting_mode != p->mode) return 0;
    if ((shooting_mode == SHOOTMODE_M || shooting_mode == SHOOTMODE_TV) && p->shutter >= 12) lens_set_rawshutter(p->shutter);
    if ((shooting_mode == SHOOTMODE_M || shooting_mode == SHOOTMODE_AV) && p->aperture)
    {
        /* diaframma che l'obiettivo non ha (f/2.8 su uno zoom f/4, f/22 su un f/16): si usa il
           limite dell'obiettivo, e lo si dice */
        int ap = p->aperture;
        if (lens_info.raw_aperture_min && lens_info.raw_aperture_max > lens_info.raw_aperture_min)
        {
            int lim = COERCE(ap, lens_info.raw_aperture_min, lens_info.raw_aperture_max);
            if (lim != ap) { prof_ap_limited = lim > ap ? 1 : 2; ap = lim; }   /* codice piu' alto = piu' chiuso */
        }
        lens_set_rawaperture(ap);
    }
    return 1;
}
