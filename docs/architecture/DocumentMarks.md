# DocumentMarks — marcas dinâmicas do ArtPlus

Módulo: `DocumentMarks.dll` (original) → `src/prepress/cut_mark.h/.cpp` e
`src/prepress/bearer_bar.h/.cpp` (ArtPlus, v1; módulo nomeado `DocumentMarks`, arquivos
batizados pela funcionalidade — ver ADR-001 D1 e ADR-002 D1).

Escopo desta visão: o subsistema de **marcas dinâmicas** — marcas que existem como entidade no
documento, com parâmetros próprios, referência a caixa de página (ou a outra marca) e geometria
regenerável. v1 implementa **Cut Mark** (F-001, ADR-001) e **Bearer Bar** (F-003, ADR-002).

Documentos relacionados:

- Specs: `docs/features/F-001.md`, `docs/features/F-003.md`
- ADRs: `docs/decisions/ADR-001-cut-mark.md`, `docs/decisions/ADR-002-bearer-bar.md`
- Evidência: `rag/evidence/F-001/` (`ghidra/`, `behavior/`), `docs/behavior/TrimMarks.md`;
  `rag/evidence/F-003/` (`ghidra/`, `behavior/`), `docs/behavior/BearerBar.md`
- Índice estático do módulo: `docs/reverse-engineering/DocumentMarks_BEARERBAR_FUNCTIONS.md`

---

## 1. Por que o módulo existe

O ArtPro+ distingue dois universos de marca:

1. **marcas estáticas de preset** — aplicadas em lote a partir de um conjunto de settings;
2. **marcas dinâmicas** — objetos do documento com identidade própria, parâmetros editáveis,
   regeneração a cada mudança e persistência (`DynamicObject_PDFPlusCutMark`).

`MarksEngine` (`src/prepress/marks.h`) cobre o universo 1 (e ainda como stub). O universo 2 tem
ciclo de vida diferente: cria objeto, resolve caixa de referência, apaga e recria geometria em uma
transação, sobrevive a save/load. Este módulo é o dono do universo 2.

## 2. Responsabilidades

| Responsabilidade | Onde |
|---|---|
| Defaults de fábrica da marca (`PageBox=3`, offset 0, `Orientation=1`, `Repetition=2`, `LineTypes=[0]`) | `CutMarkParams` |
| Resolver a caixa de referência (page box ou bounding box de outra marca) | `CutMark::resolveReferenceBox` |
| Gerar a geometria dos quatro cantos a partir de parâmetros + caixa | `CutMark::buildGeometry` |
| Inserir/regenerar a subárvore de objetos na página, substituindo a anterior | `CutMark::applyTo` / `CutMark::regenerate` |
| Garantir **uma** ação de undo por edição do usuário | `CutMarkManager::setParams` + `CutMarkEditCommand` |
| Numerar e nomear marcas (`Cut Mark N`) | `CutMarkManager::addCutMark` |
| Persistir parâmetros e reencontrar a marca no documento | `CutMarkManager::toMetadata/loadFromMetadata` |
| Defaults de fábrica da Bearer Bar (`TrimBox`, `widthMm=5`, `extendMm=0`, `offsetMm=0`, `locationMask=3`, `sizeReference=0`, `direction=1`) | `BearerBarParams` (ADR-002 D2) |
| Gerar as faixas da Bearer Bar por lado habilitado no bitmask `Location` | `BearerBar::buildGeometry` (ADR-002 D3) |
| Inserir/regenerar a Bearer Bar como grupo de paths fechados preenchidos | `BearerBar::applyTo` / `BearerBar::regenerate` |
| Undo único por edição da Bearer Bar | `BearerBarManager::setParams` + `BearerBarEditCommand` |
| Numerar/nomear e persistir Bearer Bars (`Bearer Bar Mark N`) | `BearerBarManager` (array `bearerBars` em `metadata.DocumentMarks`) |
| (F-001a / F-003b) Ler/gravar o dicionário PDF+ da marca | adaptador em `src/pdf`, consumindo `<X>Params` |

## 3. Limites — o que **não** entra aqui

- **Definição de caixas de página.** `PageBoxes`/`PageBoxType` são do núcleo do documento
  (`src/document`). O módulo só lê.
- **Parsing/emissão de PDF.** Fica em `src/pdf`. Em F-001a o adaptador PDF+ traduz
  dicionário ↔ `CutMarkParams`, chamando este módulo; o contrário não acontece.
