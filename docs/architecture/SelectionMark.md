# SelectionMark — contexto de seleção e edição de objetos no ArtPlus

Módulo: `PDFObjectActions.dll` (original) → `src/prepress/selection_mark.h/.cpp` (ArtPlus, v1).

Escopo desta visão: o subsistema de **Selection Mark** — contexto de seleção que ativa handles visuais sobre objeto existente, permite arraste/transformação/remoção e empilha **uma** ação de undo por edição. v1 implementa **F-004 Selection Mark** (ADR-004).

Documentos relacionados:

- Spec: `docs/features/F-004.md`
- ADR: `docs/decisions/ADR-004-selection-mark.md`
- Evidência: `rag/evidence/F-004/` (`ghidra/`, `behavior/` — a criar), `docs/behavior/SelectionMark.md` (a criar)
- Módulo irmão: `docs/architecture/DocumentMarks.md` (Cut Mark, Bearer Bar)

---

## 1. Por que o módulo existe

O ArtPro+ distingue dois universos de "marca":

1. **Marcas dinâmicas de conteúdo** (Cut Mark, Bearer Bar, Dimensions, Position, External File) — criam **objetos de conteúdo** no documento (`GroupObject` + `PathObject`s), com identidade própria, parâmetros editáveis, regeneração e persistência como `DynamicObject_PDFPlus*`.
2. **Selection Mark** — **não cria objeto de conteúdo**. Opera sobre o **contexto de seleção** já existente: ativa handles visuais no objeto selecionado, permite manipulação (mover, transformar, apagar) e persiste apenas o **estado da seleção** (qual objeto, transformação before/after) no `.artplus`.

`SelectionMarkManager` é o dono do universo 2. Ciclo de vida diferente: ativa contexto → captura snapshot → usuário edita → commit/cancel → **uma** entrada de undo → persiste estado em metadados. Não há geometria própria para regenerar.

---

## 2. Responsabilidades

| Responsabilidade | Onde |
|---|---|
| Ativar Selection Mark no objeto sob cursor ou primeiro selecionado (sem modal) | `SelectionMarkManager::activateSelectionMark()` |
| Validar alvo: `Path`/`Text`/`Image`/`Group` sim; `ClippingMask`/`OpacityMask` não (E3) | `activateSelectionMark()` |
| Camada oculta/bloqueada ⇒ seleção silenciosa (E2) | `activateSelectionMark()` |
| Capturar `beforeTransform` na ativação | `SelectionMarkParams::beforeTransform` |
| Atualizar transformação durante arraste (sem undo ainda) | `SelectionMarkManager::updateTransform()` |
| Commit: se transformação mudou ⇒ `SelectionMarkEditCommand` (uma ação) | `SelectionMarkManager::commitOrCancel()` |
| Delete: `DeleteObjectsCommand` (uma ação) | `SelectionMarkManager::commitOrCancel(deleted=true)` |
| Cancel (Esc, clique vazio) ⇒ sem undo, restaura posição | `SelectionMarkManager::cancel()` |
| Handles visuais: sinal `handlesActiveChanged(bool, ObjectId, PageId)` para UI | `SelectionMarkManager` (emite), `src/ui` (desenha) |
| Status bar: `"Selection Mark ativo"` / `"Selection desativado"` | `ArtPlusMainWindow` handler |
| Persistir estado ativo no `.artplus` (`metadata.SelectionMark`) | `SelectionMarkManager::saveToMetadata()` / `loadFromMetadata()` |
| Convivência com `DocumentMarks` (mesclagem, não substituição) | `saveToMetadata()` mescla chave `SelectionMark` |

---

## 3. Limites — o que **não** entra aqui

- **Desenho de handles.** São **puramente visuais** (spec §6: "Só visual (não gravado no PDF)"). UI (`src/ui`) desenha ao receber sinal; este módulo expõe `targetObjectId`, `pageId`, `handlesActive`.
- **Seleção bruta (rubber-band, clique, Shift-clique).** `Document::selectObject()`, `selectedObjects()`, `clearSelection()` são do núcleo (`src/document`). Selection Mark **usa** a seleção bruta; não a reimplementa.
- **Snapping a grades/caixas/objetos.** Fora de v1 (spec §8 não menciona). Se vier, entra como parâmetro opcional em `updateTransform` ou policy separada.
- **Coordenadas numéricas / Inspector.** Spec E10: "nenhum campo numérico próprio". Fora de v1.
- **Múltiplos objetos simultâneos.** Spec §2: "objeto selecionado" (singular). v1 ativa no primeiro/objeto sob cursor. Múltiplo fica para spec futura.
- **Parsing/emissão de PDF.** Selection Mark **não** aparece no PDF (spec §6). Nenhum adaptador PDF+.
- **Color management.** Handles usam cor de UI fixa; não envolve `src/color`.
- **Automação / Action Lists.** `ActionListEngine` já depende de `prepress` via `registerMarksEngine`; inverter criaria ciclo. Selection Mark não expõe ações graváveis em v1.

