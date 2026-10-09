# 목적: 팩토리 스크립트 실행기 — 어느 폴더에서 불러도 자기 위치(저장소 안 factory/)를 기준으로 맞는 스크립트를 돈다.
# 사용(저장소 루트 또는 desc 폴더의 factory.cmd 가 이 파일을 부른다):
#   factory status [-All]                 작업 상태 표
#   factory new -Slug <slug> -Title "<제목>" [-Request "<사용자 말>"] [-Base feature/design-v2] [-Force]
#   factory verify <ID> [-Full] [-UiAudit auto|on|off]
#   factory ci <sha> [-Wait]              GitHub Actions 결과(gh)
#   factory help
# 종료 코드: 부른 스크립트의 종료 코드 그대로. 모르는 명령은 2.
$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = New-Object System.Text.UTF8Encoding $false } catch {}
$S = Join-Path $PSScriptRoot 'scripts'
$cmd = if ($args.Count -gt 0) { [string]$args[0] } else { 'help' }
# PowerShell 은 원소 하나짜리 배열을 문자열로 풀고, 빈 결과를 $null 로 만들기 쉽다 → Select-Object -Skip 을 @( ) 로 감싸 늘 배열로 둔다
$rest = @($args | Select-Object -Skip 1)

function Usage {
    Write-Host '사용: factory <명령> [인자]'
    Write-Host '  status [-All]                                   작업 상태 표'
    Write-Host '  new -Slug <slug> -Title "<제목>" [-Request "<말>"]  작업 생성(worktree + 브랜치 + 기록)'
    Write-Host '  verify <ID> [-Full] [-UiAudit auto|on|off]      빌드 · 시험 · 기준값 · ui-audit · CI 기록'
    Write-Host '  ci <sha> [-Wait]                                GitHub Actions 결과'
    Write-Host "스크립트 위치: $S"
}

switch ($cmd.ToLower()) {
    'status' { & (Join-Path $S 'status.ps1') @rest; exit $LASTEXITCODE }
    'new'    { & (Join-Path $S 'new-job.ps1') @rest; exit $LASTEXITCODE }
    'verify' {
        if ($rest.Count -lt 1) { Write-Host 'ERROR: 작업 ID 가 필요하다. 예: factory verify 20261009-01-readme'; Usage; exit 2 }
        $id = [string]$rest[0]; $more = @($rest | Select-Object -Skip 1)
        & (Join-Path $S 'verify.ps1') -Id $id @more; exit $LASTEXITCODE
    }
    'ci' {
        if ($rest.Count -lt 1) { Write-Host 'ERROR: 커밋 SHA 가 필요하다. 예: factory ci de14dc2'; Usage; exit 2 }
        $sha = [string]$rest[0]; $more = @($rest | Select-Object -Skip 1)
        & (Join-Path $S 'ci-status.ps1') -Sha $sha @more; exit $LASTEXITCODE
    }
    'help' { Usage; exit 0 }
    default { Write-Host "ERROR: 모르는 명령 '$cmd'"; Usage; exit 2 }
}
