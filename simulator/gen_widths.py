from PIL import ImageFont
# ML font ids -> (pixel height of the line, ttf size)
fonts = {0:(12,11), 1:(20,17), 2:(23,19), 3:(28,23), 4:(32,27)}
out = ["/* generated */", "static const int sim_font_h[8] = {12,20,23,28,32,0,0,0};"]
out.append("static const unsigned char sim_w[8][128] = {")
for fid in range(8):
    if fid in fonts:
        h, size = fonts[fid]
        path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf" if fid < 2 else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
        f = ImageFont.truetype(path, size)
        w = [int(round(f.getlength(chr(c)))) if c >= 32 else 0 for c in range(128)]
    else:
        w = [0]*128
    out.append("  {" + ",".join(map(str, w)) + "},")
out.append("};")
open("shim/widths.h","w").write("\n".join(out)+"\n")
