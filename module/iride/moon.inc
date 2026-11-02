/* ==================================================================== Luna
 *
 * La Luna disegnata pixel per pixel, come la si vede dall'emisfero nord
 * (nord in alto, Mare Crisium a destra):
 *  - mari e crateri nelle loro posizioni selenografiche, proiettati sulla sfera
 *    (vicino al bordo si schiacciano da soli);
 *  - luce di Lommel-Seeliger: la Luna piena e' piatta, il bordo illuminato di
 *    una falce e' brillante e la luce cala verso il terminatore;
 *  - rilievo dei crateri e terminatore frastagliato, visibili solo con la luce
 *    radente (alla Luna piena spariscono, come dal vero);
 *  - aloni chiari e raggi di Tycho, Copernico e Keplero, piu' evidenti verso la
 *    Luna piena; luce cinerea sulla parte in ombra; bordo sfumato.
 * Tutti i conti sono interi (Q10: il raggio vale 1024), la 6D non ha unita' in
 * virgola mobile. Il risultato grande resta in una cache finche' non cambia.
 */

#define MQ 1024
#define MDEG 0.0174532925f

enum { MF_MARE, MF_CRATER, MF_BRIGHT };

/* latitudine e longitudine in decimi di grado (est positivo), diametro in km,
   albedo in % di grigio (0 = come intorno), rilievo (0..100), raggi e loro
   lunghezza in centesimi del raggio lunare */
struct mdef { short lat, lon, d; unsigned char kind, alb, dep, rays, rlen; };

