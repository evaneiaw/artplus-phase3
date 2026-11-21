# Auditoria F-001 — Trim Marks (Cut Mark) — reauditoria

Data: 2026-11-19 · Auditor: auditor
Veredito: **APROVADO COM RESSALVAS**

Base: working tree em `HEAD 43124c874353d2bb8324de49097415f26f7c2683` + alterações não
commitadas (mesmo estado do laudo anterior, já corrigido). Nada foi editado fora de
`docs/audit/`. Laudo anterior: `docs/audit/F-001-audit.md`.

---

## 1. O que mudou desde o laudo anterior

| Item | Estado anterior | Estado verificado agora | Evidência |
|---|---|---|---|
| D1 redo da criação | REPROVADO: manager perdia a marca | **Corrigido**; redo reinsere com parâmetros e `displayIndex` | `src/prepress/cut_mark.cpp:384-404`, `:509-525` |
| D1 testes | ausentes | **3 casos presentes e verdes**; 12/12 em `CutMarkTests` | `tests/unit/test_cut_mark.cpp:573,642,686` |
| D1 mutation test | não havia | **3 casos falham** com a mutação que remove a reinserção | seção 4.2 |
| D2 título `*` | ausente | **Implementado**; save limpa, abrir não herda | `src/ui/main_window.cpp:1490-1500`, `:771,797,819` |
| D3 frontmatter | `status: spec`, `adr: null`, `tests: []` | **`status: tested`, `adr: ADR-001`, `tests: [CutMarkTests]`** | `docs/features/F-001.md:6,9,10` |
| D4 ADR × `markBoundsImpl` | ADR dizia `boundingBox()` | **Corpo da decisão corrigido**; restam 2 linhas obsoletas (ressalva R1) | `ADR-001:143-155`, residual `:334,344` |
| D5 spec R7 | "4 grupos" | **"1 `GroupObject` com 8 `PathObject`"** | `docs/features/F-001.md:192-196` |
| D6 manifest | 6 IDs | **10 `estatico` + `documentos` (4)**; todos os arquivos existem | `rag/evidence/F-001/manifest.json` |
| D7 R13 | fora de F-001 sem ID | **F-002 — Mark Sets (a especificar)** no spec e ADR | `F-001.md:40,293-307,424-428`; `ADR-001:209-213` |
| D8 `applySide` | declarado e não definido | **Removido** (0 ocorrências) | grep em `src/prepress/` |

---

## 2. Evidência re-verificada

| Item | Esperado | Encontrado | OK |
|---|---|---|---|
| SHA-256 do binário | `FFA8809D…BEE10` | `FFA8809D294A7AA08BD2794E0B0C1E64608D898B5F7BA698A4E58A8E575BEE10` | ✅ |
| SHA-256 do input | `ADFD91…40D7` | `ADFD91B4E95BCE3708DB37055885C355880068EE53A1472AB1DC82E0820640D7` | ✅ |
| `ghidra/` | 14 arquivos | 14 arquivos; 10 IDs + 4 `documentos` do manifest todos existentes | ✅ |
| `behavior/` | 21 capturas | 21 PNGs; `docs/behavior/TrimMarks.md` presente | ✅ |
| Manifest | 10 IDs + `documentos` (4) | exatamente 10 + 4; caminhos existem | ✅ |

Hashes dos artefatos auditados:

```text
27BAA1F02072FC0E28445A3D83095388E1EDCCBD672FFB25BC85A1182D7DBAD2  src/prepress/cut_mark.cpp
E6DE8E618FC9D0E321C2640A461AA4AE509E7EC42EE04FEE14CFAE8232D49812  src/prepress/cut_mark.h
CE0FF8A3CB6BB5A66AA45832BE82F78D4486AFAEF3EE184566226AA0E742ADD2  tests/unit/test_cut_mark.cpp
DE81042A9527CF52D89D9FAFA15C0405555C84167CD2DB696C79D9A132E431D0  src/ui/main_window.cpp
AD58D3459B5268FCF504126470197204B656A7C6A5092BE5D7329CA1442A79E2  docs/features/F-001.md
65DCBD374286EB2B08DE9D1581EB4934609AA9EF331DD681987CA39327DE4689  docs/decisions/ADR-001-cut-mark.md
218C40BD48C27AF37C6242E523387B9B723AEC5194337CA8C368719C7DA09CFC  rag/evidence/F-001/manifest.json
```

---

## 3. Build limpo

Comando executado (build apagado antes):

