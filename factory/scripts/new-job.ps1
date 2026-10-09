# 목적: 팩토리 작업 하나를 시작한다 — 브랜치 factory/<ID> + worktree(C:\dev\kerf-wt\<ID>) + 기록 폴더 factory/jobs/<ID>.
#       worktree 는 ASCII 경로에 둔다(한글 경로에서는 리소스 컴파일러가 멈춤). 빌드 폴더는 <worktree>-build 로 따로.
# 사용: powershell -NoProfile -ExecutionPolicy Bypass -File factory/scripts/new-job.ps1 -Slug readme-cli -Title "README 명령줄 절 보강" [-Request "사용자 말 그대로"] [-Base feature/design-v2] [-Force]
# 종료 코드: 0 ok · 2 인자·환경 오류 · 3 동시 작업 제한(진행 중 작업 있음, -Force 로 무시) · 4 git 실패
# 하지 않는 것: 사용자 작업 트리를 건드리지 않음(checkout·reset·stash 없음). push 없음.
param(
    [Parameter(Mandatory = $true)][ValidatePattern('^[a-z0-9][a-z0-9-]{1,30}$')][string]$Slug,
    [Parameter(Mandatory = $true)][string]$Title,
    [string]$Request = '',
    [string]$Base = 'feature/design-v2',
    [string]$WtRoot = 'C:\dev\kerf-wt',
    [switch]$Force
)
$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = New-Object System.Text.UTF8Encoding $false } catch {}
$Repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$Jobs = Join-Path $Repo 'factory\jobs'
$utf8 = New-Object System.Text.UTF8Encoding $false
function Fail($code, $msg) { Write-Host "ERROR: $msg"; exit $code }

if (-not (Test-Path (Join-Path $Jobs '_template\job.md'))) { Fail 2 "양식이 없다: $Jobs\_template" }

# 기준 브랜치 확인
git -C $Repo rev-parse --verify --quiet "${Base}^{commit}" | Out-Null
if ($LASTEXITCODE -ne 0) { Fail 2 "기준 브랜치가 없다: $Base" }
$baseSha = (git -C $Repo rev-parse $Base).Trim()

# 동시 작업 1개 제한: merged·closed·rejected 가 아닌 작업이 있으면 멈춘다
$active = @(Get-ChildItem $Jobs -Directory | Where-Object { $_.Name -ne '_template' } | ForEach-Object {
        $m = Select-String -Path (Join-Path $_.FullName 'job.md') -Pattern '^status:\s*(\S+)' | Select-Object -First 1
        if ($m) { [pscustomobject]@{ Id = $_.Name; Status = $m.Matches[0].Groups[1].Value } }
    } | Where-Object { $_.Status -notin @('merged', 'closed', 'rejected') })
if ($active.Count -gt 0 -and -not $Force) {
    $active | Format-Table -AutoSize | Out-String | Write-Host
    Fail 3 "진행 중 작업이 있다(동시 작업은 1개). 먼저 끝내거나 -Force."
}

# ID = 날짜-순번-slug
$date = Get-Date -Format 'yyyyMMdd'
$n = @(Get-ChildItem $Jobs -Directory | Where-Object { $_.Name -like "$date-*" }).Count + 1
$Id = '{0}-{1:00}-{2}' -f $date, $n, $Slug
$branch = "factory/$Id"
$wt = Join-Path $WtRoot $Id
$bd = "$wt-build"
$jobDir = Join-Path $Jobs $Id
if (Test-Path $wt) { Fail 2 "worktree 폴더가 이미 있다: $wt" }
if (Test-Path $jobDir) { Fail 2 "기록 폴더가 이미 있다: $jobDir" }
git -C $Repo rev-parse --verify --quiet "refs/heads/$branch" | Out-Null
if ($LASTEXITCODE -eq 0) { Fail 2 "브랜치가 이미 있다: $branch" }

New-Item -ItemType Directory -Force $WtRoot | Out-Null
git -C $Repo worktree add -b $branch $wt $Base
if ($LASTEXITCODE -ne 0) { Fail 4 "git worktree add 실패" }

# 기록 폴더: 양식 복사 + 자리 표시 채우기
Copy-Item (Join-Path $Jobs '_template') $jobDir -Recurse
$now = Get-Date -Format 'yyyy-MM-dd HH:mm'
Get-ChildItem $jobDir -File | ForEach-Object {
    $t = [IO.File]::ReadAllText($_.FullName, [System.Text.Encoding]::UTF8)
    $t = $t.Replace('{{ID}}', $Id).Replace('{{TITLE}}', $Title).Replace('{{BASE_BRANCH}}', $Base).Replace('{{BASE_SHA}}', $baseSha)
    $t = $t.Replace('{{BRANCH}}', $branch).Replace('{{WORKTREE}}', $wt).Replace('{{BUILD_DIR}}', $bd).Replace('{{NOW}}', $now).Replace('{{REQUEST}}', $Request)
    [IO.File]::WriteAllText($_.FullName, $t, $utf8)
}

Write-Output "job: $Id"
Write-Output "branch: $branch (base $Base @ $baseSha)"
Write-Output "worktree: $wt"
Write-Output "build: $bd"
Write-Output "record: $jobDir"
exit 0
