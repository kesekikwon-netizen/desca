# Kerf 아이콘: 사진·바탕 이미지 없이 전부 면과 선으로 그림.
# Kerf = 톱으로 자른 자리. 흙 층(상아·황토·흙색·암갈) 덩어리를 가는 틈으로 잘라 오른쪽이 살짝 내려앉고,
# 왼쪽 자른 면을 따라 빨간 단면선이 선다. 어두운 기반암 스퀴클 바탕. 16 px 에서도 띠·틈·빨강이 보이게 굵은 면만 씀.
import io, math, os, struct, sys
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))

S, SS = 1024, 4
W = S * SS
BED = (34, 33, 31, 255)
RIM = (66, 63, 58, 255)
IVORY = (250, 249, 245, 255)
BANDS = [(238, 228, 208, 255), (214, 176, 136, 255), (184, 88, 58, 255), (118, 54, 35, 255)]
RED = (255, 36, 28, 255)


def sc(v):
    return v * SS


def squircle(cx, cy, r, n=5.0, steps=720):
    pts = []
    for i in range(steps):
        a = 2 * math.pi * i / steps
        c, s = math.cos(a), math.sin(a)
        pts.append((cx + r * math.copysign(abs(c) ** (2 / n), c), cy + r * math.copysign(abs(s) ** (2 / n), s)))
    return pts


def wavy(y0, x0, x1, amp, phase, steps=80, cycles=1.1):
    return [(x0 + (x1 - x0) * i / steps, y0 + amp * math.sin(phase + 2 * math.pi * cycles * i / steps)) for i in range(steps + 1)]


def block(d, x0, x1, ground, bounds, drop, phase):
    """x0..x1 흙 덩어리. ground = 지표 곡선, bounds = 층 경계 y, drop = 내려앉은 양."""
    bottom = sc(900)
    curves = [[(x, y + drop) for x, y in ground]] + [wavy(b + drop, x0, x1, sc(12), phase + 1.7 * (i + 1)) for i, b in enumerate(bounds)]
    for i in range(len(BANDS)):
        upper = curves[i]
        d.polygon(upper + [(x1, bottom), (x0, bottom)], fill=BANDS[i])


def draw():
    im = Image.new('RGBA', (W, W), (0, 0, 0, 0))
    mask = Image.new('L', (W, W), 0)
    sq = squircle(W / 2, W / 2, W / 2 - sc(8))
    ImageDraw.Draw(mask).polygon(sq, fill=255)
    art = Image.new('RGBA', (W, W), BED)
    d = ImageDraw.Draw(art)
    L0, R1 = sc(150), sc(874)
    g0, g1 = sc(540), sc(604)          # 톱 자리(kerf)
    bounds = [sc(470), sc(590), sc(708)]
    drop = sc(54)
    block(d, L0, g0, wavy(sc(340), L0, g0, sc(12), 0.3), bounds, 0, 0.0)
    block(d, g1, R1, wavy(sc(340), g1, R1, sc(12), 2.4), bounds, drop, 1.1)
    # 흙 덩어리를 둥근 사각 창으로 자름(창 밖은 기반암)
    win = Image.new('L', (W, W), 0)
    ImageDraw.Draw(win).rounded_rectangle([L0, sc(150), R1, sc(874)], radius=sc(56), fill=255)
    art = Image.composite(art, Image.new('RGBA', (W, W), BED), win)
    d = ImageDraw.Draw(art)
    # 틈(kerf) 바닥까지 기반암으로 다시 칠해 확실한 검은 선으로
    d.rectangle([g0, sc(150), g1, sc(874)], fill=BED)
    # 빨간 단면선: 왼쪽 덩어리의 자른 면 + 끝 손잡이(상아 원)
    topY = sc(340) + sc(12) * math.sin(0.3 + 2 * math.pi * 1.1)
    edge = g0 - sc(4)
    d.line([(edge, topY), (edge, sc(874))], fill=RED, width=sc(34))
    d.ellipse([edge - sc(36), topY - sc(36), edge + sc(36), topY + sc(36)], fill=IVORY, outline=RED, width=sc(16))
    # 테
    d.line(sq + [sq[0]], fill=RIM, width=sc(12))
    im.paste(art, (0, 0), mask)
    return im.resize((S, S), Image.LANCZOS)


def bmp_entry(im):
    im = im.convert('RGBA'); w, h = im.size
    px = im.tobytes('raw', 'BGRA')
    rows = [px[y * w * 4:(y + 1) * w * 4] for y in range(h)][::-1]
    xor = b''.join(rows); mask_row = ((w + 31) // 32) * 4; andm = b''
    for y in range(h - 1, -1, -1):
        bits = bytearray(mask_row)
        for x in range(w):
            if im.getpixel((x, y))[3] == 0: bits[x // 8] |= 0x80 >> (x % 8)
        andm += bytes(bits)
    return struct.pack('<IiiHHIIiiII', 40, w, h * 2, 1, 32, 0, len(xor) + len(andm), 0, 0, 0, 0) + xor + andm

def write_ico(path, imgs):
    datas = []
    for i in imgs:
        if i.size[0] >= 256:
            b = io.BytesIO(); i.save(b, 'PNG'); datas.append(b.getvalue())
        else: datas.append(bmp_entry(i))
    out = struct.pack('<HHH', 0, 1, len(imgs)); off = 6 + 16 * len(imgs)
    for i, dd in zip(imgs, datas):
        w = i.size[0]; out += struct.pack('<BBBBHHII', w % 256, w % 256, 0, 0, 1, 32, len(dd), off); off += len(dd)
    open(path, 'wb').write(out + b''.join(datas))

if __name__ == '__main__':
    os.chdir(HERE)
    im = draw()
    im.save('app_1024.png')
    rs = lambda s: im.resize((s, s), Image.LANCZOS)
    write_ico('app.ico', [rs(s) for s in (16, 20, 24, 32, 40, 48, 64, 128, 256)])
    rs(256).save('app_256.png'); rs(64).save('app_64.png')
    wiz = Image.new('RGB', (164, 314), (247, 246, 242)); wiz.paste(rs(120), (22, 60), rs(120))
    ImageDraw.Draw(wiz).rectangle([0, 300, 164, 314], fill=(181, 87, 58))
    wiz.save('wizard.bmp')
    hd = Image.new('RGB', (150, 57), (255, 255, 255)); hd.paste(rs(44), (98, 6), rs(44)); hd.save('header.bmp')
    print('icons ok')
