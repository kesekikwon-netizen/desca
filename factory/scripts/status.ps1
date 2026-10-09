# 목적: factory/jobs/*/job.md 의 머리말을 모아 작업 상태 표를 보여 준다(/factory status 가 부른다).
# 사용: powershell -NoProfile -ExecutionPolicy Bypass -File factory/scripts/status.ps1 [-All]
#       기본은 merged·closed·rejected 를 숨긴다. -All 이면 전부.
# 종료 코드: 0 (표를 못 읽은 작업은 status=? 로 표시)
param([switch]$All)
$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = New-Object System.Text.UTF8Encoding $false } catch {}
$Repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$Jobs = Join-Path $Repo 'factory\jobs'
$keys = 'id', 'title', 'status', 'rework_count', 'ai_review', 'user_approval', 'base_sha', 'head_sha', 'updated'

$rows = foreach ($d in Get-ChildItem $Jobs -Directory | Where-Object { $_.Name -ne '_template' } | Sort-Object Name) {
    $f = Join-Path $d.FullName 'job.md'
    $o = [ordered]@{ id = $d.Name }
    foreach ($k in $keys) { if ($k -ne 'id') { $o[$k] = '?' } }
    if (Test-Path $f) {
        $inHead = $false
        foreach ($line in Get-Content $f -Encoding UTF8) {
            if ($line -match '^---\s*$') { if ($inHead) { break } else { $inHead = $true; continue } }
            if ($inHead -and $line -match '^([a-z_]+):\s*(.*?)\s*(#.*)?$') {
                $k = $Matches[1]; $v = $Matches[2]
                if ($k -in $keys) { $o[$k] = $v }
            }
        }
    }
    foreach ($k in 'base_sha', 'head_sha') { if ($o[$k].Length -gt 9) { $o[$k] = $o[$k].Substring(0, 9) } }
    [pscustomobject]$o
}
if (-not $All) { $rows = $rows | Where-Object { $_.status -notin @('merged', 'closed', 'rejected') } }
if (-not $rows) { Write-Output '(진행 중 작업 없음)'; exit 0 }
$rows | Format-Table -AutoSize -Property id, status, rework_count, ai_review, user_approval, base_sha, head_sha, updated, title | Out-String -Width 220 | Write-Output
exit 0
