# Auditoria F-001 — Trim Marks (Cut Mark)

Data: 2026-11-19 · Auditor: auditor
Veredito: **REPROVADO**

Escopo auditado: working tree em `HEAD 43124c874353d2bb8324de49097415f26f7c2683` **com alterações
não commitadas** (`src/prepress/` ainda untracked, `document_model.h`, `document_serializer.cpp`,
`main_window.*`, `CMakeLists.txt` e `tests/unit/test_cut_mark.cpp`). O laudo vale para esse estado,
não para o HEAD. Nada foi editado fora de `docs/audit/`.

---

## 1. Evidência verificada

| Item | Esperado | Encontrado | OK |
|---|---|---|---|
| `rag/evidence/F-001/manifest.json` | id/versão/binário/hashes/listas | presente, `versao_ref = 25.0` | ✅ |
| SHA-256 do binário | `FFA8809D…BEE10` | `FFA8809D294A7AA08BD2794E0B0C1E64608D898B5F7BA698A4E58A8E575BEE10` | ✅ |
| SHA-256 do input | `ADFD91…40D7` | `ADFD91B4E95BCE3708DB37055885C355880068EE53A1472AB1DC82E0820640D7` | ✅ |
| `binario` existe | caminho v250 | `C:\Esko\...\Windows\DocumentMarks.dll` existe | ✅ |
| 6 IDs `GHIDRA-*` do manifest | 6 arquivos | `rag/evidence/F-001/ghidra/` com 14 arquivos; os 6 citados existem | ✅ |
| `CAP-20261119-01` | diretório/capturas | `behavior/` com 21 PNGs; `docs/behavior/TrimMarks.md` existe | ✅ |
| Conteúdo estático mínimo | ctor/regenerate/GetReferenceBox | `DocumentMarks__0x18013A8B0.md` e `__0x18013EE90.md` contêm defaults e sequência de geração | ✅ |
| Manifest × spec §7 | lista de evidência estática completa | manifest lista **6** IDs; a spec cita **14** documentos Ghidra. Todos os 14 arquivos existem, mas 8 não estão em `estatico` | ⚠️ ressalva (D6) |

Arquivos de evidência conferidos por listagem e hash de binário/input; nenhum caminho citado no
manifest está ausente. `bloqueios` registra corretamente a ausência de PDF de saída e de mensagem
E1/E2 reproduzida.

---

## 2. Cadeia spec → evidência → código → teste

| Regra | Evidência | Código | Teste | Elo |
|---|---|---|---|---|
| R2 defaults | `GHIDRA-…0x18013A8B0` | `src/prepress/cut_mark.h:35-49` | `test_cut_mark.cpp:125-156` | ✅ |
| R3 caixa de referência | `GHIDRA-…0x18013CB50`, `Document__GetPageBox.md` | `src/prepress/cut_mark.cpp:107-119` | `test_cut_mark.cpp:288-331` | ✅ |
| R4 referência por Cut Mark | `GHIDRA-…0x18013CB50` | `cut_mark.cpp:121-135,141-156` + `CutMarkManager` | `test_cut_mark.cpp:392-486` | ✅ (ver D4) |
| R5 offset + 1 undo | `GHIDRA-…0x1800ADB10` | `cut_mark.cpp:361-383,497-513` | `test_cut_mark.cpp:219-285` | ✅ na edição; ❌ no redo de criação (D1) |
| R7 geometria | `GHIDRA-…0x18013EE90` | `cut_mark.cpp:158-188,190-219` | `test_cut_mark.cpp:159-216` | ✅ |
| R8 regeneração | `GHIDRA-…0x18013EE90` | `cut_mark.cpp:221-250` | `test_cut_mark.cpp:334-384` | ✅ |
| R9 identidade/nome | `GHIDRA-…0x18013E3B0` | `cut_mark.cpp:103-105,252-279` | `test_cut_mark.cpp:125-156` | ✅ |
| R10 `.artplus` | ADR-001 D4 | `cut_mark.cpp:429-467` + `document_serializer.cpp:337-338,450` | `test_cut_mark.cpp:489-562` | ✅ (metade `.artplus`) |
| R6 `fx` | `GHIDRA-…0x18013D160` | campo persiste, **não avalia** (`cut_mark.h:40`) | — | adiado F-001c (documentado) |
| R11 import PDF+ | `GHIDRA-…0x18017C5D0` | ausente | — | adiado F-001a (documentado) |
| R12 legado AI | `GHIDRA-…0x180111410`, `0x180052CA0` | ausente | — | adiado F-001b (documentado) |
| R13 mark set | `GHIDRA-…0x18024D3E0` | ausente | — | "fora de F-001" no ADR, **sem ID de continuação** (D7) |
| R1 criação/UI | `CAP-20261119-01` | `main_window.cpp:688-689,741,1062-1076` | — | parcial; painel/barra/título `*` ausentes (D2) |

