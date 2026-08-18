<#
.SYNOPSIS
  FPG 전체 검증 — 데이터 검증 + C++ 빌드 + 자동화 테스트를 한 번에.

.DESCRIPTION
  docs/16 §16.14의 테스트 종류 중 CI가 대신해 줄 수 없는 것들을 로컬에서 돌립니다.
  GitHub 호스티드 러너에는 언리얼 엔진이 없어(약 43GB + 라이선스) C++ 자동화
  테스트를 클라우드에서 돌릴 수 없습니다. 자체 호스팅 러너를 두기 전까지는
  **커밋 전에 이 스크립트를 돌리는 것**이 유일한 방어선입니다.

  ⚠️ 언리얼 에디터가 실행 중이면 Live Coding이 빌드를 막습니다. 먼저 닫으십시오.

.PARAMETER SkipBuild
  빌드를 건너뛰고 이미 빌드된 바이너리로 테스트만 돌립니다.

.PARAMETER Filter
  자동화 테스트 필터. 기본 'FPG' — 전체. 예: 'FPG.Flight'

.EXAMPLE
  .\Tools\run_tests.ps1
  .\Tools\run_tests.ps1 -Filter FPG.Data -SkipBuild
#>
[CmdletBinding()]
param(
    [switch]$SkipBuild,
    [string]$Filter = 'FPG',
    [string]$EngineDir = 'D:\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
$UProject = Join-Path $RepoRoot 'FPG.uproject'
$Failed = $false

function Write-Section($Text) {
    Write-Host ''
    Write-Host "=== $Text ===" -ForegroundColor Cyan
}

# ── 0. 에디터가 떠 있으면 빌드가 실패합니다 ──────────────────
if (-not $SkipBuild) {
    $Editor = Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue
    if ($Editor) {
        Write-Host '언리얼 에디터가 실행 중입니다. Live Coding이 빌드를 막으므로 먼저 닫으십시오.' -ForegroundColor Red
        $Editor | Select-Object Id, MainWindowTitle | Format-Table -AutoSize
        exit 1
    }
}

# ── 1. 데이터 검증 (엔진 불필요, 빠름) ───────────────────────
# 먼저 돌립니다. CSV가 깨졌으면 빌드에 시간을 쓸 이유가 없습니다.
Write-Section '데이터 검증 (Tools/validate_data.py)'
$Python = if (Get-Command py -ErrorAction SilentlyContinue) { 'py' } else { 'python' }
& $Python (Join-Path $PSScriptRoot 'validate_data.py')
if ($LASTEXITCODE -ne 0) {
    Write-Host '데이터 검증 실패' -ForegroundColor Red
    $Failed = $true
}

# ── 2. 빌드 ──────────────────────────────────────────────────
if (-not $SkipBuild) {
    Write-Section '빌드 (FPGEditor Win64 Development)'
    & (Join-Path $EngineDir 'Engine\Build\BatchFiles\Build.bat') `
        FPGEditor Win64 Development -Project="$UProject" -WaitMutex |
        Select-Object -Last 3
    if ($LASTEXITCODE -ne 0) {
        Write-Host '빌드 실패 — 테스트를 건너뜁니다.' -ForegroundColor Red
        exit 1
    }
}

# ── 3. 자동화 테스트 ─────────────────────────────────────────
Write-Section "자동화 테스트 ($Filter)"
$LogPath = Join-Path $RepoRoot 'Saved\Logs\AutomationRun.log'
& (Join-Path $EngineDir 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') `
    $UProject `
    -ExecCmds="Automation RunTests $Filter; Quit" `
    -unattended -nopause -nosplash -nullrhi -NoSound `
    -log -abslog="$LogPath" 2>&1 | Out-Null

if (-not (Test-Path $LogPath)) {
    Write-Host "테스트 로그를 찾을 수 없습니다: $LogPath" -ForegroundColor Red
    exit 1
}

$Results = Select-String -Path $LogPath -Pattern 'Test Completed\. Result=\{(\w+)\} Name=\{([^}]+)\}' -AllMatches
$Pass = 0
$Fail = 0
foreach ($Line in $Results) {
    foreach ($M in $Line.Matches) {
        $Result = $M.Groups[1].Value
        $Name = $M.Groups[2].Value
        if ($Result -eq 'Success') {
            Write-Host "  [통과] $Name" -ForegroundColor Green
            $Pass++
        } else {
            Write-Host "  [실패] $Name" -ForegroundColor Red
            $Fail++
        }
    }
}

if ($Pass -eq 0 -and $Fail -eq 0) {
    Write-Host "  테스트가 하나도 실행되지 않았습니다. 필터 '$Filter' 를 확인하십시오." -ForegroundColor Yellow
    $Failed = $true
}

if ($Fail -gt 0) {
    Write-Section '실패 상세'
    Select-String -Path $LogPath -Pattern 'Expected .* to be|Error: LogFPGData' |
        ForEach-Object { '  ' + ($_.Line -replace '^\[[^\]]+\]\[\s*\d+\]', '') }
    Write-Host ''
    Write-Host "전체 로그: $LogPath" -ForegroundColor Yellow
    $Failed = $true
}

# ── 결과 ─────────────────────────────────────────────────────
Write-Host ''
if ($Failed) {
    Write-Host "실패 — 통과 $Pass / 실패 $Fail" -ForegroundColor Red
    exit 1
}
Write-Host "전부 통과 — $Pass 개" -ForegroundColor Green
exit 0
