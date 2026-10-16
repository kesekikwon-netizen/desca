# 발굴 단면뷰어 아이콘(C안): 남색 스퀴클 + 수혈이 파인 흰 지면 + 호박색 레벨 표식. 32 px 이하는 굵은 단순화판
import io, struct, os
from PIL import Image, ImageDraw
SS = 4; N = 1024 * SS  # 초과 표본
NAVY, GROUND, AMBER = (22, 52, 84, 255), (246, 244, 238, 255), (245, 176, 65, 255)
P = lambda pts: [(x * SS, y * SS) for x, y in pts]
def master(small):
    im = Image.new('RGBA', (N, N), NAVY); d = ImageDraw.Draw(im)
    d.polygon(P([(0, 470), (300, 470), (360, 700), (664, 700), (724, 470), (1024, 470), (1024, 1024), (0, 1024)]), fill=GROUND)
    y, t = 330, (110 if small else 70)
    d.line(P([(150, y), (800, y)]), fill=AMBER, width=(70 if small else 30) * SS)
    d.polygon(P([(874 - t, y - t * 0.8), (874 + t * 0.2, y - t * 0.8), (874 - t * 0.4, y + t * 0.1)]), fill=AMBER)
    m = Image.new('L', (N, N), 0); R = int(N * 0.225)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, N - 1, N - 1], radius=R, fill=255)
    out = Image.new('RGBA', (N, N), (0, 0, 0, 0)); out.paste(im, (0, 0), m); od = ImageDraw.Draw(out)
    od.rounded_rectangle([0, 0, N - 1, N - 1], radius=R, outline=(0, 0, 0, 34), width=5 * SS)   # 밝은 바탕 가장자리
    od.rounded_rectangle([5 * SS, 5 * SS, N - 1 - 5 * SS, N - 1 - 5 * SS], radius=R - 5 * SS, outline=(255, 255, 255, 30), width=6 * SS)  # 어두운 바탕 가장자리
    return out
BIG, SMALL = master(False), master(True)
im = BIG.resize((1024, 1024), Image.LANCZOS)
im.save('app_1024.png')

def bmp_entry(im):
    im = im.convert('RGBA'); w, h = im.size
    px = im.tobytes('raw', 'BGRA')
    rows = [px[y*w*4:(y+1)*w*4] for y in range(h)][::-1]
    xor = b''.join(rows); mask_row = ((w + 31)//32)*4; andm = b''
    for y in range(h-1, -1, -1):
        bits = bytearray(mask_row)
        for x in range(w):
            if im.getpixel((x, y))[3] == 0: bits[x//8] |= 0x80 >> (x % 8)
        andm += bytes(bits)
    return struct.pack('<IiiHHIIiiII', 40, w, h*2, 1, 32, 0, len(xor)+len(andm), 0, 0, 0, 0) + xor + andm
def write_ico(path, imgs):
    datas = []
    for i in imgs:
        if i.size[0] >= 256:
            b = io.BytesIO(); i.save(b, 'PNG'); datas.append(b.getvalue())
        else: datas.append(bmp_entry(i))
    out = struct.pack('<HHH', 0, 1, len(imgs)); off = 6 + 16*len(imgs)
    for i, dd in zip(imgs, datas):
        w = i.size[0]; out += struct.pack('<BBBBHHII', w % 256, w % 256, 0, 0, 1, 32, len(dd), off); off += len(dd)
    open(path, 'wb').write(out + b''.join(datas))
rs = lambda s: (SMALL if s <= 32 else BIG).resize((s, s), Image.LANCZOS)
write_ico('app.ico', [rs(s) for s in (16, 20, 24, 32, 40, 48, 64, 128, 256)])
rs(256).save('app_256.png'); rs(64).save('app_64.png')
# NSIS 마법사 그림(164x314) · 머리 그림(150x57): 아이보리 바탕 + 아이콘 + 남색 띠
wiz = Image.new('RGB', (164, 314), (247, 246, 242)); wiz.paste(rs(120), (22, 60), rs(120))
dw = ImageDraw.Draw(wiz); dw.rectangle([0, 300, 164, 314], fill=NAVY[:3])
wiz.save('wizard.bmp')
hd = Image.new('RGB', (150, 57), (255, 255, 255)); hd.paste(rs(44), (98, 6), rs(44)); hd.save('header.bmp')
print('ok')