```powershell
$cmake = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe\cmake-4.4.4-windows-x86_64\bin\cmake.exe'
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
& $cmake --preset mingw-msvcrt
& $cmake --build --preset release-mingw
```

Saída (final):

```text
-- Build files have been written to: H:/Desktop/ArtPlus/build
CONFIGURE_EXIT=0
...
[ 93%] Built target cut_mark_test
...
[100%] Built target integration_test_basic
BUILD_EXIT=0
```

- Configure: exit 0; único aviso é `CMake Warning (unused-cli): CMAKE_C_COMPILER` (inofensivo).
- Build: exit 0; **nenhum `warning:` ou `error:` de compilação** no log.
- Toolchain: GCC 16.2.0 (WinLibs POSIX MSVCRT), Qt 6.8.3 mingw_64.

---

## 4. Testes

### 4.1 `ctest` completo

```powershell
$env:PATH = 'C:\Qt\6.8.3\mingw_64\bin;' + $env:PATH
ctest --preset unit-tests
```

Saída real (exit 0):

```text
1/7 Test #1: FormXObjects .....................   Passed    0.22 sec
2/7 Test #2: PdfSmoke .........................   Passed    0.20 sec
3/7 Test #3: UnitTests ........................   Passed    0.19 sec
4/7 Test #4: PdfRoundTrip .....................   Passed    0.29 sec
5/7 Test #5: EditingTests .....................   Passed    0.21 sec
6/7 Test #6: CutMarkTests .....................   Passed    0.18 sec
7/7 Test #7: IntegrationTestBasic .............   Passed    0.24 sec

100% tests passed out of 7
Total Test time (real) =   1.54 sec
CTEST_EXIT=0
```

`CutMarkTests` detalhado (`-o` do QtTest, pois o executável não imprime em stdout quando
redirecionado):

```text
PASS   : TestCutMark::initTestCase()
PASS   : TestCutMark::testDefaults_R2_R9()
PASS   : TestCutMark::testGeometryTopology_R7()
PASS   : TestCutMark::testOffsetAndSingleUndo_R5()
PASS   : TestCutMark::testReferenceBoxResolution_R3()
PASS   : TestCutMark::testRegenerationReplaces_R8()
PASS   : TestCutMark::testReferenceByMark_R4_E5()
PASS   : TestCutMark::testPersistence_R10v1()
PASS   : TestCutMark::testUndoRedoCreation_D1()
PASS   : TestCutMark::testNoDuplicateNumberingAfterRedo_D1()
PASS   : TestCutMark::testUndoRedoCreationSurvivesReload_D1()
PASS   : TestCutMark::cleanupTestCase()
Totals: 12 passed, 0 failed, 0 skipped, 0 blacklisted, 27ms
```

**12/12 confirmado** (10 casos + init/cleanup).

### 4.2 Mutation test (D1) — executado por mim

Não encontrei artefato do mutation test do agente `testes`: `docs/testing/` só contém
`F-001-probe.txt` (antigo, ainda mostra 9 passes e não cobre D1) e `TEST_PLAN.md`. Para não
confiar em relato, repeti a mutação:

- Cópia de `src/prepress/cut_mark.cpp` em `%TEMP%\f001_mut\cut_mark_mutated.cpp` (nenhuma
  alteração no repositório).
- Mutação: `m_marks.append(restored);` → `(void)restored;` (comportamento pré-correção: não
  reinsere no manager).
- Compilação do teste real `tests/unit/test_cut_mark.cpp` (moc + g++/Qt 6.8.3) contra a cópia
  mutada.

Saída real (exit 3):

```text
PASS   : TestCutMark::testDefaults_R2_R9()
PASS   : TestCutMark::testGeometryTopology_R7()
PASS   : TestCutMark::testOffsetAndSingleUndo_R5()
PASS   : TestCutMark::testReferenceBoxResolution_R3()
PASS   : TestCutMark::testRegenerationReplaces_R8()
PASS   : TestCutMark::testReferenceByMark_R4_E5()
PASS   : TestCutMark::testPersistence_R10v1()
FAIL!  : TestCutMark::testUndoRedoCreation_D1() Compared values are not the same
   Actual   (mgr.cutMarks().size()): 0
   Expected (1)                    : 1
H:\Desktop\ArtPlus\tests\unit\test_cut_mark.cpp(601) : failure location
FAIL!  : TestCutMark::testNoDuplicateNumberingAfterRedo_D1() Compared values are not the same
   Actual   (mgr.cutMarks().size()): 0
   Expected (1)                    : 1
H:\Desktop\ArtPlus\tests\unit\test_cut_mark.cpp(656) : failure location
FAIL!  : TestCutMark::testUndoRedoCreationSurvivesReload_D1() Compared values are not the same
   Actual   (mgr.cutMarks().size()): 0
   Expected (1)                    : 1
H:\Desktop\ArtPlus\tests\unit\test_cut_mark.cpp(698) : failure location
Totals: 9 passed, 3 failed, 0 skipped, 0 blacklisted, 16ms
```

