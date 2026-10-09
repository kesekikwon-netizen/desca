# 목적: 팩토리 작업의 worktree 를 실제로 구성·빌드·시험하고, 결과(명령 · 종료 코드 · 요약 · 로그 위치)를
#       factory/jobs/<ID>/verify.md 에 덧붙인다. 미실행 단계는 「건너뜀(이유)」로 적고 통과로 쓰지 않는다.
# 사용: powershell -NoProfile -ExecutionPolicy Bypass -File factory/scripts/verify.ps1 -Id <ID> [-Full] [-UiAudit auto|on|off]
#   -Full      core/·tests/ 변경이 없어도 전체 시험을 돈다(기본: core/tests/tools/CMake 가 바뀌면 전체, 아니면 빠른 시험)
#   -UiAudit   auto(기본: app/ 가 바뀌면 실행) · on · off
#   -UiSizes   ui-audit 창 크기 목록(쉼표). 화면 변경 작업은 '1280x800,1920x1040,3440x1440,3840x2160' 로 돈다(와이드 · 4K)
#   -UiScales  ui-audit 배율 목록(쉼표, QT_SCALE_FACTOR). 화면 변경 작업은 '1,2' 로 돈다(200 % = 4K 모니터 기본 배율)
# 단계: configure(처음 한 번) → build → tests → asec-section(합성 모델 기준값 대조) → ui-audit(offscreen) → ci(기준 커밋의 GitHub Actions 결과, 참고용)
# 판정: 모든 단계 exit 0 → PASS. 하나라도 실패 → FAIL. 전체 시험이 알려진 불안정 시험(CoalescingWorker)만으로 실패하면
#       그 시험을 뺀 묶음을 한 번 더 돌려 통과하면 PASS(+FLAKY 표시). asec-section 기준값 불일치는 WARN(검토자가 판단).
# 종료 코드: 0 PASS · 1 FAIL · 2 인자·환경 오류
# 빌드 폴더: <worktree>-build. Qt DLL 은 복사하지 않고 PATH 와 QT_QPA_PLATFORM_PLUGIN_PATH 로 Qt 설치 폴더를 쓴다.
param(
    [Parameter(Mandatory = $true)][string]$Id,
    [switch]$Full,
    [ValidateSet('auto', 'on', 'off')][string]$UiAudit = 'auto',
    [string]$UiSizes = '1920x1040',      # 쉼표로 여러 개: '1280x800,1920x1040,3440x1440,3840x2160' (와이드 · 4K 안정성 요구)
    [string]$UiScales = '1',             # 쉼표로 여러 개: '1,2' (QT_SCALE_FACTOR — 2 = 200 % 배율, 4K 모니터 기본값)
    [string]$WtRoot = 'C:\dev\kerf-wt',
    [string]$Qt = 'C:\Qt\6.10.3\msvc2022_64',
    [string]$Vcpkg = 'C:\dev\vcpkg',
    [string]$Synth = 'C:\dev\kerf-synth\Synthetic.3mx',
    [string]$ExpectSection = 'polylines=1 vertices=54'
)
$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = New-Object System.Text.UTF8Encoding $false } catch {}
$Repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$jobDir = Join-Path $Repo "factory\jobs\$Id"
$jobMd = Join-Path $jobDir 'job.md'
$verifyMd = Join-Path $jobDir 'verify.md'
$wt = Join-Path $WtRoot $Id
$bd = "$wt-build"
$utf8 = New-Object System.Text.UTF8Encoding $false
function Fail($code, $msg) { Write-Host "ERROR: $msg"; exit $code }

if (-not (Test-Path $jobMd)) { Fail 2 "작업 기록이 없다: $jobMd" }
if (-not (Test-Path $wt)) { Fail 2 "worktree 가 없다: $wt (new-job.ps1 로 만든다)" }
if (-not (Test-Path (Join-Path $Qt 'bin\Qt6Core.dll'))) { Fail 2 "Qt 가 없다: $Qt" }
foreach ($tool in 'cmake', 'ninja', 'clang-cl', 'git') { if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { Fail 2 "PATH 에 $tool 이 없다" } }