static const struct mdef moon_defs[] = {
    /* mari */
    {  328, -156, 1146, MF_MARE, 50, 0, 0, 0 },   /* Imbrium */
    {  280,  175,  674, MF_MARE, 52, 0, 0, 0 },   /* Serenitatis */
    {   85,  314,  873, MF_MARE, 44, 0, 0, 0 },   /* Tranquillitatis */
    {  170,  591,  556, MF_MARE, 45, 0, 0, 0 },   /* Crisium */
    {  -78,  513,  909, MF_MARE, 50, 0, 0, 0 },   /* Fecunditatis */
    { -152,  355,  333, MF_MARE, 51, 0, 0, 0 },   /* Nectaris */
    { -213, -166,  715, MF_MARE, 52, 0, 0, 0 },   /* Nubium */
    { -244, -386,  389, MF_MARE, 49, 0, 0, 0 },   /* Humorum */
    {  133,   36,  245, MF_MARE, 54, 0, 0, 0 },   /* Vaporum */
    {   75, -309,  513, MF_MARE, 50, 0, 0, 0 },   /* Insularum */
    { -100, -231,  376, MF_MARE, 53, 0, 0, 0 },   /* Cognitum */
    {   24,   17,  300, MF_MARE, 61, 0, 0, 0 },   /* Sinus Medii */
    {  109,  -88,  270, MF_MARE, 58, 0, 0, 0 },   /* Sinus Aestuum */
    {  441, -315,  236, MF_MARE, 51, 0, 0, 0 },   /* Sinus Iridum */
    {  380,  290,  384, MF_MARE, 57, 0, 0, 0 },   /* Lacus Somniorum */
    /* Oceanus Procellarum: piu' cerchi per la sua forma irregolare, uniti a Imbrium e Nubium */
    {  300, -560,  700, MF_MARE, 48, 0, 0, 0 },
    {  120, -560,  800, MF_MARE, 47, 0, 0, 0 },
    {  -40, -480,  650, MF_MARE, 48, 0, 0, 0 },
    {  220, -700,  520, MF_MARE, 48, 0, 0, 0 },
    {  -60, -620,  420, MF_MARE, 49, 0, 0, 0 },
    {  220, -400,  560, MF_MARE, 49, 0, 0, 0 },
    {    0, -370,  480, MF_MARE, 51, 0, 0, 0 },
    { -140, -300,  380, MF_MARE, 52, 0, 0, 0 },
    /* Mare Frigoris: una fascia stretta a nord */
    {  590, -400,  250, MF_MARE, 54, 0, 0, 0 },
    {  575, -290,  260, MF_MARE, 54, 0, 0, 0 },
    {  565, -180,  260, MF_MARE, 54, 0, 0, 0 },
    {  560,  -70,  260, MF_MARE, 54, 0, 0, 0 },
    {  555,   40,  260, MF_MARE, 54, 0, 0, 0 },
    {  555,  150,  260, MF_MARE, 55, 0, 0, 0 },
    {  560,  260,  250, MF_MARE, 55, 0, 0, 0 },
    {  570,  370,  230, MF_MARE, 56, 0, 0, 0 },
    /* mari sul bordo est, molto scorciati */
    {   13,  875,  373, MF_MARE, 50, 0, 0, 0 },   /* Smythii */
    {  133,  861,  420, MF_MARE, 50, 0, 0, 0 },   /* Marginis */
    {  568,  815,  273, MF_MARE, 52, 0, 0, 0 },   /* Humboldtianum */
    { -389,  930,  603, MF_MARE, 52, 0, 0, 0 },   /* Australe */
    /* crateri giovani e chiari, con i raggi */
    { -433, -114,   85, MF_BRIGHT, 97, 60, 14, 85 },  /* Tycho */
    {   96, -201,   96, MF_BRIGHT, 88, 60, 10, 45 },  /* Copernico */
    {   81, -380,   31, MF_BRIGHT, 90, 50,  8, 26 },  /* Keplero */
    {  237, -474,   40, MF_BRIGHT, 99, 50,  0,  0 },  /* Aristarco */
    {  161,  468,   28, MF_BRIGHT, 92, 40,  6, 22 },  /* Proclo */
    {  163,  160,   26, MF_BRIGHT, 88, 40,  0,  0 },  /* Menelao */
    /* crateri dal fondo scuro */
    {  516,  -94,  101, MF_CRATER, 42, 40, 0, 0 },    /* Platone */
    {  -52, -686,  173, MF_CRATER, 40, 30, 0, 0 },    /* Grimaldi */
    {  -33, -746,  139, MF_CRATER, 45, 30, 0, 0 },    /* Riccioli */
    {  539,  570,  123, MF_CRATER, 50, 40, 0, 0 },    /* Endimione */
    {  297,  -40,   83, MF_CRATER, 52, 40, 0, 0 },    /* Archimede */
    { -215,  332,  124, MF_CRATER, 54, 30, 0, 0 },    /* Fracastoro */
    { -443, -546,  206, MF_CRATER, 62, 35, 0, 0 },    /* Schickard */
    {  -93,  -19,  154, MF_CRATER, 64, 35, 0, 0 },    /* Tolomeo */
    /* gli altri grandi crateri */
    { -584, -144,  231, MF_CRATER, 0, 55, 0, 0 },     /* Clavius */
    { -134,  -28,  108, MF_CRATER, 0, 55, 0, 0 },     /* Alfonso */
    { -182,  -19,   97, MF_CRATER, 0, 55, 0, 0 },     /* Arzachel */
    { -114,  264,  100, MF_CRATER, 0, 60, 0, 0 },     /* Teofilo */
    { -132,  240,   98, MF_CRATER, 0, 50, 0, 0 },     /* Cirillo */
    { -181,  236,   99, MF_CRATER, 0, 50, 0, 0 },     /* Caterina */
    {  -89,  610,  132, MF_CRATER, 0, 55, 0, 0 },     /* Langreno */
    { -253,  604,  177, MF_CRATER, 0, 55, 0, 0 },     /* Petavio */
    { -164,  616,  147, MF_CRATER, 0, 45, 0, 0 },     /* Vendelino */
    { -360,  604,  125, MF_CRATER, 0, 45, 0, 0 },     /* Furnerio */
    {  276,  555,  125, MF_CRATER, 0, 50, 0, 0 },     /* Cleomede */
    { -500,  -62,  194, MF_CRATER, 0, 45, 0, 0 },     /* Magino */
    { -496, -217,  157, MF_CRATER, 0, 50, 0, 0 },     /* Longomontano */
    { -176, -401,  110, MF_CRATER, 0, 45, 0, 0 },     /* Gassendi */
    {  145, -113,   58, MF_CRATER, 0, 60, 0, 0 },     /* Eratostene */
    {  502,  174,   88, MF_CRATER, 0, 55, 0, 0 },     /* Aristotele */
    {  319,  299,   95, MF_CRATER, 0, 40, 0, 0 },     /* Posidonio */
    { -411,   60,  126, MF_CRATER, 0, 45, 0, 0 },     /* Stoefler */
    { -331,   10,  128, MF_CRATER, 0, 50, 0, 0 },     /* Walther */
    { -255,  -23,  118, MF_CRATER, 0, 45, 0, 0 },     /* Purbach */
    {  -51,   52,  138, MF_CRATER, 0, 35, 0, 0 },     /* Ipparco */
    { -112,   40,  131, MF_CRATER, 0, 50, 0, 0 },     /* Albategnio */
    { -454,  415,  199, MF_CRATER, 0, 35, 0, 0 },     /* Janssen */
    { -420,  140,  114, MF_CRATER, 0, 50, 0, 0 },     /* Maurolico */
    { -297,  322,   88, MF_CRATER, 0, 55, 0, 0 },     /* Piccolomini */
    {  467,  444,   87, MF_CRATER, 0, 55, 0, 0 },     /* Atlante */
    {  467,  391,   69, MF_CRATER, 0, 55, 0, 0 },     /* Ercole */
    { -207, -222,   61, MF_CRATER, 0, 60, 0, 0 },     /* Bullialdo */
    { -299, -135,  106, MF_CRATER, 0, 40, 0, 0 },     /* Pitato */
    {   22, -676,  115, MF_CRATER, 0, 40, 0, 0 },     /* Hevelius */
    { -331,  -52,  256, MF_CRATER, 0, 30, 0, 0 },     /* Deslandres */
    { -668, -691,  303, MF_CRATER, 0, 40, 0, 0 },     /* Bailly */
    { -706,  -55,  114, MF_CRATER, 0, 55, 0, 0 },     /* Moreto */
};

