---
name: verify
description: Verify Kerf before saying "done" or before a commit - incremental build, quick tests, full tests when core/ or tests/ changed, --ui-audit when app/ changed, line-ending-only check, Korean report. Uses the commands in .claude/cpp-project.json.
---
<!-- 목적: 이 저장소의 실제 명령으로 「끝났다」고 말하기 전 · 커밋 전에 돌리는 검증 절차. 전역 verify 스킬은 없다(프로젝트 전용). -->
# Verify Kerf (SectionViewer)

Run from the repository root with the Bash or PowerShell tool. Stop at the first failure, fix it, then restart from step 1. The commands are the ones in `.claude/cpp-project.json`; forward-slash paths work in both shells.

1. **Build** (incremental): `cmake --build C:/dev/kerf-v4c` → exit 0 and no new warnings (the pre-existing ones are `fopen` and `mirrored` only). If `C:/dev/kerf-v4c` does not exist, run `commands.configure` first (needs the junction `C:\dev\desca` → this repository; see `docs/DEV_PROGRESS.md` §2).
2. **Quick tests**: `C:/dev/kerf-v4c/asec_tests.exe "~CoalescingWorker*"` → `All tests passed` (about 2 s).
3. **Full tests** when `core/` or `tests/` changed: `C:/dev/kerf-v4c/asec_tests.exe`. A failure of `CoalescingWorker: 끄는 동안의 미리보기는…` (`tests/test_schedule.cpp`) is the known Windows timer flake; everything else must pass. `[real]` is skipped without `ASEC_REAL_3MX` (normal).
4. **UI audit** when `app/` changed (PowerShell, tool timeout 300000; the GUI exe needs `Start-Process -PassThru` + `WaitForExit`, and is killed if it hangs — DEV_RULES R7.2):
   ```
   Remove-Item C:\dev\tmp\kerf-check -Recurse -Force -ErrorAction SilentlyContinue; $env:QT_QPA_PLATFORM='offscreen'; $p = Start-Process -FilePath C:\dev\kerf-v4c\app\SectionViewer.exe -ArgumentList 'C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\tmp\kerf-check --ui-audit C:\dev\tmp\ui --quit' -PassThru; $null = $p.Handle; if ($p.WaitForExit(240000)) { "EXIT=" + $p.ExitCode } else { $p.Kill(); 'TIMEOUT(죽임)' }
   ```
   → `EXIT=0` and `RESULT ok` in `C:\dev\tmp\ui\ui-audit.txt`. `EXIT=9` means a layout rule failed: read that file. If the synthetic model is missing: `C:/dev/kerf-v4c/asec-make-synthetic.exe C:/dev/kerf-synth`.
5. **Visible UI change** (a screen looks different): run the `ui-visual-check` skill with `--themes light --scales 1` (no dark theme; the screenshot command has no scale argument — set `QT_SCALE_FACTOR` for 200 %), open the PNGs with Read, and compare with `docs/design/DESIGN_SPEC.md` (v5) and the board `docs/design/boards/kerf-design-v5.html`. Use the v4 boards in `docs/design-v4/boards/` only for what v5 does not cover.
6. **Line endings**: compare `git status --short` with `git diff HEAD --ignore-cr-at-eol --stat`. Files that appear only in the first are line-ending-only changes: do not stage them; tell the user.
7. **Report in Korean**: each command, its result (pass/fail counts, exit codes), and anything skipped with the reason. If a DEV_PROGRESS step was completed, update `docs/DEV_PROGRESS.md` (§1, §3, §4, §5).

This repository has no `.clang-format` or `.clang-tidy`: never format whole files; keep changed lines in the surrounding style.
