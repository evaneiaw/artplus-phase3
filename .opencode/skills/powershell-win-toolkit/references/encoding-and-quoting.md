# Encoding e quoting no PowerShell 5.1

## Ler UTF-8

```powershell
Get-Content H:\Desktop\ArtPlus\AGENTS.md -Encoding UTF8
```

Sem `-Encoding UTF8`, acentos viram `Ã©`/`�`. O arquivo não está corrompido; a leitura está.

## Escrever UTF-8 sem BOM

```powershell
$utf8 = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText('H:\Desktop\ArtPlus\docs\x.md', $texto, $utf8)
```

## Console e comando nativo

```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8
```

Se um `.exe` ainda emitir mojibake, redirecione para arquivo e leia com `-Encoding UTF8`.

## Quoting

```powershell
& 'C:\Program Files\Eclipse Adoptium\jdk-25.0.5.8-hotspot\bin\java.exe' -version
$args = @('-S', 'H:\Desktop\ArtPlus', '-B', 'H:\Desktop\ArtPlus\build', '--preset', 'mingw-msvcrt')
& $cmake @args
```

- Array de argumentos evita problema de espaço.
- `Start-Process` com `.bat` exige `cmd.exe /c` e aspas escapadas.
- Não use `Invoke-Expression` com caminho do usuário.

## Exit code

```powershell
& $exe @args
if ($LASTEXITCODE -ne 0) { throw "falhou ($LASTEXITCODE)" }
```

## Jobs

```powershell
$job = Start-Job -ScriptBlock { & $using:headless @using:args }
Wait-Job $job -Timeout 1800 | Out-Null
Receive-Job $job
Remove-Job $job -Force
```

Para processo que pausa, prefira `Start-Process` com stdin vazio e log em arquivo.

## Diagnóstico

| Sintoma | Causa |
|---|---|
| `Token inesperado` em `.ps1` | arquivo UTF-8 com acento lido como ANSI |
| acento `?` na saída | console/OEM |
| argumento cortado | falta de aspas/array |
| `cmake` errado | PATH WinLibs |
| `0xc0000135` | PATH sem Qt |
