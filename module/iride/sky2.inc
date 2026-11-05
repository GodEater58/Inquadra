/* sky2.inc - Luna da fotografare e Dove puntare la Via Lattea */

/* altezza e azimut (da nord verso est) in gradi */
static void alt_az(float ra, float dec, float lst, float lat, float *alt, float *az)
{
    float h = (lst - ra) * DEG2RAD, d = dec * DEG2RAD, p = lat * DEG2RAD;
    float sa = sinf(p) * sinf(d) + cosf(p) * cosf(d) * cosf(h);
    *alt = asinf(COERCE(sa, -1.0f, 1.0f)) * RAD2DEG;
    float a = atan2f(-cosf(d) * sinf(h), sinf(d) * cosf(p) - cosf(d) * cosf(h) * sinf(p)) * RAD2DEG;
    *az = a < 0.0f ? a + 360.0f : a;
}

/* direzione a 16 punti, all'italiana */
static const char *compass16(float az)
{
    static const char *const n[16] = { "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                                       "S", "SSO", "SO", "OSO", "O", "ONO", "NO", "NNO" };
    return n[((int)(az / 22.5f + 0.5f)) & 15];
}

/* massa d'aria (Kasten e Young), anche vicino all'orizzonte */
static float airmass(float alt)
{
    if (alt < 0.0f) alt = 0.0f;
    return 1.0f / (sinf(alt * DEG2RAD) + 0.50572f * powf(alt + 6.07995f, -1.6364f));
}

/* ================================================================== Luna da fotografare */

static CONFIG_INT("iride.moon.iso", opt_moon_iso, 1);     /* indice in moon_isos */
static CONFIG_INT("iride.moon.ap",  opt_moon_ap, 110);    /* diaframma x10 */
static CONFIG_INT("iride.moon.es",  opt_moon_es, 0);      /* 0 Luna, 1 luce cinerea */
static const int moon_isos[] = { 100, 200, 400, 800, 1600 };
static const int moon_aps[] = { 28, 40, 56, 63, 71, 80, 90, 100, 110, 130, 160 };

struct moonnow { float alt, az; int illum, waxing, rise, set; };   /* rise/set: minuti locali, -1 se no */
static struct moonnow mn;

/* dove e' la Luna adesso e quando sorge e tramonta nelle prossime 24 ore */
static void moon_now_compute()
{
    int d70, mins, off;
    struct jtime j = jt_now(&d70, &mins, &off);
    float lat = lat_deg(), lon = lon_deg(), ra, dec;
    moon_pos(j, &ra, &dec, 0);
    alt_az(ra, dec, lst_deg(j, lon), lat, &mn.alt, &mn.az);
    mn.alt -= 0.95f * cosf(mn.alt * DEG2RAD);              /* parallasse */
    mn.illum = moon_illum(j, &mn.waxing);
    mn.rise = mn.set = -1;
    float prev = 0.0f;
    for (int k = 0; k <= 144; k++)                          /* ogni 10 minuti per 24 ore */
    {
        int m = mins + k * 10;
        struct jtime jk = jt_from_local(d70, m, off);
        moon_pos(jk, &ra, &dec, 0);
        float a = altitude(ra, dec, lst_deg(jk, lon), lat);
        a -= 0.95f * cosf(a * DEG2RAD);
        if (k > 0)
        {
            int t = MOD(m - 10 + (int)(10.0f * (prev + 0.833f) / (prev - a) + 0.5f), 1440);
            if (prev < -0.833f && a >= -0.833f && mn.rise < 0) mn.rise = t;
            if (prev >= -0.833f && a < -0.833f && mn.set < 0) mn.set = t;
        }
        prev = a;
    }
}

/* stop in piu' rispetto alla Luna piena alta: fase (un stop ogni ottavo circa, Looney 11)
   e assorbimento dell'atmosfera rispetto a 60 gradi di altezza */
static float moon_phase_stops(int illum)
{
    static const short k[5] = { 1000, 750, 500, 250, 80 };   /* piena, gibbosa, quarto, falce larga, falce sottile */
    if (illum >= k[0]) return 0.0f;
    for (int i = 1; i < 5; i++)
        if (illum >= k[i]) return (float)(i - 1) + (float)(k[i - 1] - illum) / (float)(k[i - 1] - k[i]);
    return 4.0f;
}

static float moon_alt_stops(float alt)
{
    float d = 0.20f * (airmass(MAX(alt, 3.0f)) - airmass(60.0f));   /* magnitudini */
    return MAX(d, 0.0f) / 0.753f;                                  /* 1 stop = 0,753 mag */
}

/* tempo in secondi per ISO e diaframma scelti */
static float moon_exposure(float *phase_st, float *alt_st)
{
    int iso = moon_isos[COERCE(opt_moon_iso, 0, (int)COUNT(moon_isos) - 1)];
    float n = (float)opt_moon_ap / 10.0f;
    float a = mn.alt > 0.0f ? mn.alt : 30.0f;                 /* sotto l'orizzonte: come quando sara' a 30 gradi */
    float sa = moon_alt_stops(a);
    if (alt_st) *alt_st = sa;
    if (opt_moon_es)
    {
        /* luce cinerea: 2 s a f/2.8 e ISO 250 per una falce (astropix) */
        if (phase_st) *phase_st = 0.0f;
        return 2.0f * (n / 2.8f) * (n / 2.8f) * 250.0f / (float)iso * powf(2.0f, sa);
    }
    float sp = moon_phase_stops(mn.illum);
    if (phase_st) *phase_st = sp;
    return (1.0f / (float)iso) * (n / 11.0f) * (n / 11.0f) * powf(2.0f, sp + sa);
}

/* codice Canon del tempo a terzi di stop piu' vicino */
static int shutter_code_near(float t)
{
    int raw = (int)(56.0f - log2f(MAX(t, 0.0001f)) * 8.0f + 0.5f);
    raw = COERCE(raw, 16, 152);
    for (int d = 0; d < 3; d++)
    {
        if (raw + d <= 152 && ((raw + d) % 8 == 0 || (raw + d) % 8 == 3 || (raw + d) % 8 == 5)) return raw + d;
        if (raw - d >= 16 && ((raw - d) % 8 == 0 || (raw - d) % 8 == 3 || (raw - d) % 8 == 5)) return raw - d;
    }
    return raw;
}

static void moonshot_enter() { moon_now_compute(); }

static int moonshot_tick()
{
    static int last = 0;
    int now = get_ms_clock();
    if (now - last < 60000) return 0;                        /* la Luna si muove: ogni minuto */
    last = now;
    moon_now_compute();
    return 1;
}

static const char *const moon_labels[] = { "Soggetto", "ISO", "Diaframma", "Applica (ghiera su M)" };

/* diaframma senza ".0" inutili: f/11, f/5.6 */
static void fmt_fnum(char *b, int len, int ap10)
{
    if (ap10 % 10 == 0) snprintf(b, len, "%d", ap10 / 10);
    else snprintf(b, len, "%d.%d", ap10 / 10, ap10 % 10);
}

static void moon_value(int row, char *buf, int len, int *kind)
{
    char a[12];
    switch (row)
    {
        case 0: snprintf(buf, len, "< %s >", opt_moon_es ? "luce cinerea" : "superficie della Luna"); break;
        case 1: snprintf(buf, len, "< %d >", moon_isos[COERCE(opt_moon_iso, 0, (int)COUNT(moon_isos) - 1)]); break;
        case 2: fmt_fnum(a, sizeof(a), opt_moon_ap); snprintf(buf, len, "< f/%s >", a); break;
        default: *kind = VK_ACTION; break;
    }
}

static void moon_change(int row, int dir, int big)
{
    if (row == 0) opt_moon_es = !opt_moon_es;
    else if (row == 1) opt_moon_iso = COERCE(opt_moon_iso + dir, 0, (int)COUNT(moon_isos) - 1);
    else if (row == 2) opt_moon_ap = step_in(moon_aps, COUNT(moon_aps), opt_moon_ap, dir);
}

static void moon_apply()
{
    if (shooting_mode != SHOOTMODE_M) { say(C_WARN, "Metti la ghiera su M e riprova"); return; }
    snap_take();
    float t = moon_exposure(0, 0);
    int raw_sh = shutter_code_near(t), ok = 1;
    if (lens_info.raw_aperture_min)
    {
        int ra = val2raw_aperture(opt_moon_ap);
        if (ra > 0) ok &= lens_set_rawaperture(ra);
    }
    ok &= lens_set_rawshutter(raw_sh);
    int raw_iso = val2raw_iso(moon_isos[COERCE(opt_moon_iso, 0, (int)COUNT(moon_isos) - 1)]);
    if (raw_iso > 0) ok &= lens_set_rawiso(raw_iso);
    lens_set_kelvin(opt_moon_es ? 3900 : 5200);
    char buf[96];
    snprintf(buf, sizeof(buf), "%s %s  %s  ISO %s", ok ? "Applicato:" : "In parte:",
        lens_format_shutter(raw_sh), lens_format_aperture(lens_info.raw_aperture), lens_format_iso(lens_info.raw_iso));
    say(ok ? C_OK : C_WARN, buf);
}

static void moon_activate(int row)
{
    if (row < 3) { moon_change(row, 1, 0); return; }
    moon_apply();
}

static void moon_extra()
{
    char buf[96], a[8], b[8];
    /* la Luna di adesso, con fase e orientamento giusti */
    int r = 80, cx = 112, cy = 152;
    fill(C_BG, cx - r - 4, cy - r - 4, 2 * r + 8, 2 * r + 8);
    draw_moon(cx, cy, r, mn.illum, mn.waxing, 0);

    int x = 222;
    snprintf(buf, sizeof(buf), "%s %d%%", moon_phase_name(mn.illum, mn.waxing), (mn.illum + 5) / 10);
    txt(FONT_MED_LARGE, C_TEXT, x, 58, buf);
    if (mn.alt > 0.0f)
        snprintf(buf, sizeof(buf), "Adesso a %s, alta %d gradi", compass16(mn.az), (int)(mn.alt + 0.5f));
    else snprintf(buf, sizeof(buf), "Adesso sotto l'orizzonte (%s)", compass16(mn.az));
    txt(FONT_MED, mn.alt > 0.0f ? C_TEXT : C_DIM, x, 90, buf);
    a[0] = b[0] = 0;
    if (mn.rise >= 0) fmt_hm(a, sizeof(a), mn.rise);
    if (mn.set >= 0) fmt_hm(b, sizeof(b), mn.set);
    if (a[0] && b[0]) snprintf(buf, sizeof(buf), "Sorge alle %s, tramonta alle %s", a, b);
    else if (a[0]) snprintf(buf, sizeof(buf), "Sorge alle %s", a);
    else if (b[0]) snprintf(buf, sizeof(buf), "Tramonta alle %s", b);
    else snprintf(buf, sizeof(buf), "%s", mn.alt > 0.0f ? "In cielo per tutte le prossime 24 ore" : "Non sorge nelle prossime 24 ore");
    txt(FONT_MED, C_DIM, x, 114, buf);

    /* l'esposizione */
    float sp, sa;
    float t = moon_exposure(&sp, &sa);
    int raw = shutter_code_near(t);
    info_card(x, 144, SCR_W - 24 - x, 92, opt_moon_es ? "ESPOSIZIONE PER LA LUCE CINEREA" : "ESPOSIZIONE (REGOLA LOONEY 11)");
    snprintf(buf, sizeof(buf), "%s", lens_format_shutter(raw));
    txt(FONT_LARGE, C_ACCENT, x + 16, 162, buf);
    char ap[12];
    fmt_fnum(ap, sizeof(ap), opt_moon_ap);
    snprintf(buf, sizeof(buf), "f/%s  ISO %d", ap, moon_isos[COERCE(opt_moon_iso, 0, (int)COUNT(moon_isos) - 1)]);
    txt(FONT_MED, C_TEXT, x + 24 + txt_w(FONT_LARGE, lens_format_shutter(raw)), 170, buf);
    int sp10 = (int)(sp * 10.0f + 0.5f), sa10 = (int)(sa * 10.0f + 0.5f);
    if (t > 31.0f) txt(FONT_SMALL, C_WARN, x + 16, 194, "Oltre 30 s: apri il diaframma o alza gli ISO");
    if (opt_moon_es) snprintf(buf, sizeof(buf), "base 2 s a f/2.8 e ISO 250; altezza +%d,%d stop", sa10 / 10, sa10 % 10);
    else snprintf(buf, sizeof(buf), "f/11 e 1/ISO; fase +%d,%d stop, altezza +%d,%d stop", sp10 / 10, sp10 % 10, sa10 / 10, sa10 % 10);
    txt(FONT_SMALL, C_DIM, x + 16, 210, buf);
}

static const struct form moon_form = { 4, moon_labels, moon_value, moon_change, moon_activate, moon_extra,
    "^/v riga   </> cambia   SET applica   INFO aiuto   MENU indietro", 262, 44, 0, 0, 0 };

/* ================================================================== Dove puntare la Via Lattea */

/* mw_night (notte del calendario), mw_back_sky (aperta dal calendario: MENU torna li')
   e sky_keep sono dichiarate in astro.inc, accanto al calendario */
static int mw_slot = 0;                  /* mezz'ora scelta, dalle 16 */
#define MW_SLOTS 33                      /* dalle 16:00 alle 8:00, ogni 30 minuti */

struct mwpoint { float alt, az; };
static struct mwpoint mw_gc[MW_SLOTS];
static int mw_day = -1;                  /* giorno calcolato */

/* punto del piano galattico (b = 0) alla longitudine l, in coordinate equatoriali di adesso */
static void gal_to_eq(float l, float *ra, float *dec)
{
    const float a_ngp = 192.85948f * DEG2RAD, d_ngp = 27.12825f * DEG2RAD, l_ncp = 122.93192f;
    float x = (l_ncp - l) * DEG2RAD;
    float sd = cosf(d_ngp) * cosf(x);
    *dec = asinf(COERCE(sd, -1.0f, 1.0f)) * RAD2DEG;
    float r = (a_ngp + atan2f(sinf(x), -sinf(d_ngp) * cosf(x))) * RAD2DEG;
    /* J2000 -> oggi: lo stesso spostamento del centro galattico, basta per disegnare la fascia */
    r += GC_RA - 266.40499f;
    *dec += GC_DEC - (-28.93617f);
    *ra = r < 0.0f ? r + 360.0f : r >= 360.0f ? r - 360.0f : r;
}

static struct jtime mw_time(int day1970, int slot)
{
    int mins = 960 + slot * 30;
    int y, m, d;
    civil_from_days(day1970, &y, &m, &d);
    return jt_from_local(day1970, mins, utc_offset_min(y, m, d, (mins / 60) % 24));
}

static void mw_compute()
{
    int d70, mins, off;
    jt_now(&d70, &mins, &off);
    if (mins < 720) d70 -= 1;
    if (!mw_back_sky)
    {
        /* dal menu Astro: stanotte (il calendario si ricalcola comunque quando lo apri) */
        mw_night = 0;
        if (!sky_ready || nights[0].day1970 != d70) { sky_ready = 0; sky_compute_night(0, d70); }
    }
    int day = nights[COERCE(mw_night, 0, SKY_NIGHTS - 1)].day1970;
    mw_day = day;
    for (int i = 0; i < MW_SLOTS; i++)
    {
        struct jtime j = mw_time(day, i);
        alt_az(GC_RA, GC_DEC, lst_deg(j, lon_deg()), lat_deg(), &mw_gc[i].alt, &mw_gc[i].az);
    }
}

static void mwplan_enter()
{
    mw_compute();
    /* parte dalla mezz'ora migliore, o da quando il centro e' piu' alto col buio */
    const struct night *n = &nights[COERCE(mw_night, 0, SKY_NIGHTS - 1)];
    int best = -1;
    for (int i = 0; i < MW_SLOTS - 1; i++)
    {
        int v = n->s[MIN(i * 3, SKY_SAMPLES - 1)];
        if ((v & SK_SUN) != SK_NIGHT) continue;
        if (best < 0 || mw_gc[i].alt > mw_gc[best].alt) best = i;
    }
    mw_slot = best >= 0 ? best : 16;
    /* stanotte il centro non c'e' col buio: si cerca quando torna (a pezzi, nel tick) */
    if (n->gc_start < 0 && gcn_state != 2)
    {
        int any = 0;
        for (int k = 0; k < SKY_SAMPLES; k++) if ((n->s[k] & SK_GC) && (n->s[k] & SK_SUN) == SK_NIGHT) any = 1;
        if (!any) { gcn_state = 1; gcn_from = gcn_day = n->day1970 + 1; }
    }
}

static int mwplan_tick()
{
    if (gcn_state != 1) return 0;
    gc_scan_step();
    return 1;
}

/* il campione del calendario (10 minuti) che corrisponde alla mezz'ora */
static int mw_sample(int slot) { return nights[COERCE(mw_night, 0, SKY_NIGHTS - 1)].s[MIN(slot * 3, SKY_SAMPLES - 1)]; }

static void mwplan_key(int key, int dx, int dy)
{
    if (dx) mw_slot = COERCE(mw_slot + dx, 0, MW_SLOTS - 1);
    if (dy && sky_ready)
    {
        mw_night = COERCE(mw_night + dy, 0, SKY_NIGHTS - 1);
        mw_compute();
    }
}

static void draw_mwplan()
{
    fill(C_BG, 0, 46, SCR_W, 398);
    draw_header("Dove puntare");
    char buf[112], a[8], b[16];
    const struct night *n = &nights[COERCE(mw_night, 0, SKY_NIGHTS - 1)];

    /* il cielo verso sud: azimut da est (90) a ovest (270), altezza da 0 a 45 gradi */
    const int px = 24, py = 84, pw = SCR_W - 48, ph = 236;
    const float az0 = 90.0f, az1 = 270.0f, altmax = 45.0f;
    #define MWX(az) (px + (int)(((az) - az0) * (float)pw / (az1 - az0)))
    #define MWY(alt) (py + ph - (int)((alt) * (float)ph / altmax))
    fill_r(night ? COLOR_BLACK : COLOR_GRAY(4), px, py, pw, ph, R_ROW);
    for (int g = 10; g <= 40; g += 10)
    {
        fill(C_PANEL2, px, MWY((float)g), pw, 1);
        snprintf(buf, sizeof(buf), "%d", g);
        txt(FONT_SMALL, C_DIM, px + 4, MWY((float)g) - 13, buf);
    }
    static const char *const card[] = { "E", "SE", "S", "SO", "O" };
    for (int i = 0; i < 5; i++)
    {
        int x = MWX(90.0f + i * 45.0f);
        fill(C_LINE, x, py, 1, ph);
        txt_c(FONT_SMALL, C_DIM, x, py + ph + 4, card[i]);
    }
    /* orizzonte */
    fill(C_LINE, px, py + ph - 1, pw, 2);

    /* la Via Lattea all'ora scelta: il piano galattico disegnato sul cielo */
    struct jtime js = mw_time(mw_day, mw_slot);
    float lst = lst_deg(js, lon_deg()), lat = lat_deg();
    int lx = -1, ly = -1;
    float tilt = -1000.0f, a0 = 0.0f, z0 = 0.0f;
    for (int l = -80; l <= 80; l += 4)
    {
        float ra, dec, al, az;
        gal_to_eq((float)l, &ra, &dec);
        alt_az(ra, dec, lst, lat, &al, &az);
        int ok = al > -2.0f && az >= az0 && az <= az1 && al <= altmax;
        int x = MWX(az), y = MWY(al);
        if (ok && lx >= 0 && ABS(x - lx) < pw / 3)
        {
            int c = night ? COLOR_DARK_RED : COLOR_GRAY(30);
            for (int w = -3; w <= 3; w++) { draw_line(lx, ly + w, x, y + w, c); draw_line(lx + w, ly, x + w, y, c); }
        }
        if (l == -4 || l == 4)
        {
            /* inclinazione dell'arco vicino al centro, rispetto all'orizzonte */
            if (l == -4) { a0 = al; z0 = az; }
            else
            {
                float dz = (az - z0) * cosf((al + a0) * 0.5f * DEG2RAD);
                tilt = atan2f(al - a0, dz < 0.0f ? -dz : dz) * RAD2DEG;
                if (tilt < 0.0f) tilt = -tilt;
            }
        }
        lx = ok ? x : -1; ly = y;
    }

    /* il centro della Via Lattea ogni mezz'ora: verde quando si fotografa */
    for (int i = 0; i < MW_SLOTS; i++)
    {
        float al = mw_gc[i].alt, az = mw_gc[i].az;
        if (al < 0.0f || az < az0 || az > az1) continue;
        int v = mw_sample(i);
        int dark = (v & SK_SUN) == SK_NIGHT, moon = (v & SK_MOON) != 0;
        int c = dark && !moon && al > 10.0f ? C_OK : dark ? C_DIM : C_LINE;
        int x = MWX(az), y = MWY(al);
        disc(x, y, i == mw_slot ? 6 : 3, c);
        if (i % 2 == 0 && i != mw_slot)
        {
            snprintf(buf, sizeof(buf), "%d", (16 + i / 2) % 24);
            txt_c(FONT_SMALL, C_DIM, x, y - 18, buf);
        }
    }
    /* l'ora scelta */
    {
        float al = mw_gc[mw_slot].alt, az = mw_gc[mw_slot].az;
        int mins = (960 + mw_slot * 30) % 1440;
        fmt_hm(a, sizeof(a), mins);
        if (al > 0.0f && az >= az0 && az <= az1)
        {
            int x = MWX(az), y = MWY(al);
            ring_r(C_ACCENT, x - 9, y - 9, 19, 19, 5, 2);
            txt_c(FONT_SMALL, C_ACCENT, x, y - 26, a);
        }
    }
    #undef MWX
    #undef MWY

    /* le parole */
    fmt_day(b, sizeof(b), n->day1970);
    snprintf(buf, sizeof(buf), "Notte di %s", b);
    txt(FONT_SMALL, C_ACCENT, 24, 52, buf);
    float al = mw_gc[mw_slot].alt, az = mw_gc[mw_slot].az;
    if (al > 0.0f)
    {
        if (tilt > -999.0f)
            snprintf(buf, sizeof(buf), "Alle %s: verso %s (%d), alto %d gradi, arco a %d gradi",
                     a, compass16(az), (int)(az + 0.5f), (int)(al + 0.5f), (int)(tilt + 0.5f));
        else snprintf(buf, sizeof(buf), "Alle %s: verso %s (%d), alto %d gradi", a, compass16(az), (int)(az + 0.5f), (int)(al + 0.5f));
    }
    else snprintf(buf, sizeof(buf), "Alle %s il centro della Via Lattea e' sotto l'orizzonte", a);
    txt(FONT_MED, C_TEXT, 24, 340, buf);
    if (n->gc_start >= 0)
    {
        fmt_hm(a, sizeof(a), n->gc_start); fmt_hm(b, sizeof(b), n->gc_end);
        snprintf(buf, sizeof(buf), "Da fotografare: %s - %s, col buio e senza Luna", a, b);
        txt(FONT_MED, C_OK, 24, 366, buf);
    }
    else
    {
        gc_missing_line(0, buf, sizeof(buf));
        txt(FONT_MED, C_DIM, 24, 366, buf);
    }
    txt(FONT_SMALL, C_DIM, 24, 396, "Verde: buio senza Luna e centro sopra 10 gradi. Grigio: la Via Lattea all'ora scelta.");
    draw_footer(sky_ready ? "</> ora   ^/v notte   INFO aiuto   MENU indietro" : "</> ora   INFO aiuto   MENU indietro");
}
