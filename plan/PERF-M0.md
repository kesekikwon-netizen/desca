# M0 성능 측정 기록 (2026-10-06, 실제 모델 jeju1 Production_2, 139 MB / 타일 265 / 잎 220)

환경: Linux 상자, Xvfb + **llvmpipe(소프트웨어 GL)** — 실제 GPU 수치 아님. 실제 PC 에서 다시 재야 함.

| 항목 | 값 |
|---|---|
| 열기(루트 머리) | 25–39 ms |
| 첫 화면 | 228–239 ms |
| 잎 전체 디코드(지오메트리) | 3,668,495 삼각형, ~575 ms (8 스레드, 텍스처 제외) |
| 카메라 240프레임(맞춤→확대→이동→축소) | 평균 32.8 ms, 최악 159 ms(이동 중 큰 텍스처 업로드 1건, paint 145 ms) |
| 단면 끌기 60프레임(~16 ms 간격) | 미리보기 58회(29/s), 화면 그리기 평균 4.6 ms·최악 32 ms |
| 놓은 뒤 최종(잎) 단면 | 628 ms (524,338 삼각형) |
| 잎 피킹 | 0.3–0.5 ms(asec-info, 디코드 후), 앱 14–53 ms(첫 접근 시 텍스처 디코드 포함) |

다시 재는 법:
```
xvfb-run -a ./build-linux/app/SectionViewer <파일.3mx> --perf-log perf.csv --log perf.log --quit   # 또는 실제 PC 에서 xvfb 없이
./build-linux/asec-info <파일.3mx> --tree --decode --pick X Y --section AX AY BX BY --csv info.csv
```
Windows: `SectionViewer.exe <파일.3mx> --perf-log perf.csv --log perf.log --quit` (실제 GPU 로 측정)

발견·수정: 1.0 의 단면 틈 잇기(joinGaps)가 O(n³) 라 실제 모델 최종 단면(조각 수천 개)이 수 분 멈춤 → 3aa68cf 에서 수정.
남은 것: GPU 업로드를 프레임당 예산으로 나누기(이동 중 159 ms 프레임), 실제 GPU 프레임 예산 조정.