- **UI de painel/Inspector.** `Marks` panel, barra `Dynamic Marks`, dropdown `Attach to:` com
  crosshair, guia vermelha do Trim Box, rótulos: são UI (`src/ui`), com spec própria. O módulo
  expõe API; não desenha chrome de visualização.
- **Marcas estáticas e presets.** `MarksEngine` continua dono de crop/registration/color bar/
  crease/fold/bleed/... a partir de `MarkSettings`. Nada de identidade ou persistência lá.
- **Mark sets.** `ApplyMarksFromSet` (cópia da primeira página de cada documento do set) é
  funcionalidade de mark set/tarefa (`0x18024D3E0`, spec R13), fora de F-001 e com dono
  declarado: **F-002 — Mark Sets (a especificar)**.
- **Caminho legado `AIDynamicMarks-CutMark`** (clonagem de símbolo, `DM-CutMarkParams`) — F-001b.
- **Avaliação de fórmula (`fx`)** — F-001c; sem motor de expressão em `src/prepress`.
- **Color management da marca** (registro × tinta de separação). v1 usa cor fixa no parâmetro,
  marcada como provisória; resolução por perfil/`ColorManager` é decisão futura.
- **Geometria fechada do PDF+ da Bearer Bar** (transformação de `FormPaint`, anchors, escala e
  guarda de `0,01 mm`) — F-003a. v1 gera faixas retangulares a partir dos parâmetros; a
  espessura (`widthMm`) vem do ctor **legado** e **não** é um campo do
  `DynamicObject_PDFPlusBearerBar` (ADR-002 A2).
- **Roundtrip PDF+/legado da Bearer Bar** (`Position`, `Extends`, `Offset`,
  `Reference`/`ReferenceID`, `DM-BearerBarParams`, `AIDynamicMarks-BearerBar`,
  `IsVertical` v1) — F-003b.
- **Referência por objeto da Bearer Bar** (`ObjectID`, `+0x728 == 1`, E8) — F-003b; v1 só
  resolve caixa de página.
- **Cor `Registration` e filete da Bearer Bar** — decisão de cor + F-003a (ADR-002 A5).

## 4. Dependências

Permitidas:

```
src/prepress/document_marks|cut_mark|bearer_bar
    ├─► src/document/document_model.h   (Document, Page, PageBoxes, PageBoxType,
    │                                    GroupObject, PathObject, Color, FillStroke)
    ├─► src/document/undo.h             (Command, UndoStack)
    ├─► src/document/document_serializer.h   (apenas o formato .artplus, via metadata)
    ├─► src/prepress/marks.h            (MarksUtils::mmToPoints / pointsToMm)
    └─► Qt 6.8.3 Core/Gui               (QVector, QRectF, QLineF, QJsonObject, QColor)
```

`bearer_bar.*` não inclui `cut_mark.h`: as duas marcas são irmãs e não se conhecem
(ADR-002 D1). A ordem de dependência permitida é sempre `prepress → document`.

Proibidas:

| Dependência | Por quê |
|---|---|
| `src/pdf/**` | parser/emissor não pode virar dependência de geometria; direção correta é `pdf → prepress` em F-001a |
| `src/ui/**` | módulo não conhece janela, painel nem ação de menu |
| `src/automation/**` | automação já depende de prepress (`registerMarksEngine`); inverter criaria ciclo |
| `src/color/**` | resolução de cor da marca não está decidida (spec §8 item 9) |
| `MarksEngine` como estado | nenhum estado de marca em `MarksEngine`; só conversão de unidade |
| `src/prepress/cut_mark.h` dentro de `bearer_bar.*` (e vice-versa) | as duas marcas são entidades irmãs; acoplá-las impede a consolidação futura e permite que a edição de uma quebre a outra |

## 5. Fluxo de dados

```
[UI: Marks > Add Cut Mark]                     (F-001 v1: QAction, sem modal)
        │  document, pageId = currentPage()
        ▼
CutMarkManager::addCutMark(pageId)
        │  CutMarkParams{}  → TrimBox, 0,00 mm, Orientation=1, Repetition=2, LineTypes=[0]
        ▼
CutMark::applyTo(doc, page, index)
        │
        ├─ resolveReferenceBox(doc, page)
        │      PageBox → Page::boxes().trimBox/cropBox/...
        │      Mark    → CutMark::markBoundsImpl: união dos 8 segmentos da marca referenciada
        │                (GroupObject::boundingBox() é stub no núcleo ⇒ não é GetPDFPlusBoundingBox)
        │      falha   → QRectF() ⇒ retorna 0, nada é criado
        │
        ├─ buildGeometry(refBoxPt)       mm → pt (MarksUtils::mmToPoints)
        │      4 vértices × 2 segmentos; offset positivo = para fora
        │
        └─ Document::addObject(PathObject × N) + Layer::addObject + GroupObject::addChild
               grupo nomeado "Cut Mark N", inserido no topo da camada de topo
        ▼
UndoStack::push(CutMarkEditCommand)          uma ação por criação/regeneração
        ▼
CutMarkManager::saveToMetadata()  → Document::setMetadata(...)
        ▼
DocumentSerializer::toJson  → root["metadata"]["DocumentMarks"]
```

