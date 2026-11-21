---
name: Ghidra Analysis
description: Extrai funções, xrefs, vtables/RTTI, estruturas e pseudocódigo de binários do ArtPro+ com Ghidra 12.2 headless e JDK 25. Use ao importar módulo, analisar função, revalidar contra v24.03.10017, rodar post-script ou registrar evidência GHIDRA-<modulo>-<endereco>.
metadata:
  version: "1"
---

# Ghidra 12.2 headless

## Ambiente verificado

```
JAVA_HOME = C:\Program Files\Eclipse Adoptium\jdk-25.0.5.8-hotspot
GHIDRA    = H:\Desktop\ArtPlus\tools\ghidra_12.2_PUBLIC
HEADLESS  = H:\Desktop\ArtPlus\tools\ghidra_12.2_PUBLIC\support\analyzeHeadless.bat
PROJETOS  = H:\Desktop\ArtPlus\tools\ghidra_projects
ALVOS     = C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows   (referência v24.03.10017)
SCRIPTS   = H:\Desktop\ArtPlus\scripts\ghidra_scripts
EXPORTS   = H:\Desktop\ArtPlus\rag\ghidra\exports
```

Não use a raiz `RE-Tools` (não existe nesta máquina) nem o Ghidra global. JDK 21 falha; exporte `JAVA_HOME` na sessão.

## Antes de importar

1. Procure o projeto do módulo em `tools/ghidra_projects` (`.rep`/`.gpr`).
2. Procure relatório em `docs/reverse-engineering/<Modulo>_*.md`.
3. Se já existe análise, use `-process` em vez de reimportar.
4. Import de binário grande leva minutos a horas. Nunca repita por conveniência.

## Import (assíncrono, sempre)

`analyzeHeadless.bat` pausa em erro esperando tecla. Rode em background, com stdin vazio e log:

```powershell
$env:JAVA_HOME = 'C:\Program Files\Eclipse Adoptium\jdk-25.0.5.8-hotspot'
$headless = 'H:\Desktop\ArtPlus\tools\ghidra_12.2_PUBLIC\support\analyzeHeadless.bat'
$projects = 'H:\Desktop\ArtPlus\tools\ghidra_projects'
$target   = 'C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows\DocumentMarks.dll'
$log      = 'H:\Desktop\ArtPlus\tools\ghidra_analysis\DocumentMarks.import.log'
$empty    = Join-Path $env:TEMP 'ghidra-empty-stdin.txt'
New-Item -ItemType File -Force $empty | Out-Null

$cmd = "/c `"`"$headless`" `"$projects`" ArtProPlus -import `"$target`" -overwrite`""
$p = Start-Process cmd.exe -ArgumentList $cmd -RedirectStandardInput $empty `
     -RedirectStandardOutput $log -RedirectStandardError "$log.err" -WindowStyle Hidden -PassThru
$p.Id
```

Acompanhe o log; não bloqueie a sessão esperando. `-noanalysis` só para smoke test.

## Exportar funções e estruturas

```powershell
& $headless $projects ArtProPlus -process DocumentMarks.dll -noanalysis `
  -scriptPath 'H:\Desktop\ArtPlus\scripts\ghidra_scripts' `
  -postScript ExportFunctions.java
```

`ExportFunctions.java` grava em `rag/ghidra/exports/<programa>/`: funções, símbolos, xrefs, strings e call graph.
`ExportStructures.java` exporta estruturas. Se faltar um dado, **estenda o script** em `scripts/ghidra_scripts/`; não produza evidência por screenshot.

## Pseudocódigo

`ExportFunctions.java` não exporta C decompilado. Para uma função específica, use o REA CLI validado:

```powershell
$node = 'C:\Users\Evanei\.local\share\opencode\node-v26.13.1\node.exe'
$rea  = 'C:\Users\Evanei\.local\share\opencode\rea-side\node_modules\rea-agents\scripts\rea.mjs'
$env:GHIDRA_INSTALL_DIR = 'H:\Desktop\ArtPlus\tools\ghidra_12.2_PUBLIC'
& $node $rea decompile '<binário>' --procedure 0x18001A2C0 --format json
```

Salve o JSON bruto em `rag/evidence/F-###/ghidra/` e cite a função no relatório.

## O que registrar por função

- Endereço (`0x...`), nome atual e assinatura.
- Xrefs de entrada e saída (quem chama, o que chama).
- Vtable/RTTI e classe provável.
- Strings vizinhas e mensagens de UI literais.
- Recursos/dialogs referenciados.
- Confiança (`alta`/`media`/`baixa`) **e o motivo**.

Formato: `rag/evidence/F-###/ghidra/<modulo>__<endereco>.md`, ID `GHIDRA-<Modulo>-<endereco>`.
Índice navegável: `docs/reverse-engineering/<Modulo>_FUNCTIONS.md`.

## Revalidação contra a versão-alvo

Confirme que o achado corresponde à v24.03.10017 (versão-alvo). Registros da v25 são históricos e fora do fluxo ativo; só afirme "é assim" depois de confirmar contra a versão-alvo.

## Limites

- Não edite `src/`, `tests/`, specs, roadmap nem `INVENTORY.md`.
- Não invente nome de função, classe ou comportamento; inferência é marcada como inferência.
- Um import grande por vez. Não lance subagentes.
- Binário protegido/ofuscado: registre o bloqueio com DIE, seções e entropia; não insista.

## Referências

- `references/headless-recipes.md` — import, process, post-script, batch e diagnóstico de log.
- `references/evidence-format.md` — cabeçalho YAML, IDs e exemplo preenchido.
