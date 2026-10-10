# Trench 앱 아이콘 포장: 앱이 직접 그린 PNG(--app-icon DIR)를 app.ico · app_256 · app_64 · app_1024 로 묶는다.
# 도형을 여기서 다시 그리지 않는다 — 한 곳(app/icons/app/trench*.svg)이 앱 안 아이콘과 .ico 의 원본이다.
# 사용: SectionViewer.exe --app-icon C:\dev\tmp\app-icon --quit   (offscreen 가능, 종료 12 = 눈 검사 실패)
#       python packaging/assets/make_app_ico.py C:\dev\tmp\app-icon
# 필요: Pillow. 만드는 파일은 이 폴더(packaging/assets/)의 app.ico · app_256.png · app_64.png(app.rc · 설치 프로그램이 읽음).
import os
import sys
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SIZES = (16, 24, 32, 48, 256)


def main(src):
    frames = []
    for px in SIZES:
        path = os.path.join(src, f"app-{px}.png")
        if not os.path.exists(path):
            sys.exit(f"missing {path} — run SectionViewer.exe --app-icon first")
        im = Image.open(path).convert("RGBA")
        if im.size != (px, px):
            sys.exit(f"{path}: size {im.size} != {px}")
        frames.append(im)
    big = frames[-1]
    # Pillow 의 ICO 저장: 가장 큰 그림을 기준으로 sizes 의 각 크기를 따로 줄이므로, 크기마다 앱이 그린 장을 그대로 쓰려고 append_images 로 넘긴다
    big.save(os.path.join(HERE, "app.ico"), format="ICO", sizes=[(px, px) for px in SIZES], append_images=frames[:-1])
    big.save(os.path.join(HERE, "app_256.png"))
    Image.open(os.path.join(src, "app-48.png")).convert("RGBA").resize((64, 64), Image.LANCZOS).save(os.path.join(HERE, "app_64.png"))
    print("app.ico frames:", [f.size[0] for f in frames])


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
