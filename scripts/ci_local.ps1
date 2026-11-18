<#
.SYNOPSIS
  Local CI for ArtPlus: validate skills/commands/agents, configure, build and run ctest.
.DESCRIPTION
  Uses the absolute Kitware CMake (not the WinLibs one on PATH) and the
  mingw-msvcrt preset. Does not commit or modify sources.
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts\ci_local.ps1
  powershell -ExecutionPolicy Bypass -File scripts\ci_local.ps1 -SkipTests
#>
param(
    [switch]$SkipSkills,
    [switch]$SkipBuild,
    [switch]$SkipTests
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root 'build'
$QtBin = 'C:\Qt\6.8.3\mingw_64\bin'
$Cmake = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe\cmake-4.4.4-windows-x86_64\bin\cmake.exe'

if (-not (Test-Path $Cmake)) {
    $found = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Directory -Filter 'Kitware.CMake_*' -ErrorAction SilentlyContinue |
        ForEach-Object { Get-ChildItem $_.FullName -Recurse -Filter 'cmake.exe' -ErrorAction SilentlyContinue } |
        Select-Object -First 1
    if ($found) { $Cmake = $found.FullName }
}

if (-not (Test-Path $Cmake)) { throw "CMake Kitware nao encontrado. Ajuste o caminho em scripts\ci_local.ps1." }
if (-not (Test-Path $QtBin)) { throw "Qt nao encontrado em $QtBin." }

$failures = @()
$started = Get-Date

function Write-Step([string]$Text) {
    Write-Host ""
    Write-Host "== $Text" -ForegroundColor Cyan
}

if (-not $SkipSkills) {
    Write-Step 'Skills, comandos e agentes'
    & python (Join-Path $PSScriptRoot 'validate_skills.py')
    if ($LASTEXITCODE -ne 0) { $failures += 'skills' }
}

if (-not $SkipBuild) {
    Write-Step 'Configure (mingw-msvcrt)'
    & $Cmake -S $Root -B $Build --preset mingw-msvcrt
    if ($LASTEXITCODE -ne 0) {
        $failures += 'configure'
    } else {
        $cache = Join-Path $Build 'CMakeCache.txt'
        $compiler = Select-String -Path $cache -Pattern '^CMAKE_CXX_COMPILER:' | Select-Object -First 1
        $prefix = Select-String -Path $cache -Pattern '^CMAKE_PREFIX_PATH:' | Select-Object -First 1
        if ($compiler) { Write-Host $compiler.Line }
        if ($prefix) { Write-Host $prefix.Line }

        Write-Step 'Build (release-mingw)'
        & $Cmake --build $Build --preset release-mingw
        if ($LASTEXITCODE -ne 0) { $failures += 'build' }
    }
}

if (-not $SkipTests) {
    Write-Step 'CTest (unit-tests)'
    $env:PATH = "$QtBin;$env:PATH"
    & $Cmake -E chdir $Root ctest --preset unit-tests
    if ($LASTEXITCODE -ne 0) { $failures += 'ctest' }
}

$elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
Write-Host ""
if ($failures.Count -gt 0) {
    Write-Host "FALHOU ($elapsed s): $($failures -join ', ')" -ForegroundColor Red
    exit 1
}
Write-Host "OK ($elapsed s): skills + build + testes" -ForegroundColor Green
exit 0