---

## 3. Cobertura R1–R13

| Regra | Estado | Observação |
|---|---|---|
| R1 | **Parcial** | Comando existe, sem modal, insere e empilha undo. Não há painel `Marks`, barra `Dynamic Marks` nem `*` no título (`main_window.cpp:601` título fixo; `:1488-1492` só status bar). Painel foi explicitamente adiado no ADR (D4, linha 200-203); o `*` não foi. |
| R2 | Implementada | `TrimBox`, offset 0, `orientation=1`, `repetition=2`, `lineTypes={0}`; testado. |
| R3 | Implementada | `PageBox` → `Page::boxes()`; caixa vazia ⇒ nada criado. Sem tabela de fallback (E7), desvio declarado no ADR. |
| R4 | Implementada no modelo | Referência só entre Cut Marks, via catálogo do manager; testado, incluindo auto-referência e id inexistente. Ver D4 sobre a caixa usada. |
| R5 | **Implementada na edição, defeituosa no redo de criação** | `setParams` empilha exatamente 1 comando; teste cobre undo/redo da edição. Undo da criação remove o grupo, mas o redo não reinscreve a marca no manager (D1/D6). |
| R6 | Adiada F-001c | `offsetFormula` é persistido; não avaliado. Documentado. |
| R7 | Implementada (inferência) | 4 vértices × 2 segmentos = 8, um único `GroupObject`. Constantes 5,0 mm / 0,25 pt / magenta são `TODO(F-001)`. |
| R8 | Implementada | Regeneração substitui 8 paths; teste prova que a contagem não cresce. |
| R9 | Implementada no `.artplus` | Nome `Cut Mark N`; `kind` = `DynamicObject_PDFPlusCutMark` no metadata. Não há tipo de objeto PDF+ real (F-001a). |
| R10 | Implementada só em `.artplus` | Chaves PDF+ (`Position`, `Reference`, `Orientation`, …) adiadas para F-001a. |
| R11 | Adiada F-001a | Parser/validações E1–E3 ausentes. |
| R12 | Adiada F-001b | Caminho legado ausente. |
| R13 | Não implementada | Mark sets fora de F-001 conforme ADR; sem `F-001x`. |
| E5/E6 | Implementadas | Retângulo vazio ⇒ `applyTo == 0`; testado. |
| E1–E4, E7, E8 | Ver acima | E1–E3 F-001a; E4 F-001c; E7 desvio declarado; E8 impossível pelo enum. |

---

## 4. Build limpo

Comando executado (build apagado antes):

```powershell
$cmake = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe\cmake-4.4.4-windows-x86_64\bin\cmake.exe'
Remove-Item -Recurse -Force build
& $cmake --preset mingw-msvcrt
& $cmake --build --preset release-mingw
```

Resultado: **exit 0** nos dois comandos. Configure usou GCC 16.2.0 (WinLibs POSIX MSVCRT) e Qt
6.8.3; `Build files have been written to: H:/Desktop/ArtPlus/build`. Build chegou a
`[100%] Built target integration_test_basic`.

