# 목적: GitHub Actions(.github/workflows/ci.yml) 결과를 커밋 SHA 기준으로 읽어 한 줄 판정을 돌려준다(gh CLI, 읽기만).
#       팩토리는 push 를 하지 않으므로 ① 기준 커밋(base_sha)의 CI 가 통과했는지 ② 사용자가 push 한 뒤 병합 커밋의 CI 가 통과했는지 볼 때 쓴다.
# 사용: powershell -NoProfile -ExecutionPolicy Bypass -File factory/scripts/ci-status.ps1 -Sha <sha> [-Branch feature/design-v2] [-Wait]
#   -Wait  진행 중이면 끝날 때까지 기다린다(최대 20분, 30초 간격)
# 출력: "CI <sha7> <상태> <결론> jobs: <이름=결론, …> <url>"  또는  "CI <sha7> none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전)"
# 종료 코드: 0 success · 1 failure/cancelled 등 실패 · 2 gh 없음·로그인 안 됨 · 3 실행 없음 · 4 아직 진행 중(-Wait 없이)
param(
    [Parameter(Mandatory = $true)][ValidatePattern('^[0-9a-f]{7,40}$')][string]$Sha,
    [string]$Branch = '',
    [switch]$Wait
)
# 'Stop' 이면 네이티브 명령(gh · git)의 stderr 한 줄도 종료 오류가 되므로 Continue 로 두고 종료 코드로 판정한다
$ErrorActionPreference = 'Continue'
try { [Console]::OutputEncoding = New-Object System.Text.UTF8Encoding $false } catch {}
if (-not (Get-Command gh -ErrorAction SilentlyContinue)) { Write-Host 'ERROR: gh 가 PATH 에 없다'; exit 2 }
# gh 는 현재 폴더의 git 저장소로 대상을 정한다 → 어디서 불러도 저장소 안에서 돈다
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-Location $repo
gh auth status 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Host 'ERROR: gh 로그인 안 됨(gh auth login)'; exit 2 }

# gh run list --commit 은 40자 전체 SHA 만 받는다 → 짧으면 저장소에서 풀어 준다
if ($Sha.Length -lt 40) {
    $full = (git -C $repo rev-parse --verify --quiet "$Sha^{commit}" 2>$null)
    if ($LASTEXITCODE -ne 0 -or -not $full) { Write-Host "ERROR: 커밋을 찾을 수 없다: $Sha"; exit 2 }
    $Sha = $full.Trim()
}
$short = $Sha.Substring(0, 7)
$deadline = (Get-Date).AddMinutes(20)
while ($true) {
    $args = @('run', 'list', '--commit', $Sha, '--limit', '5', '--json', 'databaseId,status,conclusion,headSha,url,workflowName')
    if ($Branch) { $args += @('--branch', $Branch) }
    $json = (& gh @args 2>$null) -join "`n"      # 여러 줄 출력을 한 문자열로(줄마다 따로 파싱되면 깨진다)
    if ($LASTEXITCODE -ne 0 -or -not $json) { Write-Host "CI $short none (gh run list 실패 또는 결과 없음)"; exit 3 }
    $runs = @(($json | ConvertFrom-Json) | Where-Object { $_ })   # PS 5.1: "[]" 는 $null 로 풀린다 → 빈 배열로
    if ($runs.Count -eq 0) { Write-Host "CI $short none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전)"; exit 3 }
    $run = $runs[0]
    if ($run.status -ne 'completed') {
        if ($Wait -and (Get-Date) -lt $deadline) { Write-Host "CI $short $($run.status) … 30초 뒤 다시"; Start-Sleep -Seconds 30; continue }
        Write-Host "CI $short $($run.status) (진행 중) $($run.url)"; exit 4
    }
    # jq 식의 큰따옴표는 PS 5.1 이 네이티브 명령에 넘길 때 벗겨지므로 JSON 을 받아 여기서 푼다
    $jobsJson = (& gh run view $run.databaseId --json jobs 2>$null) -join "`n"
    $jobLine = ''
    if ($jobsJson) { $jobLine = (@(($jobsJson | ConvertFrom-Json).jobs | Where-Object { $_ }) | ForEach-Object { "$($_.name)=$($_.conclusion)" }) -join ', ' }
    Write-Host "CI $short completed $($run.conclusion) jobs: $jobLine $($run.url)"
    exit $(if ($run.conclusion -eq 'success') { 0 } else { 1 })
}