$baseLine = Select-String -Path $jobMd -Pattern '^base_sha:\s*([0-9a-f]{7,40})' | Select-Object -First 1
if (-not $baseLine) { Fail 2 "job.md 에 base_sha 가 없다" }
$baseSha = $baseLine.Matches[0].Groups[1].Value

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logs = Join-Path $bd "logs\$stamp"
New-Item -ItemType Directory -Force $logs | Out-Null
$env:TEMP = 'C:\dev\tmp'; $env:TMP = 'C:\dev\tmp'; New-Item -ItemType Directory -Force 'C:\dev\tmp' | Out-Null
$env:Path = "$Qt\bin;C:\Program Files\LLVM\bin;$env:Path"

# ---- 바뀐 파일(기준 커밋 대비, 미커밋 포함) ----
$changed = @(git -C $wt diff --name-only $baseSha) + @(git -C $wt ls-files --others --exclude-standard) | Where-Object { $_ } | Sort-Object -Unique
$coreChanged = @($changed | Where-Object { $_ -match '^(core/|tests/|tools/|CMakeLists\.txt$)' }).Count -gt 0
$appChanged = @($changed | Where-Object { $_ -match '^app/' }).Count -gt 0
$headSha = (git -C $wt rev-parse HEAD).Trim()
$dirty = @(git -C $wt status --porcelain).Count -gt 0

# ---- 실행 도우미: 표준 출력·오류를 로그 파일로, 종료 코드·시간을 표에 ----
$rows = New-Object System.Collections.Generic.List[object]
$overall = 'PASS'
function Run($step, $exe, $argLine, $logName, [switch]$Gui) {
    $log = Join-Path $logs $logName
    $t0 = Get-Date
    # PowerShell 5.1: -ArgumentList 에 빈 문자열을 주면 오류 → 인자가 없을 때는 매개변수를 빼고 부른다
    $sp = @{ FilePath = $exe; WorkingDirectory = $wt; Wait = $true; PassThru = $true; NoNewWindow = $true
             RedirectStandardOutput = $log; RedirectStandardError = "$log.err" }
    if ($argLine) { $sp.ArgumentList = $argLine }
    $p = Start-Process @sp
    $sec = [int]((Get-Date) - $t0).TotalSeconds
    if ((Test-Path "$log.err") -and (Get-Item "$log.err").Length -gt 0) { Add-Content $log "`n--- stderr ---" -Encoding UTF8; Get-Content "$log.err" | Add-Content $log -Encoding UTF8 }
    Remove-Item "$log.err" -ErrorAction SilentlyContinue
    return [pscustomobject]@{ Step = $step; Cmd = "$exe $argLine"; Code = $p.ExitCode; Sec = $sec; Log = $log }
}
function Tail($log, $n = 3) { if (Test-Path $log) { ((Get-Content $log -Tail $n) -join ' / ') -replace '\|', '¦' } else { '' } }
function Add($r, $summary, $verdict) {
    $rows.Add([pscustomobject]@{ Step = $r.Step; Cmd = $r.Cmd; Code = $r.Code; Sec = $r.Sec; Summary = $summary; Verdict = $verdict; Log = $r.Log })
    Write-Host ("[{0}] {1} exit={2} {3}s — {4}" -f $verdict, $r.Step, $r.Code, $r.Sec, $summary)
}
function Skip($step, $why) { $rows.Add([pscustomobject]@{ Step = $step; Cmd = ''; Code = ''; Sec = ''; Summary = "건너뜀: $why"; Verdict = 'SKIP'; Log = '' }); Write-Host "[SKIP] $step — $why" }

