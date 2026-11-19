# AGENTS.md — contrato do projeto ArtPlus

ArtPlus é uma reimplementação funcional do Esko ArtPro+ em C++17/Qt 6.8.3.
Este arquivo define como os agentes trabalham. Em qualquer conflito, **este arquivo ganha**.

## 1. Postura do projeto

- Engenharia reversa **profunda e declarada** do ArtPro+ (Ghidra + triagem + operação do original).
- O objetivo é **clone funcional completo**, não uma ferramenta inspirada.
- Toda funcionalidade implementada precisa de evidência rastreável; nada de "achismo" virando código.
- Decisão do proprietário do projeto: o ArtPlus é de uso próprio e não será distribuído como
  substituto do produto comercial. Responsabilidade de distribuição/licenciamento é de quem publicar.

## 2. Pipeline

```
ghidra ─┐
rea ────┼─► evidência bruta ─► documentacao ─► F-### ─► arquitetura ─► ADR
comportamento ─┘                                              │
                                                              ▼
                                                        implementacao
                                                              │
                                                              ▼
                                                           testes
                                                              │
                                                              ▼
                                                           auditor
```

Nenhuma etapa começa sem o artefato da anterior. O `auditor` tem veto.

## 3. Agentes e donos de diretório

| Agente | Modo | Escreve em | Não pode |
|---|---|---|---|
| `orquestrador` | primary | `ROADMAP.md`, `FEATURE_MATRIX.csv`, `docs/phases/**`, `docs/agents/**`, `docs/reverse-engineering/INVENTORY.md` | editar código, analisar binário, rodar shell |
| `ghidra` | subagent | `tools/ghidra_analysis/**`, `tools/ghidra_projects/**`, `docs/reverse-engineering/**`, `rag/evidence/**` | editar `src/`, `tests/`, specs |
| `rea` | subagent | `rag/evidence/**`, `rag/sources/**`, `docs/reverse-engineering/**` | editar `src/`, `tests/`, specs |
| `comportamento` | subagent | `tools/reference_capture/**`, `docs/behavior/**`, `rag/evidence/**` | editar `src/`, `tests/`, specs |
| `documentacao` | subagent | `docs/features/**`, `docs/glossary/**`, `rag/index/**`, `rag/metadata/**` | editar `src/`, evidência bruta |
| `arquitetura` | subagent | `docs/architecture/**`, `docs/decisions/**` | editar `src/`, `tests/` |
| `implementacao` | subagent | `src/**`, `CMakeLists.txt`, `tools/pdf_inspector/**` | editar `tests/`, `docs/`, `rag/` |
| `testes` | subagent | `tests/**`, `docs/testing/**`, `CMakePresets.json`, `scripts/**`, `tools/visual_compare/**`, `dossiers/**` | editar `src/` |
| `auditor` | subagent | `docs/audit/**` | editar qualquer outra coisa |

Nenhum subagente lança subagente. O `orquestrador` é o único que despacha.

## 4. Identificadores e evidência

- Spec: `F-###` em `docs/features/F-###.md` (funcionalidade grande: `F-025a`, `F-025b`).
- Decisão: `ADR-###` em `docs/decisions/`.
- Evidência: `rag/evidence/F-###/` com subpastas `ghidra/`, `rea/`, `behavior/`.
- Todo achado estático é identificado como `GHIDRA-<modulo>-<endereco>`.
- Toda captura é identificada como `CAP-<AAAAMMDD>-<nn>`.

`rag/evidence/F-###/manifest.json` mínimo:

```json
{
  "id": "F-025",
  "versao_ref": "25.0",
  "binario": "C:\\Esko\\bg_prog_artpro+_v250\\ArtPro+.app\\Contents\\Windows\\DocumentMarks.dll",
  "sha256": "...",
  "estatico": ["GHIDRA-DocumentMarks-0x18001A2C0"],
  "comportamental": ["CAP-20261118-03"],
  "bloqueios": []
}
```

## 5. Comandos canônicos

```powershell
# CMake correto (o do PATH é WinLibs UCRT e mistura CRT com o Qt)
$cmake = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe\cmake-4.4.4-windows-x86_64\bin\cmake.exe'

& $cmake --preset mingw-msvcrt
& $cmake --build --preset release-mingw
& $cmake --build build --target ArtPlus
ctest --preset unit-tests
```

```powershell
# Ghidra 12.2 (exige JDK 25; JDK 21 falha)
$env:JAVA_HOME = 'C:\Program Files\Eclipse Adoptium\jdk-25.0.5.8-hotspot'
& 'H:\Desktop\ArtPlus\tools\ghidra_12.2_PUBLIC\support\analyzeHeadless.bat' `
  'H:\Desktop\ArtPlus\tools\ghidra_projects' 'ArtProPlus' -import '<binário>'