Os **3 casos D1 falham exatamente** no código sem a reinserção; os 7 casos anteriores e
init/cleanup continuam verdes. Isso prova que os testes cobrem o defeito, não só o caminho feliz.

---

## 5. Verificação item a item

### D1 — redo da criação

- `src/prepress/cut_mark.cpp:384-404`: após atualizar marcas existentes, se o grupo voltou e a
  entrada não existe, cria `CutMark restored(params)`, restaura `m_groupId`, `m_displayIndex`
  (do comando; fallback `nextDisplayIndex()`), seta o manager e faz `m_marks.append(restored)`.
- `:509-525`: `CutMarkEditCommand::undo/redo` passam `m_displayIndex` ao manager.
- `:337-358`: `addCutMark` usa `nextDisplayIndex()` e grava o índice no comando de criação.
- Testes: `testUndoRedoCreation_D1` cobre criação + edição + undo duplo + redo + metadata +
  save; `testNoDuplicateNumberingAfterRedo_D1` cobre `Cut Mark 2/3`; 
  `testUndoRedoCreationSurvivesReload_D1` cobre `.artplus` após undo→redo.
- Mutação: 3/3 falham sem a correção (seção 4.2). **Atendido.**

### D2 — `*` no título

- `src/ui/main_window.cpp:1490-1500`: título reconstruído a cada chamada:
  `"ArtPlus *"` se `m_document.modified()`, senão `"ArtPlus"`. Sem duplicação de `*`.
- Save: `:817` `setModified(false)` + `:819` `updateModifiedIndicator()` → título limpo.
- Abrir PDF: `:766` `setModified(false)` + `:771` update → não herda `*`.
- Abrir `.artplus`: `DocumentSerializer::fromJson` faz `setModified(false)` e `:797` update.
- Não existe `File > New`; os dois caminhos de documento novo (PDF e `.artplus`) limpam o estado.
- **Atendido por inspeção de código.** Não há teste de UI para o título (ressalva R3).

### D3 — frontmatter da spec

`docs/features/F-001.md:1-11`:

```yaml
status: tested
adr: ADR-001
tests: [CutMarkTests]
```

**Atendido.**

### D4 — ADR descreve `markBoundsImpl`

`ADR-001:143-155` descreve a caixa de referência por marca como união dos 8 segmentos
(`CutMark::markBoundsImpl`), explica que `GroupObject::boundingBox()` é stub e registra a
divergência conhecida em relação a `GetPDFPlusBoundingBox`. `ADR-001:399-406` reforça a
limitação. **Atendido no corpo da decisão**, mas restam duas linhas antigas contraditórias
(ressalva R1).

### D5 — spec R7

`docs/features/F-001.md:192-196`: "existe **1 `GroupObject` com 8 `PathObject`** (4 vértices ×
2 segmentos)…". **Atendido.**

### D6 — manifest

`rag/evidence/F-001/manifest.json`: `estatico` com 10 IDs e `documentos` com 4 caminhos. Todos
os 14 caminhos existem; os 6 originais + `0x18013D160`, `0x1800ADB10`, `0x180172550`,
`0x18024D3E0` estão em `estatico`; `identity`, `strings`, `vtables_rtti` e `GetPageBox` em
`documentos`. **Atendido.**

### D7 — mark sets como F-002

`docs/features/F-001.md:40` e `:293-307` declaram R13 fora de F-001 e pertencente a
**F-002 — Mark Sets (a especificar)**; `:424-428` registra a lacuna com instrução de não criar
`F-002.md` antes da spec. `ADR-001:209-213` idem. **Atendido** no rastreamento de features.
Ressalva R2: F-002 não aparece em `docs/ROADMAP.md`/`INVENTORY.md`/índice de features (por
decisão explícita, ainda sem spec própria).

### D8 — código morto

