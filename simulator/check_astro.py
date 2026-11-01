import json, warnings, numpy as np
warnings.filterwarnings("ignore")
from astropy.utils import iers
iers.conf.auto_download = False; iers.conf.auto_max_age = None
from astropy.time import Time
from astropy.coordinates import EarthLocation, AltAz, SkyCoord, FK5, get_sun, get_body
import astropy.units as u
loc = EarthLocation(lat=41.90*u.deg, lon=12.50*u.deg, height=20*u.m)
pol = SkyCoord(ra='02h31m49.09s', dec='+89d15m50.8s', frame=FK5(equinox='J2000'))
gc = SkyCoord(ra='17h45m40.04s', dec='-29d00m28.1s', frame=FK5(equinox='J2000'))
rows = json.load(open('out/astro.json'))
worst = {}
def upd(k, v): worst[k] = max(worst.get(k, 0), abs(v))
print("%-17s %8s %8s | %7s %7s | %7s %7s | %6s %6s | %7s %7s" % ("ora locale","HA C","HA ref","sole C","ref","luna C","ref","ill C","ref","GC C","ref"))
for r in rows:
    t = Time(r['local'] + ':00', scale='utc') - r['utc_offset'] * u.min
    jnow = FK5(equinox=t)
    p = pol.transform_to(jnow)
    last = t.sidereal_time('apparent', longitude=loc.lon)
    ha = ((last - p.ra).wrap_at(24*u.hourangle)).hour % 24
    aa = AltAz(obstime=t, location=loc)
    sun = get_sun(t).transform_to(aa).alt.deg
    moon_b = get_body('moon', t, loc)
    moon = moon_b.transform_to(aa).alt.deg
    sun_b = get_sun(t)
    elong = sun_b.separation(moon_b).rad
    sd = sun_b.distance.km; md = moon_b.distance.km
    i = np.arctan2(sd*np.sin(elong), md - sd*np.cos(elong))
    ill = (1 + np.cos(i)) / 2
    gca = gc.transform_to(aa).alt.deg
    moon_c_topo = r['moon_alt'] - 0.95*np.cos(np.radians(r['moon_alt']))
    dha = (r['polar_ha'] - ha + 12) % 24 - 12
    upd('HA polare (min)', dha*60); upd('sole (gradi)', r['sun_alt']-sun); upd('luna (gradi)', moon_c_topo-moon); upd('illuminazione (%)', (r['moon_illum']-ill)*100); upd('centro VL (gradi)', r['gc_alt']-gca)
    print("%-17s %8.3f %8.3f | %7.2f %7.2f | %7.2f %7.2f | %6.3f %6.3f | %7.2f %7.2f" % (r['local'], r['polar_ha'], ha, r['sun_alt'], sun, moon_c_topo, moon, r['moon_illum'], ill, r['gc_alt'], gca))
print("\nERRORE MASSIMO:", {k: round(v, 3) for k, v in worst.items()})

# notti: tramonto, buio, alba vs astropy (passo 1 minuto)
nights = json.load(open('out/nights.json'))
def local_minutes(t, off):
    dt = (t + off*u.min).datetime
    return dt.hour*60 + dt.minute
errs = []
for n in nights[:3]:
    day = Time('1970-01-01', scale='utc') + n['day1970']*u.day
    off = 60  # novembre: CET
    ts = day + (12*60 - off)*u.min + np.arange(0, 24*60, 1)*u.min
    aa = AltAz(obstime=ts, location=loc)
    s = get_sun(ts).transform_to(aa).alt.deg
    def cross(th, down):
        for k in range(1, len(s)):
            if down and s[k-1] >= th > s[k]: return local_minutes(ts[k], off)
            if not down and s[k-1] < th <= s[k]: return local_minutes(ts[k], off)
    ref = dict(sunset=cross(-0.833, True), dusk=cross(-18, True), dawn=cross(-18, False), sunrise=cross(-0.833, False))
    line = []
    for k in ref:
        d = (n[k] - ref[k] + 720) % 1440 - 720
        errs.append(abs(d)); line.append("%s %02d:%02d vs %02d:%02d" % (k, n[k]//60, n[k]%60, ref[k]//60, ref[k]%60))
    print("notte", n['day1970'], " | ".join(line))
print("errore massimo orari Sole: %d min (passo di calcolo in macchina: 10 min)" % max(errs))

# Luna: sorgere e tramonto (bordo superiore con rifrazione, -0,833 gradi topocentrici) vs astropy
merrs = []
for n in nights[:7]:
    day = Time('1970-01-01', scale='utc') + n['day1970']*u.day
    off = 60
    ts = day + (12*60 - off)*u.min + np.arange(0, 24*60, 1)*u.min
    aa = AltAz(obstime=ts, location=loc)
    m = get_body('moon', ts, loc).transform_to(aa).alt.deg
    rise = next((local_minutes(ts[k], off) for k in range(1, len(m)) if m[k-1] < -0.833 <= m[k]), -1)
    sett = next((local_minutes(ts[k], off) for k in range(1, len(m)) if m[k-1] >= -0.833 > m[k]), -1)
    for name, c, r in (("sorge", n['moonrise'], rise), ("tramonta", n['moonset'], sett)):
        if c < 0 or r < 0:
            print("notte", n['day1970'], name, "C", c, "ref", r); continue
        d = (c - r + 720) % 1440 - 720
        merrs.append(abs(d))
    print("notte", n['day1970'], "Luna sorge %02d:%02d vs %02d:%02d, tramonta %02d:%02d vs %02d:%02d" % (
        n['moonrise']//60, n['moonrise']%60, rise//60, rise%60, n['moonset']//60, n['moonset']%60, sett//60, sett%60))
print("errore massimo orari Luna: %d min" % max(merrs))
