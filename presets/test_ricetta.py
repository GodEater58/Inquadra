"""Prova delle ricette fatte sulla 6D: python3 presets/test_ricetta.py"""
import glob
import os
import sys
import tempfile
import xml.dom.minidom

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_presets import read_card, recipe_film, recipe_main  # noqa: E402

CARD = ("IRIDE RICETTA\r\nnome: MIA 2\r\nbase: CINESTILL 800T\r\niso: 1600\r\nstile: Monocromo\r\n"
        "contrasto: -2\r\nsaturazione: 3\r\ntonalita: -1\r\nkelvin: 3600\r\ngrana: 3\r\nvignetta: 2\r\nscatola: 4\r\n")


def main():
    with tempfile.TemporaryDirectory() as d:
        p = os.path.join(d, "MIA2.TXT")
        with open(p, "w", newline="") as fh:
            fh.write(CARD)
        card = read_card(p)
        f = recipe_film(card)
        assert f["use"] == "partita da CineStill 800T", f["use"]
        assert f["grain"] == (50, 40, 60) and f["vignette"] == -20
        assert f["bw"] is True and f["iso"] == 1600
        recipe_main([p], os.path.join(d, "out"))
        files = glob.glob(os.path.join(d, "out", "*.xmp"))
        assert len(files) == 1, files
        xml.dom.minidom.parse(files[0])
        txt = open(files[0], encoding="utf-8").read()
        assert 'crs:ConvertToGrayscale="True"' in txt and 'crs:GrainAmount="50"' in txt
        bad = os.path.join(d, "X.TXT")
        with open(bad, "w") as fh:
            fh.write("ciao\n")
        try:
            read_card(bad)
            raise AssertionError("file sbagliato accettato")
        except ValueError:
            pass
        # pellicola di partenza col nome piu' lungo: "Portra 400" non diventa "Portra 4..."
        assert recipe_film({"base": "KODAK Portra 400"})["use"].endswith("Portra 400")
    print("ricette: ok")


if __name__ == "__main__":
    main()