#define MF_MAX 176
#define MR_MAX 48

struct mfeat
{
    short x, y, z, r, reach, rlen;
    int kq;                        /* 2^26 / r^2: (d/r)^2 senza divisioni */
    int kr;                        /* 2^20 / r:   d/r senza divisioni */
    int dk;                        /* pendenza della parete / r, Q10 */
    int reach2, rlen2;
    unsigned char kind, alb, dep, nrays, ray0;
};
struct mray { short nx, ny, nz, dx, dy, dz, len; };

static struct mfeat moon_f[MF_MAX];
static short moon_sq[1025];        /* sqrt(1 - s) in Q10, s in 1024 passi */
static struct mray moon_r[MR_MAX];
static int moon_fn, moon_rn, moon_ready;

static unsigned misqrt(unsigned v)
{
    unsigned r = 0, b = 1u << 30;
    while (b > v) b >>= 2;
    while (b)
    {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
        b >>= 2;
    }
    return r;
}

static unsigned mhash(int x, int y, int seed)
{
    unsigned h = (unsigned)x * 374761393u + (unsigned)y * 668265263u + (unsigned)seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

/* rumore a valori, liscio, 0..1023; u e v in Q10, celle di 2^sh (solo spostamenti) */
static int mnoise(int u, int v, int sh, int seed)
{
    u += 4 * MQ; v += 4 * MQ;
    int gx = u >> sh, gy = v >> sh, m = (1 << sh) - 1;
    int fx = (u & m) << (10 - sh), fy = (v & m) << (10 - sh);
    fx = (fx * fx >> 10) * (3 * MQ - 2 * fx) >> 10;
    fy = (fy * fy >> 10) * (3 * MQ - 2 * fy) >> 10;
    int a = mhash(gx, gy, seed) & 1023, b = mhash(gx + 1, gy, seed) & 1023;
    int c = mhash(gx, gy + 1, seed) & 1023, d = mhash(gx + 1, gy + 1, seed) & 1023;
    int ab = a + ((b - a) * fx >> 10), cd = c + ((d - c) * fx >> 10);
    return ab + ((cd - ab) * fy >> 10);
}

static int moon_add(float lat, float lon, float d_km, int kind, int alb, int dep, int rays, int rlen)
{
    if (moon_fn >= MF_MAX) return -1;
    struct mfeat *f = &moon_f[moon_fn];
    float la = lat * MDEG, lo = lon * MDEG;
    float cx = cosf(la) * sinf(lo), cy = -sinf(la), cz = cosf(la) * cosf(lo);
    f->x = (short)(cx * MQ); f->y = (short)(cy * MQ); f->z = (short)(cz * MQ);
    f->r = (short)MAX(4, d_km * 0.5f / 1737.4f * MQ);
    f->kq = (1 << 26) / (f->r * f->r);
    f->kr = (1 << 20) / f->r;
    f->dk = dep * MQ / 180 * MQ / f->r;
    f->kind = kind; f->alb = alb; f->dep = dep;
    f->rlen = rlen * MQ / 100;
    f->reach = kind == MF_MARE ? f->r * 13 / 10 : kind == MF_BRIGHT ? MAX(2 * f->r, f->rlen) : f->r * 14 / 10;
    f->reach2 = f->reach * f->reach; f->rlen2 = f->rlen * f->rlen;
    f->nrays = 0; f->ray0 = moon_rn;
    /* raggi: archi di cerchio massimo che partono dal cratere */
    if (rays)
    {
        float nx = cy * cx, ny = -1.0f + cy * cy, nz = cy * cz;   /* nord locale */
        float nl = sqrtf(nx * nx + ny * ny + nz * nz);
        if (nl < 1e-3f) nl = 1e-3f;
        nx /= nl; ny /= nl; nz /= nl;
        float ex = cy * nz - cz * ny, ey = cz * nx - cx * nz, ez = cx * ny - cy * nx;  /* est locale */
        for (int j = 0; j < rays && moon_rn < MR_MAX; j++)
        {
            float a = j * 6.2832f / rays + ((int)(mhash(j, moon_fn, 7) & 255) - 128) / 512.0f;
            float dx = cosf(a) * nx + sinf(a) * ex, dy = cosf(a) * ny + sinf(a) * ey, dz = cosf(a) * nz + sinf(a) * ez;
            struct mray *ry = &moon_r[moon_rn++];
            ry->dx = (short)(dx * MQ); ry->dy = (short)(dy * MQ); ry->dz = (short)(dz * MQ);
            ry->nx = (short)((cy * dz - cz * dy) * MQ);
            ry->ny = (short)((cz * dx - cx * dz) * MQ);
            ry->nz = (short)((cx * dy - cy * dx) * MQ);
            ry->len = (short)(f->rlen * (600 + (int)(mhash(j, moon_fn, 9) % 400)) / 1000);
            f->nrays++;
        }
    }
    return moon_fn++;
}

/* dentro un mare? (per non mettere crateri piccoli sui mari) */
static int moon_in_mare(int x, int y, int z)
{
    for (int i = 0; i < moon_fn; i++)
    {
        if (moon_f[i].kind != MF_MARE) continue;
        int dx = x - moon_f[i].x, dy = y - moon_f[i].y, dz = z - moon_f[i].z, r = moon_f[i].r * 9 / 10;
        if (dx * dx + dy * dy + dz * dz < r * r) return 1;
    }
    return 0;
}

static void moon_init()
{
    if (moon_ready) return;
    moon_ready = 1;
    for (int i = 0; i <= 1024; i++) moon_sq[i] = (short)(sqrtf(1.0f - i / 1024.0f) * MQ);
    for (int i = 0; i < (int)COUNT(moon_defs); i++)
    {
        const struct mdef *d = &moon_defs[i];
        moon_add(d->lat / 10.0f, d->lon / 10.0f, d->d, d->kind, d->alb, d->dep, d->rays, d->rlen);
    }
    /* altipiani: crateri minori sparsi, piu' fitti a sud come sulla Luna vera */
    for (int k = 0; k < 600 && moon_fn < MF_MAX; k++)
    {
        unsigned h = mhash(k, 31, 3);
        float lat = -82.0f + (h & 1023) * 150.0f / 1023.0f;
        if (lat > 0.0f && ((h >> 10) & 3) != 0) continue;
        float lon = -86.0f + ((h >> 12) & 1023) * 172.0f / 1023.0f;
        int a = (h >> 22) & 63, b = (h >> 26) & 63;
        float dkm = 26.0f + a * b / 40.0f;
        float la = lat * MDEG, lo = lon * MDEG;
        int x = (int)(cosf(la) * sinf(lo) * MQ), y = (int)(-sinf(la) * MQ), z = (int)(cosf(la) * cosf(lo) * MQ);
        if (moon_in_mare(x, y, z)) continue;
        moon_add(lat, lon, dkm, MF_CRATER, 0, 45 + (int)(h % 25), 0, 0);
    }
}

/* luminosita' percepita della superficie: (2 mu0 / (mu0 + mu)) ^ 0.6 su 0..1,28 */
static const short moon_gamma[33] = {
    0, 148, 225, 287, 341, 390, 435, 477, 517, 555, 591, 626, 659, 692, 723, 754, 783,
    812, 841, 869, 896, 922, 948, 974, 999, 1024, 1048, 1072, 1096, 1119, 1142, 1165, 1187
};
static const unsigned char moon_bayer[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };

/* parametri di un disegno */
struct mview { int r, lx, lz, pf, bg, kpx; const unsigned char *cand; int nc; };

/* colore di un pixel della Luna: 0 = fuori dal disco */
static int moon_px(const struct mview *w, int px, int py)
{
    int r = w->r, bg = w->bg;
    int d2p = px * px + py * py;
    if (d2p > (r + 1) * (r + 1)) return 0;                 /* angoli del quadrato */
    int cov = 256;                                         /* copertura del pixel, Q8 */
    if (d2p > (r - 1) * (r - 1))
    {
        int dq = (int)misqrt((unsigned)d2p << 16);         /* distanza in Q8 */
        cov = COERCE((r * 256 + 128) - dq, 0, 256);
        if (cov <= 0 || (bg < 0 && cov < 128)) return 0;
    }
    int u = px * w->kpx >> 10, v = py * w->kpx >> 10, s = u * u + v * v, nz;
    if (s >= MQ * MQ)
    {
        int sq = (int)misqrt((unsigned)s);
        u = u * MQ / sq; v = v * MQ / sq; nz = 0;
    }
    else
    {
        int i = s >> 10, f = s & 1023;
        nz = moon_sq[i] + ((moon_sq[i + 1] - moon_sq[i]) * f >> 10);
    }

    /* albedo in % di grigio x16: altipiani chiari e screziati */
    int n1 = mnoise(u, v, 8, 1), n2 = mnoise(u, v, 6, 2), n4 = mnoise(u, v, 5, 3);
    int A = 74 * 16 + (n1 - 512) * 10 * 16 / 1024 + (n2 - 512) * 6 * 16 / 1024 + (n4 - 512) * 4 * 16 / 1024;
    int nb = (n1 - 512) + (mnoise(u, v, 7, 6) - 512) / 2 + (n2 - 512) / 4;   /* contorni dei mari */
    /* ben dentro l'ombra contano solo i mari (luce cinerea): niente crateri */
    int deep = ((u * w->lx + nz * w->lz) >> 10) < -MQ * 45 / 100;
    int nx = u, nzz = nz;                      /* il Sole sta nel piano x-z: la componente y non serve */
    int big = r >= 14;
    int ray = 0;
    const unsigned char *cand = w->cand;
    int nc = w->nc;

    /* mari: campi morbidi che si sommano e si fondono tra vicini (metaball),
       poi una soglia mossa dal rumore per i contorni irregolari */
    int mfield = 0, malb = 0;
    int wu = u + (n2 - 512) * 44 / 512, wv = v + (mnoise(u, v, 6, 9) - 512) * 44 / 512;   /* contorni deformati */
    for (int k = 0; k < nc; k++)
    {
        const struct mfeat *f = &moon_f[cand[k]];
        if (f->kind != MF_MARE) continue;
        int dx = wu - f->x;
        if (dx > f->reach || dx < -f->reach) continue;
        int dy = wv - f->y, dz = nz - f->z;
        int d2 = dx * dx + dy * dy + dz * dz;
        if (d2 >= f->reach2) continue;
        int t2 = d2 * f->kq >> 16;                          /* (d/r)^2 in Q10 */
        int g = MQ - t2 * 5 / 8;                            /* 1 - t2/1,6 */
        if (g <= 0) continue;
        g = g * g >> 10;
        mfield += g; malb += g * f->alb;
    }
    if (mfield > 0)
    {
        int am = malb * 16 / mfield + (n1 - 512) * 5 * 16 / 1024 + (n2 - 512) * 3 * 16 / 1024;
        int m = COERCE((mfield + nb * 130 / 512 - 50) * MQ / 320, 0, MQ);
        m = (m * m >> 10) * (3 * MQ - 2 * m) >> 10;
        A += (am - A) * m / MQ;
    }

    /* crateri: fondo, alone, rilievo e raggi (tutto con (d/r)^2, senza radici) */
    for (int k = 0; k < nc && !deep; k++)
    {
        const struct mfeat *f = &moon_f[cand[k]];
        if (f->kind == MF_MARE) continue;
        int dx = u - f->x;
        if (dx > f->reach || dx < -f->reach) continue;
        int dy = v - f->y, dz = nz - f->z;
        int d2 = dx * dx + dy * dy + dz * dz;
        if (d2 >= f->reach2) continue;
        int t2 = d2 < 4 * f->r * f->r ? d2 * f->kq >> 16 : 4 * MQ;   /* (d/r)^2 in Q10 */
        if (f->alb && t2 < MQ)                              /* fondo del cratere */
        {
            int wt = t2 < 655 ? MQ : (MQ - t2) * MQ / (MQ - 655);
            A += (f->alb * 16 - A) * wt / MQ;
        }
        if (f->kind == MF_BRIGHT && t2 >= MQ && t2 < 4 * MQ)    /* alone chiaro */
            A += (f->alb * 16 - A) * (4 * MQ - t2) / (9 * MQ);
        if (big && f->dep && f->r * r >= 900)
        {
            if (t2 < MQ)                                    /* parete interna: guarda verso il centro */
            {
                nx -= dx * f->dk >> 10; nzz -= dz * f->dk >> 10;
            }
            else if (t2 < 2007)                             /* orlo esterno (fino a 1,4 r): guarda fuori */
            {
                int wt = (2007 - t2) * (f->dep * MQ / 180) / 2950;
                nx += (dx * f->kr >> 10) * wt >> 10; nzz += (dz * f->kr >> 10) * wt >> 10;
            }
        }
        if (f->nrays && w->pf > 0 && t2 > MQ && d2 < f->rlen2)
        {
            for (int j = f->ray0; j < f->ray0 + f->nrays; j++)
            {
                const struct mray *ry = &moon_r[j];
                int along = (u * ry->dx + v * ry->dy + nz * ry->dz) >> 10;
                if (along <= 0 || along >= ry->len) continue;
                int off = (u * ry->nx + v * ry->ny + nz * ry->nz) >> 10;
                if (off < 0) off = -off;
                int wr = f->r * 3 / 2 + along / 4;
                if (off >= wr) continue;
                int c = (wr - off) * MQ / wr * (ry->len - along) / ry->len;
                if (c > ray) ray = c;
            }
        }
    }
    if (ray) ray = ray * (256 + mnoise(u, v, 6, 8)) / (256 + MQ);   /* raggi a chiazze, non a righe */
    A += ray * 20 * 16 / MQ * w->pf / MQ;
    A = COERCE(A, 0, 99 * 16);

    /* terminatore frastagliato: piccole irregolarita' del suolo */
    if (big && !deep) nx += (n4 - 512) * (A > 62 * 16 ? 45 : 15) / 512;

    /* Lommel-Seeliger, poi luminosita' percepita */
    int mu0 = (nx * w->lx + nzz * w->lz) >> 10, mu = MAX(nzz, MQ / 5);
    int p = 0;
    if (mu0 > 0)
    {
        int q = mu0 * 2 * MQ / (mu0 + mu);                 /* 0 .. 2 in Q10 */
        int i = q * 25 >> 10;                              /* tabella: passo 0,04 */
        int g = i >= 32 ? moon_gamma[32] : moon_gamma[i] + ((moon_gamma[i + 1] - moon_gamma[i]) * (q * 25 & (MQ - 1)) >> 10);
        if (r <= 12 && g < 900) g = 900;                    /* icone: parte illuminata netta */
        p = A * g >> 10;
    }
    int earth = r > 12 ? 7 * 16 + A / 13 : 17 * 16 + A / 12;    /* luce cinerea, piu' chiara nelle icone */
    if (p < earth) p = earth;

    if (night) return p >= 30 * 16 ? COLOR_RED : COLOR_DARK_RED;
    if (bg >= 0 && cov < 256) p = bg * 16 + ((p - bg * 16) * cov >> 8);
    int b = r > 12 ? moon_bayer[((py + 64) & 3) * 4 + ((px + 64) & 3)] : 8;   /* niente retino nelle icone */
    int lvl = (p * 41 + b * 100 + 50) / 1600;
    return COLOR_GRAY(0) + COERCE(lvl, 0, 41);
}

/* cache dei disegni: una grande (Luna del calendario) e otto piccole (le righe) */
#define MC_R 46
#define MC_SMALL 10
#define MC_NS 8
struct mslot { int key[6]; int ok; };
static unsigned char mc_big[(2 * MC_R + 1) * (2 * MC_R + 1)];
static unsigned char mc_small[MC_NS][(2 * MC_SMALL + 1) * (2 * MC_SMALL + 1)];
static struct mslot ms_big, ms_small[MC_NS];
static int ms_next;

/* r in pixel; illum in millesimi; bg = grigio dello sfondo in % per sfumare il
   bordo, oppure -1 per un bordo netto (sfondi che cambiano) */
static void draw_moon(int cx, int cy, int r, int illum, int waxing, int bg)
{
    if (r < 3) return;
    moon_init();
    illum = COERCE(illum, 0, 1000);
    int south = P.lat100 < 0;                              /* dall'emisfero sud la Luna e' capovolta */
    int key[6] = { r, illum, waxing, night, bg, south };
    int side = 2 * r + 1, hit = 0;
    unsigned char *buf = 0;
    struct mslot *slot = 0;
    if (r <= MC_SMALL)
    {
        for (int i = 0; i < MC_NS && !slot; i++)
        {
            int same = ms_small[i].ok;
            for (int k = 0; k < 6; k++) if (ms_small[i].key[k] != key[k]) same = 0;
            if (same) { slot = &ms_small[i]; buf = mc_small[i]; hit = 1; }
        }
        if (!slot) { slot = &ms_small[ms_next]; buf = mc_small[ms_next]; ms_next = (ms_next + 1) % MC_NS; }
    }
    else if (r <= MC_R)
    {
        slot = &ms_big; buf = mc_big; hit = ms_big.ok;
        for (int k = 0; k < 6; k++) if (ms_big.key[k] != key[k]) hit = 0;
    }

    /* direzione del Sole: angolo di fase dalla frazione illuminata */
    struct mview w;
    w.r = r; w.bg = bg; w.kpx = MQ * MQ / r; w.nc = 0;
    w.lz = (2 * illum - 1000) * MQ / 1000;
    w.lx = (int)misqrt((unsigned)(illum * (1000 - illum))) * 2 * MQ / 1000;
    if (!waxing) w.lx = -w.lx;
    w.pf = illum > 500 ? (illum - 500) * MQ / 500 : 0;      /* raggi: verso la piena */

    unsigned char cand[MF_MAX];
    w.cand = cand;
    for (int py = -r; py <= r; py++)
    {
        unsigned char *row = buf ? &buf[(py + r) * side] : 0;
        if (!hit)
        {
            int v = (south ? -py : py) * w.kpx >> 10, nc = 0;
            for (int k = 0; k < moon_fn; k++)
            {
                int dv = v - moon_f[k].y;
                if (dv <= moon_f[k].reach && dv >= -moon_f[k].reach) cand[nc++] = k;
            }
            w.nc = nc;
        }
        int run_c = 0, run_x = 0;
        for (int px = -r; px <= r + 1; px++)
        {
            int c = 0;
            if (px <= r)
            {
                if (hit) c = row[px + r];
                else
                {
                    c = south ? moon_px(&w, -px, -py) : moon_px(&w, px, py);
                    if (row) row[px + r] = c;
                }
            }
            if (c != run_c)
            {
                if (run_c) fill(run_c, cx + run_x, cy + py, px - run_x, 1);
                run_c = c; run_x = px;
            }
        }
    }
    if (slot && !hit) { for (int k = 0; k < 6; k++) slot->key[k] = key[k]; slot->ok = 1; }
}
