"""Prova del lettore: python test_iride_session.py"""
import os
import tempfile

from iride_session import abbina_silenziosi, darks_for, find_sessions, load_folder, piano_stack, riepilogo

CSV = """# Iride 6D 1.2 - sessione del 2026-11-02
# luogo 45.10 N 11.90 E, cielo del luogo 21.0 mag/arcsec2, fuso +60 min
# obiettivo EF24-105mm f/4L IS USM
numero,file,tipo,ora,posa_s,iso,diaframma,focale_mm,temperatura_c,batteria,luna_pct,luna_alt,mag_limite,otturatore,origine
1,LGHT0101.CR2,LGHT,22:00:00,20.000,3200,4.0,24,21,80,10,-5.0,6.1,meccanico,intervallometro
2,LGHT0102.CR2,LGHT,22:01:00,20.000,3200,4.0,24,22,79,10,-5.0,6.1,meccanico,cavo
3,DARK0103.CR2,DARK,23:00:00,20.000,3200,4.0,24,20,70,10,-5.0,6.1,meccanico,intervallometro
4,DARK0104.CR2,DARK,23:01:00,20.000,3200,4.0,24,27,70,10,-5.0,6.1,meccanico,intervallometro
5,DARK0105.CR2,DARK,23:02:00,10.000,3200,4.0,24,21,70,10,-5.0,6.1,meccanico,intervallometro
6,silenzioso,LGHT,23:30:00,2.000,1600,4.0,24,23,60,10,-5.0,6.1,silenzioso,intervallometro
"""


def main():
    with tempfile.TemporaryDirectory() as d:
        os.makedirs(os.path.join(d, "DCIM", "100CANON"))
        with open(os.path.join(d, "DCIM", "100CANON", "IS110222.CSV"), "w") as fh:
            fh.write(CSV)
        info = load_folder(d)
        light = info["LGHT0101.CR2"]
        assert light.kind == "LGHT" and light.exposure_s == 20.0 and light.temp_c == 21
        assert info["LGHT0101.JPG"] is light, "RAW+JPEG: stesso scatto"
        assert info["LGHT0102.CR2"].origin == "cavo"
        darks = darks_for(light, info.values())
        assert [x.file for x in darks] == ["DARK0103.CR2"], darks   # 27 gradi troppo lontano, 10 s posa diversa
        sess = find_sessions(d)
        assert sess[0].meta["luogo"] == "45.10 N 11.90 E"
        sil = abbina_silenziosi(["B:/DCIM/100CANON/7_0001.DNG"], sess)
        assert sil["7_0001.DNG"].iso == 1600
        print(riepilogo(sess[0]))
        piano = piano_stack(d)
        g20 = next(g for g in piano if g.exposure_s == 20.0)
        assert [f.file for f in g20.light] == ["LGHT0101.CR2", "LGHT0102.CR2"]
        assert [f.file for f in g20.dark] == ["DARK0103.CR2"]
        assert any("solo 1 dark" in a for a in g20.avvisi) and any("flat" in a for a in g20.avvisi)
        gs = next(g for g in piano if g.exposure_s == 2.0)
        assert not gs.dark and any("silenzios" in a for a in gs.avvisi)
        assert len(g20.migliori(0.5)) == 1
        assert g20.come_dict()["dark"] == ["DARK0103.CR2"]
    print("ok")


if __name__ == "__main__":
    main()