Avisos relevantes: nenhum `warning:`/`error:` de compilação no log. Apenas o aviso de configure
`CMake Warning (unused-cli): Manually-specified variables were not used by the project:
CMAKE_C_COMPILER` — inofensivo, não afeta F-001.

---

## 5. Testes

Comando:

```powershell
$env:PATH = 'C:\Qt\6.8.3\mingw_64\bin;' + $env:PATH
ctest --preset unit-tests
```

Saída (exit 0):

```text
1/7 Test #1: FormXObjects .....................   Passed
2/7 Test #2: PdfSmoke .........................   Passed
3/7 Test #3: UnitTests ........................   Passed
4/7 Test #4: PdfRoundTrip .....................   Passed
5/7 Test #5: EditingTests .....................   Passed
6/7 Test #6: CutMarkTests .....................   Passed    0.17 sec
7/7 Test #7: IntegrationTestBasic .............   Passed

100% tests passed out of 7
Total Test time (real) =   1.55 sec
```

`CutMarkTests` detalhado (o executável é CUI mas não escreve em stdout quando redirecionado; usei
`-o` do QtTest):

```powershell
.\build\cut_mark_test.exe -v1 -o "$env:TEMP\f001_cutmark.txt,txt"
```

```text
PASS   : TestCutMark::initTestCase()
PASS   : TestCutMark::testDefaults_R2_R9()
PASS   : TestCutMark::testGeometryTopology_R7()
PASS   : TestCutMark::testOffsetAndSingleUndo_R5()
PASS   : TestCutMark::testReferenceBoxResolution_R3()
PASS   : TestCutMark::testRegenerationReplaces_R8()
PASS   : TestCutMark::testReferenceByMark_R4_E5()
PASS   : TestCutMark::testPersistence_R10v1()
PASS   : TestCutMark::cleanupTestCase()
Totals: 9 passed, 0 failed, 0 skipped, 0 blacklisted, 16ms
```

Cobertura real: os 7 casos tocam F-001 e fazem asserções exatas, não só "não crasha":
contagem `8` de filhos, `Move+Line` por path, coordenadas com `±0,03 pt` contra uma tabela
independente de `D3`, offset para fora em 3 mm, não-acúmulo em regeneração, `groupId` e 8
segmentos após reload. Se `buildGeometry` deixar de gerar 8 segmentos ou ignorar o offset, os
casos 2, 3, 4, 5, 6 e 7 falham. **O que o teste não prova:** espessura/cor do traço, ordem
"inserido no topo" e o ciclo undo→redo da **criação** (D1).

---

## 6. Divergências

### D1 — BLOQUEANTE: redo da criação perde o registro da marca (não documentado)

`src/prepress/cut_mark.cpp:385-398` (`onMarkRestored`) só atualiza marcas já presentes; não
reinsere uma marca ausente. Em `:497-504`, o undo da criação remove a marca do manager; em
`:506-513`, o redo reata o grupo ao documento, mas a chamada a `onMarkRestored(..., present=true)`
não encontra entrada e não a recria.