Edição de parâmetro (offset hoje; `Attach to:`/orientation/repetition quando houver UI):

```
setParams(groupId, params)
  → captura snapshot da subárvore (clone)
  → regenerate: apaga subárvore, resolve de novo, recria
  → CutMarkEditCommand(before, after) no UndoStack
```

Regeneração **substitui**, nunca acumula: a contagem de objetos da marca é invariante
(8 `PathObject` + 1 `GroupObject` na geometria v1).

### 5.1 Fluxo da Bearer Bar (F-003, ADR-002)

```
[UI: Marks > Add Bearer Bar Mark]              (v1: QAction, sem modal, E10)
        │  document, pageId = currentPage()
        ▼
BearerBarManager::addBearerBar(pageId)
        │  BearerBarParams{} → TrimBox, width 5 mm, extend 0, offset 0,
        │                      locationMask 0x3 (l+r), sizeReference 0, direction 1
        ▼
BearerBar::applyTo(doc, page, displayIndex)
        │
        ├─ resolveReferenceBox(doc, page)     PageBox → Page::boxes(); vazio ⇒ 0
        ├─ buildGeometry(refBoxPt)            1 retângulo de faixa por bit de locationMask,
        │                                     face interna a `offset`, espessura `width`,
        │                                     ± `extend` em cada ponta (ADR-002 D3)
        └─ GroupObject("Bearer Bar Mark N") + N PathObject fechados/preenchidos
               inseridos no topo da camada de topo da página
        ▼
UndoStack::push(BearerBarEditCommand)          uma ação por criação/regeneração
        ▼
BearerBarManager::saveToMetadata()  → mescla "bearerBars" em metadata.DocumentMarks
```

Edição de parâmetro (sem UI em v1; `setParams` já transacional):

```
setParams(groupId, params)
  → snapshot da subárvore → regenerate (apaga, resolve de novo, recria)
  → BearerBarEditCommand(before, after) no UndoStack
```

## 6. Modelo de dados e persistência

### 6.1 Em memória

| Objeto | Tipo no documento | Papel |
|---|---|---|
| Marca | `CutMark` (valor, vive em `CutMarkManager::m_marks`) | parâmetros + id do grupo + índice de exibição |
| Marca Bearer Bar | `BearerBar` (valor, vive em `BearerBarManager::m_bars`) | parâmetros + id do grupo + índice de exibição |
| Geometria | `GroupObject` (`Cut Mark N` / `Bearer Bar Mark N`) + `PathObject`s | conteúdo visível, serializado pelo pipeline normal |
| Parâmetros | `Document::metadata()` | sobrevive ao `.artplus`; núcleo do documento não interpreta |

Unidades: parâmetros de UI em **mm** (`offsetMm`, `lengthMm`, `widthMm`, `extendMm`),
espessura de traço em **pt** (`strokeWidthPt`), geometria em **pontos de documento** (mesma
unidade de `PageBoxes`, herdada do importador PDF). Conversão só na geração de geometria. O
miliponto da spec é o domínio interno do binário; em C++ `pt = mm × 72/25,4` e `mp = pt × 1000`,
sem terceiro tipo de unidade.

### 6.2 `.artplus` v1

```json
"metadata": {
  "DocumentMarks": {
    "version": 1,
    "cutMarks": [
      { "groupId": 41, "name": "Cut Mark 1", "displayIndex": 1,
        "kind": "DynamicObject_PDFPlusCutMark",
        "reference": { "kind": "PageBox", "box": "TrimBox" },
        "offsetMm": 0.0, "offsetFormula": "", "orientation": 1, "repetition": 2,
        "lineTypes": [0], "lengthMm": 5.0, "strokeWidthPt": 0.25 }
    ]
  }
}
```

Regras de carga: `groupId` inexistente ⇒ registro descartado; chave ausente ⇒ documento sem
marcas; `format`/`version` do `.artplus` não mudam (chave aditiva, tolerada por leitores antigos).