`applySide` não existe mais em `cut_mark.h` nem em `cut_mark.cpp` (0 ocorrências). As declarações
restantes do header têm definição correspondente; `markBoundsImpl`, `resolveReferenceBoxWith`,
`nextDisplayIndex`, `pageOf`, `pruneDetached` são usados. `TODO(F-001)` remanescentes são os
documentados (fórmula, orientation/repetition, comprimento, espessura). **Atendido.**

---

## 6. Ressalvas (não bloqueantes)

### R1 — Duas linhas obsoletas no ADR ainda dizem `boundingBox()`

- `docs/decisions/ADR-001-cut-mark.md:334` (caso de teste 6, D7): "usa o `boundingBox()`".
- `docs/decisions/ADR-001-cut-mark.md:344` (tabela de mapeamento): "ramo objeto usa
  `DocumentObject::boundingBox()`".
- `docs/decisions/ADR-001-cut-mark.md:7` (contexto): "quatro grupos de linhas finas", enquanto
  D3/D5 usam 1 grupo com 8 paths.

O corpo da decisão (D3) e as consequências estão corretos; são linhas residuais da revisão.
Corrigir na próxima passagem de `documentacao`, sem impacto de código.

### R2 — F-002 sem entrada no roadmap/índice

F-002 é citado na spec F-001 e no ADR, mas não há `docs/features/F-002.md` nem entrada em
`docs/ROADMAP.md`/`INVENTORY.md`/índice de features. A própria spec diz para não criar antes da
spec; fica registrado como dependência de planejamento, não como lacuna de F-001.

### R3 — D2 verificado por leitura, sem teste de UI

Não há teste automatizado que instancie `ArtPlusMainWindow` e verifique `windowTitle()`. A
correção é direta (`setWindowTitle` em `updateModifiedIndicator`), mas a cobertura é por
inspeção. Opcional: teste de integração com `QT_QPA_PLATFORM=offscreen`.

### R4 — Fidelidade visual continua inferência

Comprimento 5 mm, espessura 0,25 pt, cor magenta e 8 segmentos continuam `TODO(F-001)` e não são
comparados pixel a pixel com `crop-corner-tl.png`/`crop-corner-br.png`. O teste valida topologia
e coordenadas; a paridade visual segue não medida (já registrado no ADR).

### R5 — Painel `Marks` / barra `Dynamic Marks` seguem ausentes

R1 é atendida no comando (sem modal, undo, `*` no título), mas não há painel `Marks`, barra
`Dynamic Marks` nem linha `Cut Mark 1` no painel. Continua adiado para spec de painel
(`ADR-001:409-410`), como no laudo anterior.

### R6 — `docs/testing/F-001-probe.txt` está desatualizado

O arquivo registra 9 passes e não inclui os casos D1; foi gerado antes da correção. Não é
evidência do estado atual. O `ctest` e a saída de 12 passes deste laudo são a referência.

---

## 7. O que falta (opcional / próximas fases)

- [ ] Corrigir as 3 linhas residuais do ADR (R1) — `documentacao`.
- [ ] Registrar F-002 no roadmap/índice quando a spec for escrita (R2).
- [ ] Opcional: teste de UI para o `*` do título (R3).
- [ ] F-001a (PDF+ roundtrip/parser), F-001b (legado AI), F-001c (`fx`) — já rastreados.
- [ ] Fidelidade visual (R4) e painel `Marks` (R5) — fora do v1, já declarados.

Nenhuma lacuna central não documentada permanece: o defeito D1 está corrigido e provado por
teste + mutação; D2/D3/D5/D6/D7/D8 atendidos; D4 atendido no corpo da decisão com resíduo
editorial.

---

## 8. Comandos reproduzíveis

```powershell
# build limpo
$cmake = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe\cmake-4.4.4-windows-x86_64\bin\cmake.exe'
Remove-Item -Recurse -Force build
& $cmake --preset mingw-msvcrt           # exit 0
& $cmake --build --preset release-mingw  # exit 0

# suíte completa
$env:PATH = 'C:\Qt\6.8.3\mingw_64\bin;' + $env:PATH
ctest --preset unit-tests                # exit 0 · 7/7 · CutMarkTests Passed
.\build\cut_mark_test.exe -v1 -o "$env:TEMP\f001b_cutmark.txt,txt"
# Totals: 12 passed, 0 failed

# mutation test D1 (cópia em %TEMP%, sem alterar o repo)
# m_marks.append(restored); -> (void)restored; em cut_mark_mutated.cpp
# Totals: 9 passed, 3 failed (os 3 casos *_D1)
```