---

## 4. Dependências

Permitidas:

```
src/prepress/selection_mark
    ├─► src/document/document_model.h   (Document, Page, PageId, ObjectId, ObjectType,
    │                                    Document::selectedObjects, selectObject, metadata)
    ├─► src/document/undo.h             (Command, UndoStack, TransformCommand,
    │                                    DeleteObjectsCommand)
    └─► Qt 6.8.3 Core/Gui               (QTransform, QJsonObject, QVector, std::optional)
```

Proibidas:

| Dependência | Por quê |
|---|---|
| `src/pdf/**` | Selection Mark não tem representação PDF; direção correta seria `pdf → prepress` se houvesse |
| `src/ui/**` | módulo não conhece janela, painel, handle drawing; emite sinal, UI consome |
| `src/color/**` | cor de handle é decisão de UI, não de prepress |
| `src/automation/**` | automação já depende de prepress; inverter criaria ciclo |
| `MarksEngine` | Selection Mark não é preset nem marca de conteúdo |
| `CutMarkManager` / `BearerBarManager` | entidades irmãs; acoplá-las impede evolução independente e permite que edição de uma quebre a outra |

---

## 5. Fluxo de dados

### 5.1 Ativação (R1, R3, E1, E3)

```
[UI: toolbar Selection Mark] ou [Ctrl+Shift+S]
         │  document = currentDocument()
         ▼
SelectionMarkManager::activateSelectionMark()
         │
         ├─ page = document->currentPage();  if (!page) return nullptr
         ├─ targetId = objectUnderCursor() ?? first(selectedObjects())
         ├─ if (!targetId) return nullptr                                 (E1)
         ├─ obj = document->object(targetId)
         ├─ if (obj->type() == ClippingMask || OpacityMask) return nullptr (E3)
         ├─ layer = document->layer(obj->layerId())
         ├─ if (!layer->visible() || layer->locked()) → seleção silenciosa (E2)
         ├─ before = obj->transform()
         ├─ params = {targetId, page->id(), true, before, QTransform()}
         ├─ m_selectionMark = make_unique<SelectionMark>(params)
         ├─ document->selectObject(targetId)                              // seleção bruta
         ├─ emit handlesActiveChanged(true, targetId, page->id())         // UI desenha
         ├─ statusBar("Selection Mark ativo")
         ├─ document->setModified(true)
         └─ return m_selectionMark.get()
```

### 5.2 Arraste/edição (R2)

```
[Mouse drag / setas] com handles ativos
         │
         ▼
SelectionMarkManager::updateTransform(delta)
         │
         ├─ if (!m_selectionMark) return false
         ├─ obj = document->object(m_selectionMark->targetObjectId())
         ├─ obj->applyTransform(delta)
         ├─ m_selectionMark->setAfterTransform(obj->transform())
         ├─ emit handlesMoved(targetId, pageId)       // UI redesenha handles
         └─ return true
```

### 5.3 Commit / Cancel (R2, R3)

```
[Mouse release / tecla solta / Delete / clique vazio / Esc]
         │
         ▼
SelectionMarkManager::commitOrCancel(deleted)
         │
         ├─ if (!m_selectionMark) return
         ├─ if (deleted) {
         │     cmd = DeleteObjectsCommand::capture(doc, {targetId})
         │     undoStack->push(cmd)
         │  } else if (afterTransform != beforeTransform) {
         │     cmd = SelectionMarkEditCommand(targetId, beforeTransform, afterTransform)
         │     undoStack->push(cmd)                    // UMA ação
         │  }
         ├─ m_selectionMark->setHandlesActive(false)
         ├─ emit handlesActiveChanged(false, 0, 0)
         ├─ statusBar("Selection desativado")
         ├─ saveToMetadata()
         ├─ document->setModified(true)
         └─ if (!deleted) m_selectionMark.reset()     // limpa estado ativo
```