### 6.2b `.artplus` v1 — Bearer Bar (ADR-002 D4)

```json
"metadata": {
  "DocumentMarks": {
    "version": 1,
    "cutMarks": [ /* ADR-001 D4 */ ],
    "bearerBars": [
      { "groupId": 57, "name": "Bearer Bar Mark 1", "displayIndex": 1,
        "kind": "DynamicObject_PDFPlusBearerBar",
        "reference": { "kind": "PageBox", "box": "TrimBox" },
        "widthMm": 5.0, "extendMm": 0.0, "offsetMm": 0.0,
        "locationMask": 3, "sizeReference": 0, "direction": 1 }
    ]
  }
}
```

**Escritor único do objeto `DocumentMarks`.** `cutMarks` e `bearerBars` são fatias do **mesmo**
objeto JSON, cada uma de um manager. Nenhum manager pode substituir o objeto inteiro: ambos
leem o objeto existente, gravam **somente a própria chave** e preservam as demais e o
`version`. Antes de ADR-002, `CutMarkManager::saveToMetadata` fazia
`meta["DocumentMarks"] = toMetadata()`, o que apagava qualquer outra fatia; a correção
(mesclagem) é obrigatória e coberta pelo teste de regressão 7b de ADR-002 D8.

Regras de carga da Bearer Bar: `kind` presente e diferente de `DynamicObject_PDFPlusBearerBar`
⇒ descartado; numérico ausente ⇒ default de ADR-002 D2; `bearerBars` ausente ⇒ zero bearer
bars; `groupId` órfão ⇒ descartado.

### 6.3 PDF+ (F-001a, ainda não implementado)

Chaves mapeadas pela spec: `DynamicObject_PDFPlusCutMark`, `Position`, `Reference`,
`Orientation`, `Repetition`, `LineTypes`, `PDFPlusCutMark_Offset`,
`DocumentMarks_CutMark_ReferenceID`, `PDFPlusCutMark_OriginalReferenceName`. Nada disso é
escrito nem lido em v1 — a serialização de saída nunca foi observada (spec §8 item 7). O
adaptador deve viver em `src/pdf` e depender de `CutMarkParams`, nunca o inverso.

Bearer Bar (F-003b, ainda não implementado): `DynamicObject_PDFPlusBearerBar`, `Position`
(`SourceAnchor`, `ReferenceAnchor`, `Reference`, `ReferenceID`), `Offset` como array
`[valor, 0.0]` (componente vertical sempre `0.0`, E2), `Extends`, `Reference` do dicionário de
tamanho com os 8 nomes de `SizeReference`, `DocumentMarks_BearerBar_ReferenceID`,
`PDFPlusBearerBar_OriginalReferenceName`; caminho legado `DM-BearerBarParams` (`Version = 2`,
`Width`/`Extend`/`Offset` em mm, `Location`) e `AIDynamicMarks-BearerBar`. Nada disso é escrito
nem lido em v1 (Saída nunca observada, spec §8 item 2). `widthMm` **não** tem chave PDF+: no
PDF+ a largura vem do `FormPaint` escalado (ADR-002 A2).

## 7. Invariantes

1. Geometria é função pura de `(params, referenceBox)`; nenhum estado acumulado entre
   regenerações.
2. Toda edição do usuário gera **uma** entrada de undo.
3. Não existe marca sem grupo: `groupId == 0` significa "nunca aplicada" e não entra em metadados.
4. `PageBox = 3` corresponde a `TrimBox` por inferência (ADR-001 A1 / ADR-002 A1) — um único
   ponto de verdade por tipo (`CutMarkParams::referenceBox`, `BearerBarParams::referenceBox`
   + mapeamento em F-001a/F-003b).
5. `orientation`, `repetition` e `lineTypes` são carregados/persistidos e **não** influenciam
   geometria enquanto a semântica não estiver confirmada (ADR-001 A2); o mesmo vale para
   `sizeReference` e `direction` da Bearer Bar (ADR-002 A3).
6. Caixa/objeto de referência inválido não produz exceção nem objeto parcial: retorna vazio e
   `applyTo == 0` (spec E5/E6).
7. Coordenadas comparadas em testes com `±0,03 pt` (≈ `0,01 mm`); topologia comparada
   exatamente.
8. Bearer Bar: `locationMask == 0` gera zero faixas e nenhum erro (E6); o número de faixas é
   `popcount(locationMask & 0xF)`.
9. Bearer Bar: a espessura vem de parâmetro de geometria (`widthMm`, default legado 5 mm) e
   **nunca** é exportada como campo do PDF+ (ADR-002 A2).
