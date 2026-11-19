<#
.SYNOPSIS
  Overnight ArtPlus extraction runner.
.DESCRIPTION
  Picks modules with scripts/auto_queue.py and runs one fresh OpenCode session
  per module using the /auto command in static mode. Writes per-module logs and
  an Overnight report. Detached execution: start it with Start-Process.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts\auto_batch.ps1 -Mode static -Hours 8 -MaxModules 8 -IncludeDone
#>
param(
    [ValidateSet('static', 'extract', 'full')]
    [string]$Mode = 'static',
    [double]$Hours = 8,
    [int]$MaxModules = 8,
    [int]$PerModuleMinutes = 50,
    [switch]$IncludeDone,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$LogDir = Join-Path $Root 'logs\auto'
$Opencode = 'C:\Users\Evanei\AppData\Roaming\npm\opencode.ps1'
$Stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$Date = Get-Date -Format 'yyyyMMdd'
$Deadline = (Get-Date).AddHours($Hours)
$Report = Join-Path $Root "docs\phases\OVERNIGHT-$Date.md"

New-Item -ItemType Directory -Force $LogDir | Out-Null
New-Item -ItemType Directory -Force (Split-Path -Parent $Report) | Out-Null

function Write-Log([string]$Text) {
    $line = "[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $Text
    Write-Host $line
    Add-Content -Path (Join-Path $LogDir "runner-$Stamp.log") -Value $line -Encoding UTF8
}

$queueArgs = @((Join-Path $PSScriptRoot 'auto_queue.py'), '--limit', $MaxModules)
if ($IncludeDone) { $queueArgs += '--include-done' }
$modules = & python @queueArgs
if ($LASTEXITCODE -ne 0 -or -not $modules) {
    Write-Log 'fila vazia ou auto_queue.py falhou'
    exit 1
}

$header = @(
    "# Execucao noturna - $Date",
    "",
    "- Inicio: $(Get-Date -Format 'yyyy-MM-dd HH:mm')",
    "- Modo: $Mode",
    "- Orcamento: $Hours h / $MaxModules modulos / $PerModuleMinutes min por modulo",
    "- Comando: ``/auto <modulo> --$Mode``",
    "",
    "| Modulo | Resultado | Duracao | Dossie | Log |",
    "|---|---|---|---|---|"
)
$header | Set-Content -Path $Report -Encoding UTF8

if ($DryRun) {
    Write-Log ("DRY-RUN fila: " + ($modules -join ', '))
    $modules | ForEach-Object { Write-Host "  $_" }
    exit 0
 }

Write-Log ("fila: " + ($modules -join ', '))
$results = @()
$index = 0

foreach ($module in $modules) {
    if ((Get-Date) -ge $Deadline) {
        Write-Log 'orcamento de tempo atingido; parando'
        break
    }
    $index++
    $safe = ($module -replace '[^A-Za-z0-9._+-]', '_')
    $log = Join-Path $LogDir "$Stamp-$index-$safe.log"
    $err = "$log.err"
    $started = Get-Date
    Write-Log "[$index/$($modules.Count)] $module"

    $bogus = Join-Path $Root 'rag\evidence\F-UNDEF'
    if (Test-Path $bogus) {
        $rejected = Join-Path $LogDir "rejected-F-UNDEF-$Stamp-$index"
        Move-Item -Path $bogus -Destination $rejected -Force
        Write-Log "F-UNDEF invalido movido para $rejected"
    }

    $argumentString = '-NoProfile -ExecutionPolicy Bypass -File "{0}" run --agent orquestrador --dir "{1}" --command auto "{2} --{3}"' -f $Opencode, $Root, $module, $Mode
    Write-Log "comando: powershell $argumentString"

    $process = Start-Process powershell -ArgumentList $argumentString -WorkingDirectory $Root `
        -WindowStyle Hidden -RedirectStandardOutput $log -RedirectStandardError $err -PassThru

    $process | Wait-Process -Timeout ($PerModuleMinutes * 60) -ErrorAction SilentlyContinue
    $process.Refresh()
    if (-not $process.HasExited) {
        Write-Log "timeout em $module; encerrando arvore $($process.Id)"
        & taskkill /PID $process.Id /T /F | Out-Null
        $status = 'timeout'
    } elseif ($process.ExitCode -eq 0) {
        $status = 'ok'
    } else {
        $status = "exit $($process.ExitCode)"
    }

    $elapsed = [math]::Round(((Get-Date) - $started).TotalMinutes, 1)

    if (Test-Path $bogus) {
        $rejected = Join-Path $LogDir "rejected-F-UNDEF-$Stamp-$index-pos"
        Move-Item -Path $bogus -Destination $rejected -Force
        & python (Join-Path $PSScriptRoot 'dossier.py') --module $module | Out-Null
        Write-Log "F-UNDEF pos-execucao removido; dossie regenerado"
    }

    $dossier = Join-Path $Root ("dossiers\" + [IO.Path]::GetFileNameWithoutExtension($module) + "\DOSSIER.md")
    $hasDossier = (Test-Path $dossier) -and ((Get-Item $dossier).LastWriteTime -ge $started)
    if ($hasDossier) {
        $relative = $dossier.Substring($Root.Length + 1) -replace '\\', '/'
        $dossierCell = "[$relative]($relative)"
    } else {
        $dossierCell = '-'
    }
    $row = "| $module | $status | ${elapsed} min | $dossierCell | $(Split-Path -Leaf $log) |"
    Add-Content -Path $Report -Value $row -Encoding UTF8
    $results += [pscustomobject]@{ Module = $module; Status = $status; Minutes = $elapsed; Dossier = $hasDossier }
    Write-Log "$module -> $status (${elapsed} min, dossie=$hasDossier)"
}

$ok = @($results | Where-Object { $_.Status -eq 'ok' }).Count
$withDossier = @($results | Where-Object { $_.Dossier }).Count
$blocked = @($results | Where-Object { $_.Status -ne 'ok' }).Count

$footer = @(
    "",
    "## Resumo",
    "",
    "- Modulos executados: $($results.Count)",
    "- Sessoes ok: $ok",
    "- Com dossie: $withDossier",
    "- Falhas/timeout: $blocked",
    "- Fim: $(Get-Date -Format 'yyyy-MM-dd HH:mm')",
    "",
    "## Onde ler",
    "",
    "- ``dossiers/INDEX.md``",
    "- ``dossiers/<Modulo>/DOSSIER.md``",
    "- ``logs/auto/``",
    "",
    "Nenhum push foi feito. Modo estatico nao altera ``src/``."
)
$footer | Add-Content -Path $Report -Encoding UTF8
Write-Log "fim: ok=$ok dossie=$withDossier falhas=$blocked"
