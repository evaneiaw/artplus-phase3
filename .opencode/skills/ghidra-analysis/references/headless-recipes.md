# Receitas Ghidra headless

## Variáveis

```powershell
$env:JAVA_HOME = 'C:\Program Files\Eclipse Adoptium\jdk-25.0.5.8-hotspot'
$headless = 'H:\Desktop\ArtPlus\tools\ghidra_12.2_PUBLIC\support\analyzeHeadless.bat'
$projects = 'H:\Desktop\ArtPlus\tools\ghidra_projects'
$scripts  = 'H:\Desktop\ArtPlus\scripts\ghidra_scripts'
```

## Import com análise

```powershell
$target = 'C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows\DocumentMarks.dll'
$log = 'H:\Desktop\ArtPlus\tools\ghidra_analysis\DocumentMarks.import.log'
$empty = Join-Path $env:TEMP 'ghidra-empty.txt'
New-Item -ItemType File -Force $empty | Out-Null

$cmd = "/c `"`"$headless`" `"$projects`" ArtProPlus -import `"$target`" -overwrite`""
Start-Process cmd.exe -ArgumentList $cmd -RedirectStandardInput $empty `
  -RedirectStandardOutput $log -RedirectStandardError "$log.err" -WindowStyle Hidden
```

## Processar projeto existente

```powershell
& $headless $projects ArtProPlus -process DocumentMarks.dll -noanalysis `
  -scriptPath $scripts -postScript ExportFunctions.java
```

## Exportar estruturas

```powershell
& $headless $projects ArtProPlus -process DocumentMarks.dll -noanalysis `
  -scriptPath $scripts -postScript ExportStructures.java
```

## Smoke test sem análise

```powershell
& $headless $projects Smoke -import $target -noanalysis -deleteProject
```

Use `-deleteProject` só em smoke; não apague projeto com análise boa.

## Onde a saída aparece

- `ExportFunctions.java` → `H:\Desktop\ArtPlus\rag\ghidra\exports\<programa>\` (`functions.csv`, `symbols.csv`, `xrefs.csv`, `strings.csv`, call graph).
- `ExportStructures.java` → mesma árvore, estruturas.
- Log do headless → caminho passado em `-RedirectStandardOutput`.

## Diagnóstico

| Sintoma | Causa provável | Ação |
|---|---|---|
| Processo para e não termina | `analyzeHeadless.bat` esperando tecla | rodar com stdin vazio e ler log |
| “unsupported java version” | `JAVA_HOME` em 21 | apontar para jdk-25 |
| `Provider ghidra` rejeitado no REA | addon nativo com hash divergente | usar `rea-side`, não a instalação global |
| Import “concluído” sem funções | `-noanalysis` | reimportar com análise |
| Nome do programa diferente do arquivo | Ghidra renomeia | listar projeto ou usar o nome do log |

## Regra de ouro

Um import grande por vez. Antes de importar, procure `.rep`/`.gpr` e relatório existente.