10. `metadata.DocumentMarks` tem um escritor por fatia: `cutMarks` e `bearerBars` são gravados
    por mesclagem, jamais por substituição do objeto (ADR-002 D4).

## 8. Pontos de extensão

- **Novas marcas dinâmicas** (dimensions, position, external file — irmãs no menu `Marks`,
  spec §3): cada uma entra como `struct <X>Params` + `class <X>` no módulo, seguindo o contrato
  `resolveReferenceBox` / `buildGeometry` / `applyTo` / `regenerate` / `toMetadata` /
  `setParams`, com manager próprio e uma fatia nova em `metadata.DocumentMarks`.
  **Decisão vigente (ADR-002 D1):** com a chegada da segunda marca optou-se por managers
  **irmãos** (`CutMarkManager`, `BearerBarManager`), não por `DynamicMarksManager`. Gatilho para
  consolidar: a terceira marca dinâmica **ou** a primeira necessidade real de listar marcas
  mistas (painel `Marks`); a consolidação extrai `dynamic_mark_edit_command.h` + manager comum
  sob ADR própria, sem mudar o formato `.artplus`.
- **Novos tipos de referência**: `ReferenceKind` é o único ponto a estender (ex.: referência por
  objeto arbitrário, `AllStructuralDesignStations` do PDF+); na Bearer Bar isso é F-003b.
- **Novos campos de parâmetro**: adicionar em `<X>Params` + `toMetadata`; nunca em
  `MarksEngine`.
- **Import/export PDF+**: adaptador em `src/pdf` que converte dicionário ↔ `<X>Params`,
  com as validações E1–E3 no lado do importador (F-001a / F-003b).

## 9. Comportamento em falha

| Situação | Resultado |
|---|---|
| Sem página corrente / página inexistente | `addCutMark` → `nullptr`; nada criado (E9, não observado — sem mensagem inventada) |
| Caixa de referência vazia | `applyTo` → `0`; nada criado (E5/E6) |
| Caixa ausente na página | `PageBoxes` sempre tem as cinco caixas; importador iguala caixas ausentes, então a marca usa a substituta (efeito de E7, tabela `DAT_1802EF6D8` não modelada) |
| Fórmula de offset inválida | não avaliada em v1 (F-001c) |
| Metadado órfão no `.artplus` | registro descartado na carga; geometria remanescente segue como paths comuns |
| Bearer Bar sem página corrente | `addBearerBar` → `nullptr`; nada criado (E9, não observado) |
| Bearer Bar com caixa de referência vazia | `applyTo` → `0`; nada criado |
| `locationMask == 0` | zero faixas, zero objetos, **sem erro** (E6, provado) |
| `widthMm <= 0` | geometria vazia, `applyTo == 0` (guarda declarada, não medida — ADR-002 A7) |
| `sizeReference` incoerente com `direction` | v1 **não** corrige (campo opaco, sem UI/anchors); predicado `sizeReferenceCoherent` só reporta (F-003b aplica R5) |
| Referência por `ObjectID` inexistente (E8) | não modelada em v1 (sem `ReferenceKind::Mark`) — F-003b |
| Salvar após criar marcas de dois tipos | `cutMarks` **e** `bearerBars` preservados pelo contrato de escritor único (ADR-002 D4, teste 7b) |

## 10. Aberto neste módulo

- Geometria exata do Cut Mark (comprimento, espessura, nº de segmentos, relação com
  `Repetition`/`Orientation`) — spec F-001 §8 itens 1–3; bloqueia paridade visual, não a
  arquitetura.
- `PageBox = 3 = TrimBox` — inferência para as duas marcas; confirmar no Inspector
  (F-001 §8 item 4, F-003 §8 item 3).
- Cor/registro das marcas (F-001 §8 item 9, F-003 §8 item 5) e uso de `ApplyPDFPlusColor`.
- Painel `Marks`/barra `Dynamic Marks` (F-001 §3, F-003 §3): sem ele, R1 de F-001 e de F-003
  não fecham pela UI, e a ordem de exibição entre tipos (F-003 §8 item 12) segue indefinida.
- Roundtrip PDF+ (F-001a / F-003b), legado (F-001b / F-003b), fórmula (F-001c).
- Geometria fechada do PDF+ da Bearer Bar e pipeline de anchors/escala (F-003a).
- Fidelidade visual da Bearer Bar: a espessura `5 mm` vem do ctor legado, não do PDF+;
  calibração por pixel descartada (F-003 §8 itens 9–10).