# ---- 1. configure (처음 한 번) ----
if (-not (Test-Path (Join-Path $bd 'CMakeCache.txt'))) {
    $cfgArgs = "-S $wt -B $bd -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl " +
    "`"-DCMAKE_RC_FLAGS=/C 65001`" -DCMAKE_TOOLCHAIN_FILE=$($Vcpkg -replace '\\','/')/scripts/buildsystems/vcpkg.cmake " +
    "-DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_PREFIX_PATH=$($Qt -replace '\\','/')"
    $r = Run 'configure' 'cmake' $cfgArgs 'configure.log'
    if ($r.Code -ne 0) { $overall = 'FAIL'; Add $r (Tail $r.Log) 'FAIL' } else { Add $r 'CMake 구성 완료' 'PASS' }
}
else { Skip 'configure' '이미 구성됨(CMakeCache.txt 있음)' }

# ---- 2. build ----
if ($overall -eq 'PASS') {
    $r = Run 'build' 'cmake' "--build $bd" 'build.log'
    # 기존 경고 기준선(전체 빌드, 2026-10-09): CRT 사용 중단(-Wdeprecated-declarations: strcpy·sscanf·getenv) · fopen · mirrored. 그 밖은 「새 경고」로 센다
    $warnNew = @(Select-String -Path $r.Log -Pattern 'warning:' | ForEach-Object { $_.Line } | Where-Object { $_ -notmatch 'fopen|mirrored|-Wdeprecated-declarations' })
    if ($r.Code -ne 0) { $overall = 'FAIL'; $firstErr = (Select-String -Path $r.Log -Pattern 'error|FAILED:' | Select-Object -First 1).Line; Add $r "첫 오류: $firstErr" 'FAIL' }
    elseif ($warnNew.Count -gt 0) { Add $r ("빌드 성공, 새 경고 {0}개 — 첫 줄: {1}" -f $warnNew.Count, (($warnNew[0] -replace '\|', '¦').Substring(0, [Math]::Min(160, $warnNew[0].Length)))) 'WARN' }
    else { Add $r '빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만)' 'PASS' }
    if ((Test-Path "$bd\z.dll") -and (Test-Path "$bd\app")) { Copy-Item "$bd\z.dll" "$bd\app\" -Force }
}
else { Skip 'build' '구성 실패' }

# ---- 3. tests ----
$tests = Join-Path $bd 'asec_tests.exe'
if ($overall -eq 'PASS' -and (Test-Path $tests)) {
    $full = $Full -or $coreChanged
    if ($full) {
        $r = Run 'tests(full)' $tests '' 'tests-full.log'
        $sum = Tail $r.Log 2
        if ($r.Code -eq 0) { Add $r $sum 'PASS' }
        else {
            $r2 = Run 'tests(~CoalescingWorker*)' $tests '"~CoalescingWorker*"' 'tests-noflaky.log'
            $onlyFlaky = ($r2.Code -eq 0) -and ((Select-String -Path $r.Log -Pattern 'CoalescingWorker' | Measure-Object).Count -gt 0)
            if ($onlyFlaky) { Add $r "$sum → 알려진 불안정 시험(CoalescingWorker)만 실패" 'FLAKY'; Add $r2 (Tail $r2.Log 2) 'PASS' }
            else { $overall = 'FAIL'; Add $r $sum 'FAIL'; Add $r2 (Tail $r2.Log 2) ($(if ($r2.Code -eq 0) { 'PASS' } else { 'FAIL' })) }
        }
    }
    else {
        $r = Run 'tests(quick)' $tests '"~CoalescingWorker*"' 'tests-quick.log'
        if ($r.Code -ne 0) { $overall = 'FAIL' }
        Add $r (Tail $r.Log 2) ($(if ($r.Code -eq 0) { 'PASS' } else { 'FAIL' }))
    }
}
elseif ($overall -eq 'PASS') { Skip 'tests' "asec_tests.exe 없음: $tests" }
else { Skip 'tests' '빌드 실패' }

# ---- 4. asec-section 기준값(합성 모델 표준 단면선) ----
$secExe = Join-Path $bd 'asec-section.exe'
if ($overall -eq 'PASS' -and (Test-Path $secExe)) {
    if (-not (Test-Path $Synth)) {
        $mk = Join-Path $bd 'asec-make-synthetic.exe'
        if (Test-Path $mk) { $r0 = Run 'make-synthetic' $mk (Split-Path $Synth -Parent) 'make-synthetic.log'; Add $r0 (Tail $r0.Log 1) ($(if ($r0.Code -eq 0) { 'PASS' } else { 'FAIL' })) }
    }
    if (Test-Path $Synth) {
        $out = Join-Path $logs 'sec'
        $r = Run 'asec-section' $secExe "$Synth 200002 450006 200014 450006 --out $out" 'asec-section.log'
        $line = (Select-String -Path $r.Log -Pattern 'polylines=' | Select-Object -Last 1).Line
        if ($r.Code -ne 0) { $overall = 'FAIL'; Add $r (Tail $r.Log) 'FAIL' }
        elseif ($line -and $line -match [regex]::Escape($ExpectSection)) { Add $r $line 'PASS' }
        else { Add $r "기준값 '$ExpectSection' 과 다름: $line" 'WARN' }
    }
    else { Skip 'asec-section' "합성 모델 없음: $Synth" }
}
elseif ($overall -eq 'PASS') { Skip 'asec-section' 'asec-section.exe 없음' }
else { Skip 'asec-section' '앞 단계 실패' }

# ---- 5. ui-audit (offscreen) ----
$app = Join-Path $bd 'app\SectionViewer.exe'
$doUi = ($UiAudit -eq 'on') -or ($UiAudit -eq 'auto' -and $appChanged)
if ($doUi -and $overall -eq 'PASS' -and (Test-Path $app) -and (Test-Path $Synth)) {
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = "$Qt\plugins\platforms"
    # 창 크기 × 배율마다 한 번씩: 4K(3840x2160) · 와이드(3440x1440) · 200 %(QT_SCALE_FACTOR=2) 어디서나 자리 규칙이 지켜져야 한다
    foreach ($size in ($UiSizes -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })) {
        foreach ($scale in ($UiScales -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })) {
            if ($size -notmatch '^\d{3,5}x\d{3,5}$') { Skip "ui-audit $size" '크기 형식은 WxH'; continue }
            if ($scale -ne '1') { $env:QT_SCALE_FACTOR = $scale } else { Remove-Item Env:QT_SCALE_FACTOR -ErrorAction SilentlyContinue }
            $tag = "$size-x$scale"
            $settings = Join-Path $logs "settings-$tag"; $audit = Join-Path $logs "ui-audit-$tag"
            $r = Run "ui-audit $size ×$scale" $app "$Synth --line 200002 450006 200014 450006 --size $size --settings $settings --ui-audit $audit --quit" "ui-audit-$tag.log" -Gui
            $auditTxt = Join-Path $audit 'ui-audit.txt'
            $res = if (Test-Path $auditTxt) { (Select-String -Path $auditTxt -Pattern 'RESULT' | Select-Object -Last 1).Line } else { 'ui-audit.txt 없음' }
            if ($r.Code -ne 0) { $overall = 'FAIL'; Add $r "exit=$($r.Code) · $res (9 = 자리 규칙 실패, 다른 값 = 앱이 뜨지 않음)" 'FAIL' }
            else { Add $r $res 'PASS' }
            $rows[$rows.Count - 1].Log = $auditTxt
        }
    }
    Remove-Item Env:QT_QPA_PLATFORM, Env:QT_QPA_PLATFORM_PLUGIN_PATH, Env:QT_SCALE_FACTOR -ErrorAction SilentlyContinue
}
elseif (-not $doUi) { Skip 'ui-audit' ($(if ($UiAudit -eq 'off') { '-UiAudit off' } else { 'app/ 변경 없음' })) }
elseif ($overall -ne 'PASS') { Skip 'ui-audit' '앞 단계 실패' }
else { Skip 'ui-audit' "SectionViewer.exe 또는 합성 모델 없음" }

# ---- 6. GitHub Actions: 기준 커밋(base_sha)의 CI 결과(참고용 — 팩토리 브랜치는 push 되지 않으므로 기준이 깨져 있지 않은지만 본다) ----
$ciScript = Join-Path $PSScriptRoot 'ci-status.ps1'
if (Test-Path $ciScript) {
    $ciOut = & powershell -NoProfile -ExecutionPolicy Bypass -File $ciScript -Sha $baseSha 2>&1 | Out-String
    $ciCode = $LASTEXITCODE
    $ciLine = ($ciOut -split "`r?`n" | Where-Object { $_ -match '^CI ' } | Select-Object -Last 1)
    if (-not $ciLine) { $ciLine = ($ciOut.Trim() -split "`r?`n")[-1] }
    $ciVerdict = switch ($ciCode) { 0 { 'PASS' } 3 { 'WARN' } 4 { 'WARN' } default { 'WARN' } }
    $rows.Add([pscustomobject]@{ Step = 'ci(base)'; Cmd = "ci-status.ps1 -Sha $baseSha"; Code = $ciCode; Sec = ''; Summary = ($ciLine -replace '\|', '¦'); Verdict = $ciVerdict; Log = '' })
    Write-Host "[$ciVerdict] ci(base) exit=$ciCode — $ciLine"
}
else { Skip 'ci(base)' 'ci-status.ps1 없음' }

# ---- 기록 ----
$now = Get-Date -Format 'yyyy-MM-dd HH:mm'
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine("")
[void]$sb.AppendLine("## 검증 $now · **$overall** · HEAD ``$headSha``$(if ($dirty) { ' (+미커밋 변경 있음)' })")
[void]$sb.AppendLine("- 기준 ``$baseSha`` · 바뀐 파일 $($changed.Count)개: " + $(if ($changed.Count) { ($changed | Select-Object -First 12) -join ', ' + $(if ($changed.Count -gt 12) { ' …' }) } else { '(없음)' }))
[void]$sb.AppendLine("- 시험 범위: " + $(if ($Full -or $coreChanged) { '전체(core/tests/tools/CMake 변경 또는 -Full)' } else { '빠른 시험(~CoalescingWorker*)' }) + " · ui-audit: $UiAudit (크기 $UiSizes · 배율 $UiScales)")
[void]$sb.AppendLine("- 로그 폴더(미커밋, 이 PC): ``$logs``")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |")
[void]$sb.AppendLine("| --- | --- | --- | --- | --- | --- |")
foreach ($x in $rows) {
    $cmd = if ($x.Cmd) { '`' + ($x.Cmd -replace '\|', '¦') + '`' } else { '' }
    $lg = if ($x.Log) { ' · `' + (Split-Path $x.Log -Leaf) + '`' } else { '' }
    [void]$sb.AppendLine("| $($x.Step) | $($x.Verdict) | $($x.Code) | $($x.Sec) | $($x.Summary) | $cmd$lg |")
}
[IO.File]::AppendAllText($verifyMd, $sb.ToString(), $utf8)

# job.md 의 head_sha · updated 갱신(status 는 스킬이 정한다)
$jm = [IO.File]::ReadAllText($jobMd, [System.Text.Encoding]::UTF8)
$jm = [regex]::Replace($jm, '(?m)^head_sha:.*$', "head_sha: $headSha$(if ($dirty) { '  # +미커밋' })")
$jm = [regex]::Replace($jm, '(?m)^updated:.*$', "updated: $now")
$jm = [regex]::Replace($jm, '(?m)^last_verify:.*$', "last_verify: $overall ($now)")
[IO.File]::WriteAllText($jobMd, $jm, $utf8)

Write-Host ""
Write-Host "RESULT $overall · 기록: $verifyMd"
exit $(if ($overall -eq 'PASS') { 0 } else { 1 })