```

```powershell
# Triagem
diec -j <binário>
strings64 -n 8 <binário>
python scripts\inspect_pe.py <binário>
python scripts\scan_binary.py <diretório>
```

Alvos de referência:

```
v250: C:\Esko\bg_prog_artpro+_v250\ArtPro+.app\Contents\Windows  (300 binários, ~327 MB)
v240: C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows
```

> **Instalação única do Ghidra:** `H:\Desktop\ArtPlus\tools\ghidra_12.2_PUBLIC`.
> Não baixe nem extraia outra versão. Os zips antigos foram removidos da área de trabalho.

## 6. Regras de ouro

1. **Evidência primeiro.** Spec sem evidência não vira ADR; ADR sem spec não vira código.
2. **Um dono por diretório.** Quem não é dono não escreve — pede ao orquestrador.
3. **Nada de "feito" sem comando reproduzível.** Relatório sem caminho + saída é relatório vazio.
4. **Estado real no roadmap.** Build quebrado aparece como quebrado.
5. **Uma sessão de GUI por vez.** O ArtPro+ é recurso único.
6. **Nada de stub silencioso.** `TODO(F-###)` é aceitável; função que finge funcionar não é.
7. **Unidade explícita.** `mm`, `pt`, `dpi` no nome do campo e na spec.
8. **Conflito de contrato** vai para `docs/agents/CONFLITOS.md` antes de virar código.

## 7. Estado atual do projeto

- Fase: inventário e reverse engineering (ROADMAP fase 1–2).
- Build: existem alvos de teste compilando; o link do `ArtPlus` já teve falha de toolchain
  (UCRT × MSVCRT), tratada pelo `build doctor` do agente `testes`.
- Módulos já mapeados em `docs/reverse-engineering/`: `Document.dll` (call graph, estruturas,
  workflows), inventário de binários, mapa de dependências e estratégia Ghidra.
- Fila de cobertura: `docs/reverse-engineering/INVENTORY.md`.

## 8. Skills

As 26 skills em `.opencode/skills/` são o conhecimento técnico por domínio. Todas têm
frontmatter `name` + `description` e são anunciadas ao modelo. Consulte antes de inventar
abordagem; a `description` da skill diz quando ela se aplica.

| Agente | Skills principais |
|---|---|
| `orquestrador` | `project-orchestration`, `github-workflow` |
| `ghidra` | `ghidra-analysis` |
| `rea` | `rea-reverse-engineering` |
| `comportamento` | `artpro-behavior-capture` |
| `documentacao` | `technical-specification` |
| `arquitetura` | `cpp-architecture-adr`, `qt-cpp-development` |
| `implementacao` | `qt-cpp-development`, `document-core`, `pdf-engineering`, `color-management` |
| `testes` | `build-and-release`, `test-strategy`, `visual-regression-testing` |
| `auditor` | `evidence-verification` |
| todos | `powershell-win-toolkit` + skills de domínio (trapping, marcas, layers, etc.) |

Skills de domínio: `prepress-trapping`, `marks-and-barcodes`, `layers-and-separations`,
`screening-and-halftone`, `selection-and-transform`, `step-and-repeat`, `vector-geometry`,
`preflight-quality-control`, `action-lists-automation`, `dependency-license-audit`.

## 9. Problemas conhecidos do ambiente

| Problema | Estado | Correção |
|---|---|---|
| `rea` MCP falhava: exigia Node ≥ 24.22.1; o Node do sistema é 24.13.1 | resolvido em 2026-11-18 | Node 26.13.1 portátil em `C:\Users\Evanei\.local\share\opencode\node-v26.13.1\node.exe`. Não troque o Node do sistema para "consertar" isso. |
| Instalação global do `rea-agents` tem addon nativo com SHA-256 divergente (`native\windows\build\rea-windows-x64.node`), o que **rejeita o provider Ghidra** | resolvido em 2026-11-18 | Cópia limpa em `C:\Users\Evanei\.local\share\opencode\rea-side`; o MCP `rea` e o agente `rea` usam essa cópia. Se o MCP ainda não expuser `ghidra`, reinicie o OpenCode. |
| `pc-control` MCP falhava: `desktop-control-mcp` 0.1.0 usa FastMCP, mas havia `mcp` 2.3.0 | resolvido em 2026-11-18 | `python -m pip install "mcp<2"` (agora 1.30.0). Não atualize `mcp` sem migrar o `desktop_control`. |
| `C:\RE-Tools` não existe | — | Use caminhos absolutos: Ghidra em `H:\Desktop\ArtPlus\tools\ghidra_12.2_PUBLIC`. |
| `analyzeHeadless.bat` pausa em erro | — | Rode via `Start-Process -RedirectStandardInput <arquivo vazio>` em background e leia o log. |
| `cmake` do PATH é WinLibs UCRT | — | Use o CMake Kitware absoluto (seção 5). |
| Preset com caminho fixo do winget | — | Primeira tarefa do agente `testes`: tornar a descoberta do GCC dinâmica. |

### Smoke test registrado

`DocumentMarks.dll` importado com sucesso no Ghidra 12.2/JDK 25 (`-noanalysis -deleteProject`),
log em `tools/ghidra_analysis/smoke.log`. Ambiente de análise validado.