### 5.4 Persistência (D4)

```
saveToMetadata():
    meta = document->metadata()
    dm   = meta.value("SelectionMark").toObject()   // objeto existente ou vazio
    if (m_selectionMark && m_selectionMark->handlesActive()) {
        dm["version"] = 1
        dm["active"] = true
        dm["targetObjectId"] = targetId
        dm["pageId"] = pageId
        dm["beforeTransform"] = transformToArray(beforeTransform)
        dm["afterTransform"]  = transformToArray(afterTransform)
    } else {
        dm["active"] = false
    }
    meta["SelectionMark"] = dm
    document->setMetadata(meta)

loadFromMetadata():
    dm = document->metadata().value("SelectionMark").toObject()
    if (!dm.value("active").toBool()) return true
    targetId = dm["targetObjectId"].toInt()
    pageId   = dm["pageId"].toInt()
    if (!document->object(targetId) || !document->page(pageId)) return true
    before = arrayToTransform(dm["beforeTransform"].toArray())
    after  = arrayToTransform(dm["afterTransform"].toArray())
    params = {targetId, pageId, true, before, after}
    m_selectionMark = make_unique<SelectionMark>(params)
    document->selectObject(targetId)
    emit handlesActiveChanged(true, targetId, pageId)
    return true
```

> **Regra de escritor único:** `saveToMetadata` **mescla** a chave `SelectionMark` no objeto `metadata` raiz, preservando `DocumentMarks` e quaisquer outras chaves. Nunca faz `meta = toMetadata()`.

---

## 6. Modelo de dados e persistência

### 6.1 Em memória

| Objeto | Tipo | Papel |
|---|---|---|
| Contexto ativo | `SelectionMark` (valor, vive em `SelectionMarkManager::m_selectionMark`) | parâmetros + snapshot before/after + estado handles |
| Objeto alvo | `DocumentObject` existente (`Path`/`Text`/`Image`/`Group`) | manipulado via `transform()`/`applyTransform()` |
| Handles | **não existem no modelo** | desenhados pela UI a partir de sinal |
| Parâmetros | `Document::metadata()` → `metadata.SelectionMark` | sobrevive ao `.artplus`; núcleo não interpreta |

Unidades: transformação em **pontos de documento** (mesma unidade de `PageBoxes`, `QTransform` nativo). Conversão `mm ↔ pt` **não** necessária aqui (diferente de Cut Mark/Bearer Bar).

### 6.2 `.artplus` v1

```json
"metadata": {
  "DocumentMarks": { "version": 1, "cutMarks": [...], "bearerBars": [...] },
  "SelectionMark": {
    "version": 1,
    "active": true,
    "targetObjectId": 42,
    "pageId": 1,
    "beforeTransform": [1, 0, 0, 1, 100, 200],
    "afterTransform":  [1, 0, 0, 1, 150, 250]
  }
}
```

- `active: false` ⇒ Selection Mark inativa (estado padrão).
- `beforeTransform`/`afterTransform`: array de 6 doubles = `QTransform(m11, m12, m21, m22, dx, dy)`.
- `targetObjectId` inexistente ou `pageId` inválido na carga ⇒ `active = false`, registro descartado.
- `format`/`version` do `.artplus` **não mudam**; chave opcional.

---

## 7. Invariantes

1. **No máximo uma Selection Mark ativa por documento** (`m_selectionMark` é `unique_ptr`).
2. **Uma ação de undo por edição do usuário** (arraste completo → commit, ou Delete). Micro-movimentos não geram entradas.
3. **Handles não persistem** — só visual, sinal `handlesActiveChanged`.
4. **Seleção bruta e Selection Mark são camadas distintas**: `Document::selectedObjects()` pode ter N objetos; Selection Mark ativa em **um** (o alvo).
5. **Camada oculta/bloqueada (E2)**: objeto entra na seleção bruta e handles são desenhados, mas `commitOrCancel` sem `beforeTransform` válido não empilha undo de transformação (apenas Delete funciona se usuário insistir).
6. **`ClippingMask`/`OpacityMask` (E3)** nunca são alvo — `activateSelectionMark` retorna `nullptr` silenciosamente.
7. **Convivência com DocumentMarks**: `metadata.SelectionMark` e `metadata.DocumentMarks` coexistem; cada manager mescla sua chave.
8. Tolerância de teste: transformação comparada com `±0,03 pt` (≈ `0,01 mm`), igual a Cut Mark/Bearer Bar.

