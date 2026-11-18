---
name: PowerShell Windows Toolkit
description: Evita as armadilhas de shell desta máquina Windows 11 — PowerShell 5.1, UTF-8 sem BOM, acentos corrompidos, quoting, saída de comando nativo, dois CMake/Python no PATH, jobs em background e exit codes. Use ao rodar qualquer comando no projeto para não gerar evidência ilegível ou build falso.
metadata:
  version: "1"
---

# PowerShell nesta máquina

Shell padrão dos agentes: **Windows PowerShell 5.1** (não `pwsh`). Arquivos do projeto são **UTF-8 sem BOM**.

## Encoding (a armadilha número 1)

`Get-Content` sem `-Encoding` lê UTF-8 como ANSI e corrompe acentos na saída:

```powershell
Get-Content H:\Desktop\ArtPlus\AGENTS.md -Encoding UTF8
```

Ao escrever arquivo para o projeto, UTF-8 **sem BOM**:

```powershell
$utf8 = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText('H:\Desktop\ArtPlus\docs\x.md', $texto, $utf8)
```

`Set-Content -Encoding UTF8` no PS 5.1 grava **BOM**. Evite para `.md`, `.json`, `.jsonl`, `.cpp`, `.h` e `CMakeLists.txt`.

Saída de comando nativo também depende de `[Console]::OutputEncoding`:

```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8
```

## Paths e quoting

- Use aspas simples para caminho literal: `'C:\Esko\...'`.
- Caminho com espaço exige `&` e aspas: `& 'C:\Program Files\...\java.exe' -version`.
- No `Start-Process`, monte argumento com aspas escapadas (`"`"`); `.bat` precisa de `cmd.exe /c`.
- Não use `cd` global para trocar de projeto; passe `-S/-B` ou `-C` ao comando.

## PATH com duplicatas

| Comando | Resolve para | Correto |
|---|---|---|
| `cmake` | WinLibs UCRT | Kitware 4.4.4 absoluto |
| `python` | 3.12 | fixe o interpretador em script de análise |
| `g++` | pode ser UCRT | WinLibs POSIX.MSVCRT absoluto |

Nunca dependa do PATH para toolchain. Caminho absoluto sempre.

## Jobs e comandos que pausam

`analyzeHeadless.bat` pausa esperando tecla em erro. Rode em background com stdin vazio:

```powershell
$empty = Join-Path $env:TEMP 'empty-stdin.txt'
New-Item -ItemType File -Force $empty | Out-Null
$p = Start-Process cmd.exe -ArgumentList '/c', "`"$bat`" $args" `
     -RedirectStandardInput $empty -RedirectStandardOutput $log -RedirectStandardError "$log.err" `
     -WindowStyle Hidden -PassThru
```

Para trabalho curto, `Start-Job`/`Wait-Job -Timeout`; sempre leia o log, não só o exit code.

## Exit codes e pipelines

```powershell
& $cmake --build $build --preset release-mingw
if ($LASTEXITCODE -ne 0) { throw "build falhou: $LASTEXITCODE" }
```

`$LASTEXITCODE` é do último executável nativo. `try/catch` não captura exit code de `.exe`.

## Evidência legível

- Redirecione para arquivo e cite o caminho; não cole saída truncada.
- Em log, prefixe comando e data.
- Se a saída tiver mojibake, releia com `-Encoding UTF8` antes de concluir que o arquivo está corrompido.
- Não use `Write-Host` para dado que precisa ser lido depois; use `Write-Output` ou arquivo.

## Referências

- `references/encoding-and-quoting.md` — receitas de leitura/escrita, quoting e diagnóstico.