Isso contradiz `docs/decisions/ADR-001-cut-mark.md:298-302` ("o manager resynchronise … after
undo/redo") e deixa um estado em que a geometria está na página mas a marca não está no manager:
`saveToMetadata()` grava `cutMarks: []` e a marca vira path comum no próximo save. Reproduzido com
um probe compilado fora do repositório (`%TEMP%\f001_probe`, fontes do projeto, nenhuma alteração
no repo):

```cpp
// probe.cpp (trecho)
CutMark* m = mgr.addCutMark(page->id());
ObjectId gid = m->objectId();
hist.undo(doc);
hist.redo(doc);
printf("mgr=%d docGroup=%d metadataMarks=%d\n", ...);
```

Saída:

```text
after create: mgr=1 docGroup=1 canUndo=1
after undo:   mgr=0 docGroup=0 canRedo=1
after redo:   mgr=0 docGroup=1 metadataMarks=0
```

Fonte completa do probe (gravada em `%TEMP%\f001_probe\probe.cpp`; nenhuma alteração no repo):

```cpp
#include <cstdio>
#include <QJsonArray>
#include <QJsonObject>
#include "src/prepress/cut_mark.h"
#include "src/document/document_model.h"
#include "src/document/undo.h"
using namespace ArtPlus;
int main() {
    Document doc;
    auto layer = std::make_shared<Layer>("L1");
    doc.addLayer(layer);
    auto page = std::make_shared<Page>();
    QRectF r(0, 0, 595.276, 841.89);
    page->boxes().mediaBox = r; page->boxes().cropBox = r; page->boxes().bleedBox = r;
    page->boxes().trimBox = r;  page->boxes().artBox = r;
    page->addLayer(layer->id()); doc.addPage(page);
    UndoStack hist; CutMarkManager mgr(doc); mgr.setUndoStack(&hist);
    CutMark* m = mgr.addCutMark(page->id());
    if (!m) { printf("FAIL create\n"); return 1; }
    ObjectId gid = m->objectId();
    printf("after create: mgr=%d docGroup=%d canUndo=%d\n",
           (int)mgr.cutMarks().size(), doc.object(gid) ? 1 : 0, hist.canUndo() ? 1 : 0);
    hist.undo(doc);
    printf("after undo:   mgr=%d docGroup=%d canRedo=%d\n",
           (int)mgr.cutMarks().size(), doc.object(gid) ? 1 : 0, hist.canRedo() ? 1 : 0);
    hist.redo(doc);
    printf("after redo:   mgr=%d docGroup=%d metadataMarks=%d\n",
           (int)mgr.cutMarks().size(), doc.object(gid) ? 1 : 0,
           mgr.toMetadata().value("cutMarks").toArray().size());
    return 0;
}
```

Reprodução (sem tocar em `src/`):

```powershell
$dir = Join-Path $env:TEMP 'f001_probe'; New-Item -ItemType Directory -Force $dir | Out-Null
# gravar o probe.cpp acima completo; compilar:
$gxx = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.MSVCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe'
$qt = 'C:\Qt\6.8.3\mingw_64'
& $gxx -std=c++17 -O1 -I H:\Desktop\ArtPlus "-I$qt\include" "-I$qt\include\QtCore" "-I$qt\include\QtGui" `
  "$dir\probe.cpp" H:\Desktop\ArtPlus\src\document\document_model.cpp H:\Desktop\ArtPlus\src\document\undo.cpp `
  H:\Desktop\ArtPlus\src\prepress\cut_mark.cpp "-L$qt\lib" -lQt6Core -lQt6Gui -o "$dir\probe.exe"
$env:PATH = "$qt\bin;" + $env:PATH; & "$dir\probe.exe"
```

Efeito colateral observável: depois de undo+redo da criação, um novo `addCutMark` recalcula
`displayIndex` como 1 e cria outra `Cut Mark 1` (numeração duplicada).

### D2 — R1/§3: `*` no título e painel `Marks` ausentes

- `src/ui/main_window.cpp:601` fixa `setWindowTitle("ArtPlus")`; `updateModifiedIndicator()`
  (`:1488-1492`) só escreve `"Modified"` na status bar. A spec §3/R1 exige `*` no título.
- A ação está no menu **Edit** (`main_window.cpp:688`) e na toolbar (`:741`), não no painel
  `Marks`; não existe `Cut Mark 1` em painel nem barra `Dynamic Marks`.
- O painel está explicitamente adiado em `ADR-001:200-203` ("Fica para o dono da UI"); o `*` no
  título **não** está nessa lista. Registro como lacuna parcial não coberta pela ressalva.

### D3 — Frontmatter da spec inconsistente

`docs/features/F-001.md:6,9,10` ainda traz `status: spec`, `adr: null`, `tests: []`, apesar de
`ADR-001` estar aprovado e `tests/unit/test_cut_mark.cpp` existir com `CutMarkTests` verde. A
rastreabilidade declarada no próprio arquivo não fecha.

### D4 — ADR D3 × código: caixa da referência por marca

`docs/decisions/ADR-001-cut-mark.md:143-148` diz que `ReferenceKind::Mark` usa
`Document::object(id)->boundingBox()`. O código usa `CutMark::markBoundsImpl`
(`src/prepress/cut_mark.cpp:141-156`), que calcula o retângulo a partir dos **8 segmentos da
própria marca**; `GroupObject::boundingBox()` é vazio em `src/document/document_model.cpp:140-142`.
O comentário em `src/prepress/cut_mark.h:122-126` admite o desvio, mas o ADR não foi atualizado.
Consequência: com offset ≠ 0 a caixa de referência cresce `off+len` em cada lado, o que pode não
ser o `GetPDFPlusBoundingBox` do original (não medido). Não é falha comprovada, é divergência de
contrato documental.

### D5 — Spec R7 fala em 4 grupos; código cria 1 grupo com 8 paths

`docs/features/F-001.md:191-194` espera "1 grupo de marcas em cada um dos 4 vértices (4 grupos)".
`src/prepress/cut_mark.cpp:204-215` cria **um** `GroupObject` com 8 `PathObject`. O ADR D3
(`:139-142`) escolheu 1 grupo, mas a spec não foi corrigida; o teste (`:168`) valida a escolha do
ADR, não o texto da spec.

### D6 — Manifest incompleto em relação à spec §7

`rag/evidence/F-001/manifest.json:8-15` lista 6 IDs; a tabela de rastreabilidade
(`docs/features/F-001.md:353-366`) cita 15, incluindo `0x18013D160`, `0x1800ADB10`,
`0x180172550`, `0x18024D3E0`, `strings`, `vtables_rtti`, `identity` e `Document__GetPageBox`.
Os 14 arquivos existem (verificado), então não há elo falso; o manifest é que está subdeclarado.

### D7 — R13 sem dono de continuação

`0x18024D3E0` (`ApplyMarksFromSet`) não tem implementação. O ADR o coloca como "mark sets fora de
F-001" (`ADR-001:339`), sem `F-001a/b/c` nem spec de painel. É aceitável como fora de escopo, mas
fica sem rastreabilidade de continuação.

### D8 — Código morto / cobertura fina (não bloqueante)

- `src/prepress/cut_mark.h:106` declara `applySide`, que não é definido nem chamado em
  `cut_mark.cpp`. Não é stub em caminho principal (não linka), mas é resíduo.
- `tests/unit/test_cut_mark.cpp` não verifica `params.color`, `strokeWidthPt` nem a ordem de
  inserção no topo da camada; a fidelidade visual (5 mm / 0,25 pt / magenta) continua sem prova.
  O ADR registra essa limitação (`:380-383`) e o código marca `TODO(F-001)` em
  `cut_mark.h:47-49`, então é ressalva declarada, não stub silencioso.

### D9 — Sem comparação com o original

`manifest.bloqueios` confirma que nenhum PDF de saída foi gerado (`Save As` falhou) e que
`Orientation`/`Repetition` não foram reproduzidos. A única evidência comportamental é visual
(2 recortes de canto). Não há como provar paridade de geometria/valores contra o ArtPro+; a
spec §8 e o ADR já registram isso. Fica como limitação de evidência, não como divergência de
código.

---

## 7. Escopo v1 — honestidade

Explicitamente adiado e documentado:

- **F-001a**: escrita das chaves PDF+ e parser/validações E1–E3 (ADR D4, linhas 184-191).
- **F-001b**: caminho legado `AIDynamicMarks-CutMark` (ADR D4, linhas 193-195).
- **F-001c**: fórmula `fx` e fallback de fórmula inválida (ADR D4, linhas 197-198).
- **Painel `Marks` / barra `Dynamic Marks` / guia vermelha**: adiado para "dono da UI" (ADR
  D4, linhas 200-203), porém sem spec/ID de continuação.
- **R13 mark sets**: "fora de F-001" (ADR mapeamento), sem ID.
- **E7 fallback de caixa**: desvio declarado em `ADR-001:388-390`.
- **Geometria/cor**: inferência `TODO(F-001)` declarada (`cut_mark.h:46-49`,
  `ADR-001:380-383`).

O que **não** está coberto por ressalva: D1 (redo da criação) e o `*` do título em D2.

---

## 8. O que falta para aprovar

- [ ] **D1** — reinscrever a marca no `CutMarkManager` no redo da criação (ou snapshot que
      inclua a entrada do manager) e adicionar teste `add → undo → redo → saveToMetadata`
      verificando `cutMarks.size() == 1` e `doc.object(gid) != nullptr`.
- [ ] **D1** — teste de que `addCutMark` após undo/redo não duplica `Cut Mark 1`.
- [ ] **D2** — implementar `*` no título (ou registrar explicitamente no ADR que o título faz
      parte da UI adiada); manter R1 como parcial no status.
- [ ] **D3** — atualizar `docs/features/F-001.md` (`adr: ADR-001`, `tests: [CutMarkTests]`,
      `status` coerente) — dono: `documentacao`.
- [ ] **D4** — atualizar o ADR D3 para descrever a caixa por segmentos (ou mudar o código para
      `boundingBox()` quando o core deixar de ser stub).
- [ ] **D5** — alinhar spec R7 e ADR D3 sobre "4 grupos" × "1 grupo + 8 paths".
- [ ] **D6** — completar `manifest.estatico` com os 15 IDs citados pela spec.
- [ ] **D7** — dar ID de continuação a R13 (F-001d/spec de mark sets) ou remover R13 do escopo
      declarado de F-001.
- [ ] **D8** — remover `applySide` não definido; opcional: assertar `color`, `strokeWidthPt`
      e posição do grupo no topo da camada.

Nada acima exige refazer a geometria, a persistência `.artplus` ou o manager; o núcleo
R2/R3/R5(edição)/R7/R8/R9/R10 está implementado, testado e compila do zero. O bloqueio é o
contrato de undo/redo da criação (D1) e a inconsistência documental D2/D3.

---

## 9. Comandos e saídas (resumo reproduzível)

```powershell
# evidência
Get-FileHash 'C:\Esko\bg_prog_artpro+_v250\ArtPro+.app\Contents\Windows\DocumentMarks.dll' -Algorithm SHA256
# FFA8809D294A7AA08BD2794E0B0C1E64608D898B5F7BA698A4E58A8E575BEE10
Get-FileHash 'H:\Desktop\ArtPlus\tools\reference_capture\F-001\input\test_vector.pdf' -Algorithm SHA256
# ADFD91B4E95BCE3708DB37055885C355880068EE53A1472AB1DC82E0820640D7

# build
$cmake = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe\cmake-4.4.4-windows-x86_64\bin\cmake.exe'
Remove-Item -Recurse -Force build
& $cmake --preset mingw-msvcrt          # exit 0
& $cmake --build --preset release-mingw # exit 0

# testes
$env:PATH = 'C:\Qt\6.8.3\mingw_64\bin;' + $env:PATH
ctest --preset unit-tests               # exit 0 · 7/7 · CutMarkTests Passed
.\build\cut_mark_test.exe -v1 -o "$env:TEMP\f001_cutmark.txt,txt"
# Totals: 9 passed, 0 failed, 0 skipped, 0 blacklisted, 16ms
```

Hashes dos artefatos auditados:

```text
49B3EC0836C32C856DEC1263617AD4EAECA20D3F7FC8AD9A97EE9C05E141073D  src/prepress/cut_mark.cpp
2BA99B22067D07932AE69BBD9FB0E3FB8A833ADBDD048EEA653C5E43EBD4E3C8  src/prepress/cut_mark.h
91B7A1E9ED29C1460328207AED20A1CE98F991DC739A68EA3D48807C86C8DB18  tests/unit/test_cut_mark.cpp
670CCBE71802CD19C4E6D8184724C8EF900DA8016ECB0C1075CAE54D4575FF2A  src/ui/main_window.cpp
```