---

## 8. Pontos de extensão

- **Snapping/grade**: adicionar `SnapPolicy* m_snapPolicy` em `SelectionMarkManager`, consultado em `updateTransform`. Não quebra API.
- **Múltiplos objetos**: `SelectionMarkParams::targetObjectIds` (vetor) + `beforeTransforms` (vetor). Exige `MultiTransformCommand` novo. Gatilho: spec futura.
- **Coordenadas numéricas/Inspector**: expor `targetObjectId` + `transform()` via API; UI cria painel. Não muda persistência.
- **Integração Action Lists**: gravar `activateSelectionMark` + `updateTransform` + `commitOrCancel` como macro. Requer `ActionListEngine` conhecer `SelectionMarkManager` (direção `automation → prepress`, ok).
- **Estados de handle customizados** (rotação, escala não-uniforme): `SelectionMarkParams` ganha `handleState` (opaco); UI interpreta. Persistência já suporta via metadados.

---

## 9. Comportamento em falha

| Situação | Resultado |
|---|---|
| Sem página corrente / página inexistente | `activateSelectionMark` → `nullptr`; nada criado, sem mensagem (E1-like) |
| Nenhum objeto selecionado / sob cursor | `nullptr`; sem mensagem (E1) |
| Objeto é `ClippingMask`/`OpacityMask` | `nullptr`; sem mensagem (E3) |
| Objeto em camada oculta/bloqueada | Seleção silenciosa: `selectedObjects()` inclui, handles desenhados, edição não persiste até desbloquear (E2) |
| `targetObjectId` órfão no `.artplus` | `loadFromMetadata` → `active = false`, registro descartado |
| `pageId` inválido no `.artplus` | idem |
| `commitOrCancel` sem seleção ativa | no-op |
| `updateTransform` sem seleção ativa | `false` |

---

## 10. Aberto neste módulo

- **Evidência comportamental** — `CAP-YYYYMMDD-nn` ainda não capturada. Validar D3 quando existir.
- **Comportamento exato de E2** — "seleção silenciosa" precisa de definição operacional testável (handles desenham? `updateTransform` aceita? `commit` faz o quê?).
- **Atalho `Ctrl+Shift+S`** — confirmar no ArtPro+ real (spec §8 item 1).
- **Tratamento de máscara (E3)** — confirmar se `ClippingMask`/`OpacityMask` são totalmente excluídos ou têm caminho próprio.
- **Revalidação contra v24.03.10017 (versão-alvo)** — não executada; nenhuma decisão depende dela.
- **Origem do ícone/toolbar** — rótulo "Selection Mark" vem de spec; string não existe no binário (igual a Bearer Bar ADR-002 A12).

---

## 11. Relação com DocumentMarks (Cut Mark / Bearer Bar)

| Aspecto | Cut Mark / Bearer Bar | Selection Mark |
|---|---|---|
| Cria objeto de conteúdo | Sim (`GroupObject` + `PathObject`s) | Não |
| Tem geometria própria | Sim (`buildGeometry`) | Não (handles = visual) |
| Parâmetros geométricos | `offsetMm`, `widthMm`, `lengthMm`, `locationMask`... | Nenhum (só `targetObjectId`, `transform`) |
| Regeneração | `regenerate()` apaga/recria geometria | `commitOrCancel()` aplica/restore transform |
| Persistência | `metadata.DocumentMarks.{cutMarks,bearerBars}` | `metadata.SelectionMark` |
| Numeração | `Cut Mark N`, `Bearer Bar Mark N` | Sem numeração (estado único) |
| Undo | `CutMarkEditCommand` / `BearerBarEditCommand` | `SelectionMarkEditCommand` / `DeleteObjectsCommand` |
| UI | Painel `Marks`, barra `Dynamic Marks` | Toolbar/atalho, handles visuais, status bar |
| PDF+ roundtrip | `DynamicObject_PDFPlusCutMark/BearerBar` (F-001a/F-003b) | Não aparece no PDF (spec §6) |

**Consolidação:** `docs/architecture/DocumentMarks.md` §8 previa consolidar managers na terceira marca dinâmica ou painel `Marks`. Selection Mark **não** é marca dinâmica de conteúdo — tem ciclo de vida e modelo disjuntos. **Não** entra na consolidação de `DynamicMarksManager`. Se futuro exigir listar "todas as marcas ativas" no painel, a UI consulta os três managers independentemente.