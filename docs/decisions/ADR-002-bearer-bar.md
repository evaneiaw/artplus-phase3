# ADR-002 — Bearer Bar (F-003)

## Contexto

A spec `docs/features/F-003.md` (status `spec`, confiança `media`) descreve a marca criada por
`Marks > + > Add Bearer Bar Mark`: **sem diálogo modal**, uma faixa por lado habilitado num
bitmask `Location` (esquerda/direita/topo/base), encostada **por fora** de uma caixa de
referência, com espessura `width`, prolongamento `extend` e afastamento `offset`, persistida
como `DynamicObject_PDFPlusBearerBar` (caminho atual) ou `AIDynamicMarks-BearerBar` (legado).
A captura `CAP-20261119-02` mostra a faixa cinza com filete vermelho na aresta externa, título
com `*`, barra `Dynamic Marks | Bearer Bar Mark 1` e a linha `Bearer Bar Mark 1` acima de
`Cut Mark 1` no painel; **nenhum campo numérico próprio** foi observado (spec §3, E10).

Esta ADR decide a primeira implementação (v1) do objeto no ArtPlus, reusando o padrão já
aprovado em `docs/decisions/ADR-001-cut-mark.md` (F-001) e `docs/architecture/DocumentMarks.md`.

### Evidência estática que sustenta esta ADR

| Evidência | O que fixa |
|---|---|
| `GHIDRA-DocumentMarks-0x18001E3C0` | ctor legado: `width = 14173,2283464567 mp` (**5 mm exatos**), `extend = 0`, `offset = 0`, `location = 3`, `DynamicMark(this, true, 0xFD)` |
| `GHIDRA-DocumentMarks-0x18012FAA0` | ctor PDF+: `Offset = 0` (`+0x698`), `Extend = 0` (`+0x6A0`), `Direction = 1` (`+0x6A8`), referência `PageBox` índice `3` (`+0x6B0`/`+0x728`), `SizeReference = 0` (`+0x730`), `setAnchors(5,3)`, `setColorType(5,0)` |
| `GHIDRA-DocumentMarks-0x18001FDB0` | `BearerBar::regenerateMarkImp`: 1 `Path` por bit de `Location`, inserção após o símbolo de origem, remoção de excedentes |
| `GHIDRA-DocumentMarks-0x18001F870` | parse legado `DM-BearerBarParams` (`Version < 3`, mm → mp `× 360000/127`, `IsVertical` na v1) |
| `GHIDRA-DocumentMarks-0x180020950` | escrita legada `Version = 2`, `Width/Extend/Offset` em mm, `Location` bitmask; sem `IsVertical` |
| `GHIDRA-DocumentMarks-0x180132A70` | `PDFPlusBearerBar::regenerateMarkImp`: direção → anchors → size ref → apaga → `Group`/`Expander` do `FormPaint` → transformação → `InsertNewObjectOnTop` |
| `GHIDRA-DocumentMarks-0x180131C40` | `Direction` derivado dos anchors (0 vertical / 1 horizontal — inferência média) |
| `GHIDRA-DocumentMarks-0x180131DC0` | coerência `SizeReference` 1–4 × `Direction 0`; 5–8 × `Direction 1`; inválido → `0` |
| `GHIDRA-DocumentMarks-0x180131FE0` | `getMarkType()` = `DynamicObject_PDFPlusBearerBar` (30 bytes) |
| `GHIDRA-DocumentMarks-0x18001F690` | `BearerBar::getMarkType()` = `AIDynamicMarks-BearerBar` (24 bytes) |
| `GHIDRA-DocumentMarks-0x180179DA0` | parser PDF+ `Position`: `SourceAnchor`/`ReferenceAnchor`/`Reference`/`ObjectID`, `Offset` array `[valor, 0.0]` |
| `GHIDRA-DocumentMarks-0x180170C20` | `Extends` → `Extend`; 8 nomes de `SizeReference` |
| `GHIDRA-DocumentMarks-0x18016F250` | fábrica PDF+ (`new 0x790` + ctor) |
| `GHIDRA-DocumentMarks-0x1800AA260` / `0x1800AA3C0` / `0x1800AA520` | `SetBearerBarExtend` / `SetBearerBarOffset` / `SetBearerBarSizeReference` transacionais (ActionList/undo) |
| `GHIDRA-DocumentMarks-0x18001E350` | ctor de clone legado; `postProcessClonedMark` `0x1801326E0` |
| `GHIDRA-DocumentMarks-0x180111410` | criação legada: `memcmp "AIDynamicMarks-BearerBar"` → `new (0x6B8) BearerBar(true)` |
| `CAP-20261119-02` | criação sem modal, título `*`, barra de contexto, linha no topo do painel, faixa cinza + filete, ausência de campo numérico |

Código existente relevante (`src/`, leitura apenas nesta etapa):

- `src/prepress/cut_mark.h/.cpp` — `CutMarkParams`, `CutMark`, `CutMarkManager`,
  `CutMarkEditCommand` (padrão completo de marca dinâmica v1: parâmetros → geometria →
  inserção/regeneração → undo único → `.artplus`).
- `src/prepress/marks.h/.cpp` — `MarksEngine` (presets estáticos) e `MarkCutMarkSettings`;
  não existe struct de preset para bearer bar.
- `src/document/document_model.h` — `Document`, `Page`, `PageBoxes`, `PageBoxType`,
  `GroupObject`, `PathObject` (`addMove`/`addLine`/`closePath`/`setClosed`), `Color`,
  `FillStroke` (116-125), `Document::metadata()`/`setMetadata()` (431-433).
- `src/document/undo.h` — `Command`, `UndoStack`.
- `src/document/document_serializer.cpp` — `root["metadata"]` (338/450), `closed` de path
  (166/223).
- `src/ui/main_window.cpp/.h` — `m_cutMarkManager{m_document}` (h:213), `setUndoStack`
  (cpp:610), menu (`688-689`), toolbar `addTool` (`741`), `loadFromMetadata` (`768`, `794`),
  `saveToMetadata` (`812`), handler `addCutMark` (`1064-1078`).

### Lacunas da spec que esta ADR **não** resolve

Registradas em tabela (§ Ambiguidades) em vez de decididas em silêncio: geometria fechada do
PDF+ (§8 item 1), serialização de saída (§8 item 2), `PageBox = 3` (§8 item 3), campos
numéricos/Inspector (§8 item 4), natureza do filete vermelho (§8 item 5), rótulo de `Direction`
(§8 item 6), `ValidateAnchors` (§8 item 7), modelo do retângulo legado (§8 item 8),
medições de captura (§8 itens 9–10), exceções não observadas (§8 item 11), ordem no painel
(§8 item 12), revalidação v24.03.10017 (§8 item 13), `IsVertical` (§8 item 14), origem do literal de menu
(§8 item 15).

## Decisão

### D1 — Onde a funcionalidade vive: `BearerBar` + `BearerBarManager` em `src/prepress/bearer_bar.*`

Novos arquivos `src/prepress/bearer_bar.h/.cpp` com:

- `struct BearerBarParams` — parâmetros da marca;
- `class BearerBar` — parâmetros + geometria + inserção/regeneração;
- `class BearerBarManager` — dono da lista de bearer bars do `Document`, criação pelo comando,
  edição transacional, `.artplus`;
- `class BearerBarEditCommand : Command` — um comando de undo por edição.

**Não** se estende `CutMark`, `CutMarkParams` nem `CutMarkManager`:

1. Os modelos de parâmetro não se sobrepõem: `CutMarkParams` é
   `{PageBox|Mark, offsetMm, fórmula, orientation, repetition, lineTypes, lengthMm,
   strokeWidthPt, color}` (`cut_mark.h:32-65`); a bearer bar precisa de `widthMm`,
   `extendMm`, `locationMask`, `sizeReference`, `direction`. Um struct com os dois conjuntos
   teria campos sempre mortos e `operator==` mentindo sobre igualdade (ADR-001 D2 usa
   igualdade de parâmetros para decidir se há undo).
2. A geometria é de topologia diferente: cut mark ⇒ segmentos (`QVector<QLineF>`, 8 por marca,
   traço); bearer bar ⇒ faixas fechadas preenchidas, 1 por bit de `Location`
   (`0x18001FDB0` cria `Path` por lado e pinta com `Fillable::GetFillPaint`). `applyTo`
   de F-001 falha se `segments.size() != 8` (`cut_mark.cpp:198`) — não há como reusar.
3. `CutMarkManager` indexa `QVector<CutMark>` e resolve `referenceMarkId` por
   `CutMarkManager::find` (`cut_mark.cpp:124`); a bearer bar v1 não tem referência por objeto
   (§ Ambiguidades A6), e a lista de metadados precisa ser por tipo (§ D4).
4. `CutMarkEditCommand` recebe `CutMarkManager*` e chama `onMarkRestored(...)`
   (`cut_mark.h:78-108`, `cut_mark.cpp:509-525`). Reusar exigiria mudar uma classe aprovada e
   auditada de F-001 (`docs/audit/F-001-audit.md`) — ver alternativa descartada 2.

**Não** se cria `MarksEngine`-style preset: a bearer bar é entidade com estado e
regeneração (R2/R7), não preset aplicado em lote (mesma razão de ADR-001 D1).

O que é reutilizado do existente: conversão `mm → pt` (mesmo fator do `CutMark`, derivado de
`MarksUtils`), `Document`/`Page`/`PageBoxes`/`PageBoxType`, `GroupObject` + `PathObject`,
`Color`/`FillStroke`, `Command`/`UndoStack`, `Document::metadata()`, e o padrão
`setUndoStack(UndoStack*)` de `StepRepeatEngine`/`CutMarkManager`.

**Consolidação em `DynamicMarksManager`:** `docs/architecture/DocumentMarks.md` §8 previa
consolidar quando a segunda marca chegasse. Decisão consciente desta ADR: **não consolidar
agora**. Consolidar exige mudar a API pública e os testes de F-001 (aprovado, auditado) e não
destrava nenhuma funcionalidade de F-003; o custo é maior que a duplicação (um manager de
~120 linhas). Gatilho declarado para reabrir: (a) a terceira marca dinâmica, ou (b) a primeira
necessidade real de listar marcas mistas (painel `Marks`) — nesse momento extrai-se
`src/prepress/dynamic_mark_edit_command.h` e um manager comum sob ADR própria.

### D2 — Modelo de parâmetros v1

```cpp
struct BearerBarParams {
    PageBoxType referenceBox = PageBoxType::TrimBox; // PDF+ PageBox=3 (inferência; A1)
    double widthMm       = 5.0;  // espessura da faixa; ctor legado 0x18001E3C0 (A2)
    double extendMm      = 0.0;  // PDF+ +0x6A0 e legado +0x6A0
    double offsetMm      = 0.0;  // PDF+ +0x698 e legado +0x6A8
    int    locationMask  = 0x3;  // ctor legado +0x6B0: 1=l 2=r 4=t 8=b (R4)
    int    sizeReference = 0;    // PDF+ +0x730; 0..8 (R5); opaco em v1
    int    direction     = 1;    // PDF+ +0x6A8; 0=vertical 1=horizontal (A3)
    Color  color = Color::rgb(168.0f/255.0f, 164.0f/255.0f, 164.0f/255.0f); // A5
};
```

Regras do modelo:

- **Defaults** (R2): `widthMm = 5,0` (`0x18001E3C0`: `14173,2283464567 mp` = 5 mm exatos),
  `extendMm = offsetMm = 0`, `locationMask = 3` (`l+r`, barras verticais), `sizeReference = 0`,
  `direction = 1`, referência `PageBox` índice `3` → `TrimBox` por inferência herdada de
  ADR-001 A1 (ver A1: a UI mostra `Trim Box` como ancoragem, spec §3 — corrobora, não prova).
- **Unidades explícitas no nome**: `widthMm`, `extendMm`, `offsetMm` em mm (unidade de UI e de
  persistência `.artplus`); geometria em pontos de documento, a mesma unidade de `PageBoxes`
  (`pdf_importer.cpp:347-351`). O miliponto da spec (R3) é o domínio **interno do binário**;
  em C++ o par `mm`/`pt` já é exato (`pt = mm × 72/25,4`, e `mp = pt × 1000`), então não se
  introduz um terceiro tipo de unidade. Conversão só em `BearerBar::buildGeometry`.
- `widthMm` é **parâmetro de geometria v1 derivado do legado**, não um campo do PDF+.
  `+0x698` no PDF+ é `Offset` (R2, nota de layout; `0x1800AA3C0` ICF com `setWidth`).
  O PDF+ não expõe espessura numérica: a largura aparente vem do conteúdo `FormPaint` de
  origem escalado pela transformação (`0x180132A70` passos 3 e 6). Essa divergência está
  declarada em A2 e **proíbe** exportar `widthMm` como chave PDF+ (D4).
- `locationMask`: apenas os 4 bits baixos têm significado (R4); bits acima são preservados no
  metadado e ignorados pela geração (laço `i = 0..3`).
- `sizeReference` e `direction` são **opacos** em v1: não há UI que os altere e não há modelo
  de anchors para derivar `direction` (`0x180131C40`). O que entra é um predicado puro de
  coerência (D3), usado por teste; a correção silenciosa de R5 pertence a F-003b (A3).
- `color`: provisório cinza RGB `168,164,164` (a cor da faixa em `CAP-20261119-02`, spec §6).
  O default real é a tinta `Registration` (spec §3, campo não confirmado) e a resolução
  `Registration` × separação de tinta depende de decisão de cor (`src/color` é proibido neste
  módulo, `DocumentMarks.md` §4). Marcado `TODO(F-003)`; A5.
- Sem `referenceMarkId` em v1: o ctor PDF+ tem variante `ObjectID` (`+0x728 == 1`,
  `0x180179DA0`) e a UI mostra um dropdown de ancoragem, mas não há caminho de UI capturado
  que escolha objeto e a resolução de `ObjectID` inexistente não foi observada (E8). Entra em
  F-003b junto com `Reference`/`ReferenceID`.

Tabela de `SizeReference` (R5, `0x180170C20`) — carregada como inteiro, sem semântica em v1:

| Valor | Nome no PDF+ | Grupo de eixo | `Direction` exigida |
|---|---|---|---|
| `0` | (não definido) | — | qualquer |
| `1` | `LeftMostGridLayout` | left/right | `0` |
| `2` | `RightMostGridLayout` | left/right | `0` |
| `3` | `LeftMostGridLayoutColumn` | left/right | `0` |
| `4` | `RightMostGridLayoutColumn` | left/right | `0` |
| `5` | `TopMostGridLayout` | top/bottom | `1` |
| `6` | `BottomMostGridLayout` | top/bottom | `1` |
| `7` | `TopMostGridLayoutRow` | top/bottom | `1` |
| `8` | `BottomMostGridLayoutRow` | top/bottom | `1` |

Bitmask de `Location` (R4, `0x18001FDB0`) — ordem = índice do lado na geometria:

| Bit | Valor | Lado | Índice |
|---|---|---|---|
| 0 | `0x1` | esquerda | `0` |
| 1 | `0x2` | direita | `1` |
| 2 | `0x4` | topo | `2` |
| 3 | `0x8` | base | `3` |

### D3 — Geometria v1

`BearerBar::buildGeometry(const QRectF& referenceBoxPt)` devolve **retângulos de faixa**, na
ordem crescente do índice do lado, um por bit ligado em `locationMask`:

```
w = mmToPoints(widthMm);  e = mmToPoints(extendMm);  o = mmToPoints(offsetMm)
(L,T,R,B) = referenceBoxPt            // y cresce para baixo: T < B (PageBoxes = QRectF(0,0,W,H))

lado 0 (0x1, esquerda): x ∈ [L−o−w, L−o]   y ∈ [T−e, B+e]
lado 1 (0x2, direita):  x ∈ [R+o, R+o+w]   y ∈ [T−e, B+e]
lado 2 (0x4, topo):     y ∈ [T−o−w, T−o]   x ∈ [L−e, R+e]
lado 3 (0x8, base):     y ∈ [B+o, B+o+w]   x ∈ [L−e, R+e]
```

Justificativa e limites:

- A faixa fica **fora** da caixa, com a face interna a `offset` da borda e espessura `width`
  ⇒ extensão externa total `offset + width` (R6: "a faixa fica fora da borda"; "espessura =
  width + offset" é a leitura alternativa — ver A4). Com os defaults (`offset = 0`) as duas
  leituras **coincidem**, então o caso observado não é afetado.
- `extend` alonga a faixa `e` em **cada** ponta no eixo do lado (R6: "comprimento = bbox +
  extend em cada ponta"). Confiança média (spec §8 item 8) ⇒ `TODO(F-003)`.
- Cada faixa vira um `PathObject` **fechado e preenchido**: `addMove` + 3 `addLine` +
  `closePath`/`setClosed(true)`, `FillStroke::fill = params.color`, `stroke` inválido
  (`Color::Type::None`) e `strokeWidth = 0`. É o que o original faz: `Path` pintado com
  `Fillable::GetFillPaint` (`0x18001FDB0` passo 4). O renderer do ArtPlus pinta `fill` válido
  (`main_window.cpp:248-300`), então não há dependência nova.
- Todas as faixas são filhas de **um** `GroupObject` nomeado `Bearer Bar Mark N`, inserido no
  topo da camada de topo da página (`page->layers().back()`), espelhando
  `InsertNewObjectOnTop` (R7 passo 8; ordem provada, camada não — mesma nota de ADR-001 D3).
- Caixa de referência: `PageBoxType` → campo de `Page::boxes()`; caixa vazia (ou página
  inexistente) ⇒ `applyTo` devolve `0` e nada é criado (E5-like/E9), sem mensagem inventada.
- `locationMask == 0` ⇒ nenhum retângulo, nenhum `Path`, **sem erro** (E6, provado).
- `widthMm <= 0` ⇒ geometria vazia e `applyTo == 0`. É guarda declarada, não comportamento
  medido (A7); reversível numa linha.
- `locationMask == 0xF` com `extend > 0`: as quatro faixas **se sobrepõem** nos cantos; v1 não
  apara nem recorta (nenhuma evidência de trimming; `Location = 0xF` não foi observado, R4).
- O guarda anti-zero `28,346456692913385 mp` (0,01 mm, E7) **não é modelado**: ele existe
  porque o PDF+ calcula fator de escala a partir de `Extend` e do tamanho da referência
  (`0x180132A70` passo 6); v1 não calcula escala (A2/A8). Registrar como ausência declarada,
  não como equivalência.
- Tolerância de teste: `±0,03 pt` (≈ `0,01 mm`), o mesmo critério de ADR-001 D3 e da spec
  (R3/R6). Topologia (número de faixas e quais lados) é comparada exatamente.
- Não se calibra nenhum parâmetro a partir das medições em pixel da captura: as duas escalas
  derivadas (`2,762 px/mm` pela Media Box e `3,686 px/mm` pelo zoom) se contradizem
  (spec §6, §8 itens 9–10). O valor `5 mm` vem do ctor `0x18001E3C0`, não da imagem.

Predicado puro de coerência (R5, `0x180131DC0`), sem mutação:

```cpp
static bool sizeReferenceCoherent(int sizeReference, int direction); // regras 1–3 de R5
```

v1 não aplica a correção (`sizeReference = 0`); apenas expõe o predicado para teste e para
F-003b. Valores fora de `0..8` devolvem `false` (sem evidência; nenhum caminho v1 os produz).

### D4 — Persistência: `.artplus` aditivo agora; PDF+ e legado em F-003b

**Entra agora**, dentro do objeto já existente `metadata.DocumentMarks` (array novo por tipo):

```json
"metadata": {
  "DocumentMarks": {
    "version": 1,
    "cutMarks": [ /* inalterado, ADR-001 D4 */ ],
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

- A geometria já é persistida sem código novo (`GroupObject` + `PathObject`, incluindo
  `closed`, `document_serializer.cpp:166/223`).
- `format`/`version` do `.artplus` **não mudam**; `DocumentMarks.version` continua `1`:
  nenhum leitor (nem `CutMarkManager::loadFromMetadata`, `cut_mark.cpp:441-462`) ramifica por
  esse campo, e o array novo é opcional. Bump só quando houver mudança incompatível de layout.
- Carga: `groupId` inexistente ou objeto que não seja `ObjectType::Group` ⇒ registro
  descartado (mesma regra de ADR-001 D4); `kind` presente e diferente de
  `DynamicObject_PDFPlusBearerBar` ⇒ descartado; chave numérica ausente ⇒ default de D2;
  chave `bearerBars` ausente ⇒ zero bearer bars, documento carrega normalmente.
- **Regra de escritor único no objeto compartilhado (correção necessária em F-001):**
  `CutMarkManager::saveToMetadata` hoje faz `meta["DocumentMarks"] = toMetadata()`
  (`cut_mark.cpp:466-468`), o que **apaga** qualquer outra chave do objeto `DocumentMarks` —
  inclusive `bearerBars`. Os dois managers passam a **mesclar** a própria chave e a preservar
  as demais:

  ```cpp
  QJsonObject meta = m_document->metadata();
  QJsonObject dm   = meta.value("DocumentMarks").toObject();
  dm["bearerBars"] = array;                  // CutMarkManager: dm["cutMarks"]
  dm["version"]    = dm.value("version").toInt(1); // nunca reescreve/apaga outras chaves
  meta["DocumentMarks"] = dm;
  m_document->setMetadata(meta);
  ```

  Sem essa mudança, salvar uma Cut Mark depois de criar uma Bearer Bar perde a Bearer Bar —
  perda de dados silenciosa. É a única edição autorizada em `src/prepress/cut_mark.cpp`
  (D5) e exige o teste de regressão 7b de D7.

**Fica para F-003a — geometria e comportamento do PDF+** (spec §8 itens 1, 5–10):

| Item | Por que fica |
|---|---|
| Fórmula fechada do retângulo PDF+ (`0x180132A70` linhas `346..427`) | não derivada; v1 aproxima com faixas retangulares a partir de parâmetros |
| Pipeline `UpdateDirection` → `ValidateAnchors` → `UpdateSizeReference` (`0x180131C40`, `0x180131E30`, `0x180131DC0`) | exige modelo de anchors; v1 não tem |
| `GetPDFPlusBoundingBox` (`0x18012D9D0`) e `GetBoundingBoxForPositioning` (`0x18012D5F0`) | dependem do pipeline acima |
| Transformação por `Extend`/escala e guarda `0,01 mm` (E7) | v1 não calcula escala |
| Filete vermelho `255,79,79` e separação `Registration` (A5) | natureza do filete não decidida (spec §8 item 5) |

**Fica para F-003b — roundtrip PDF+/legado** (R8–R12):

| Item | Por que fica |
|---|---|
| Escrita `Position`/`Offset`/`Extends`/`Reference`/`ReferenceID` (`DynamicObject_PDFPlusBearerBar`) | serialização de saída nunca observada (`output/` vazio; spec §8 item 2) |
| Leitura de `Position` e `Extends` (`0x180179DA0`, `0x180170C20`), incluindo exceção E2 | pertence ao importador PDF+ (`src/pdf`), não ao gerador |
| `DM-BearerBarParams` (`0x18001F870`/`0x180020950`, R8/R9) e `AIDynamicMarks-BearerBar` (`0x180111410`) | caminho legado; `IsVertical` v1 (A9) |
| `ReferenceKind::Mark` / `ObjectID` (`+0x728 == 1`) e E8 | sem UI observada e sem modelo de referência por objeto |
| Setters transacionais `0x1800AA260/3C0/520` | em v1 a edição já é uma ação de undo via `setParams`; os setters reais entram quando houver UI/Inspector |
| `SizeReference`/`Direction` derivados de anchors | depende de F-003a |
| Revalidação contra v24.03.10017 (versão-alvo) (`regenerateMarkImp`, parser) | não executada (spec §8 item 13) |

**Fica para o dono da UI de painel/Inspector:** painel `Marks`, barra `Dynamic Marks`, dropdown
de ancoragem, Inspector com `Extend`/`Offset`/lados (spec §3, §8 itens 4 e 12; E10), e a ordem
painel `Bearer Bar Mark N` acima de `Cut Mark 1` (R1). O manager entrega a lista em ordem de
criação; inverter a exibição é decisão do painel, não do módulo.

**Fica para F-002 — Mark Sets (a especificar):** cópia de página de mark set (spec §2, último
parágrafo). Nenhuma parte entra aqui.

### D5 — UI v1: somente a ação de criação

- `QAction` `Add Bearer Bar Mark` no menu `Marks...` e no toolbar correspondente, sem modal;
  handler `addBearerBar()` no `ArtPlusMainWindow`, no mesmo formato de `addCutMark`
  (`main_window.cpp:1064-1078`): sem página corrente ⇒ retorna sem mensagem; falha de
  criação ⇒ retorna; sucesso ⇒ `setModified(true)`, `refresh`, `updateModifiedIndicator`,
  status `"Bearer Bar Mark N added"`.
- O rótulo `Add Bearer Bar Mark` vem de `CAP-20261119-02` (menu observado). O literal não
  existe em `DocumentMarks.dll` (spec §8 item 15) — é rótulo de UI do ArtPlus, não string do
  binário; não citar endereço para ele.
- **Fora de v1**: painel, barra de contexto, Inspector, dropdown de ancoragem, menu radial,
  edição numérica (E10 declara que não existe no caminho capturado).

### D6 — Escopo fechado de arquivos para `implementacao`

Novos:

- `src/prepress/bearer_bar.h`
- `src/prepress/bearer_bar.cpp`

Editados (lista fechada):

- `src/prepress/cut_mark.cpp` — **somente** `CutMarkManager::saveToMetadata` (466-470),
  convertendo a substituição do objeto `DocumentMarks` em mesclagem da chave `cutMarks`
  (D4). Nada mais em `cut_mark.h/.cpp`.
- `src/ui/main_window.h` / `src/ui/main_window.cpp` — declaração de `addBearerBar()`, membro
  `BearerBarManager m_bearerBarManager{m_document}`, e os cinco pontos já usados por
  `CutMarkManager`: `setUndoStack(&m_history)` (~610), `QAction` no menu (~688-689),
  `addTool(...)` (~741), `loadFromMetadata()` nos dois pontos de carga (768, 794) e
  `saveToMetadata()` no ponto de save (812).
- `CMakeLists.txt` — **somente** `src/prepress/bearer_bar.h` e `src/prepress/bearer_bar.cpp`
  na lista de fontes. O alvo `bearer_bar_test` + `add_test(NAME BearerBarTests ...)` entra em
  **segunda passada**, depois que `testes` criar `tests/unit/test_bearer_bar.cpp`
  (referenciar arquivo inexistente quebra o configure — proibido).

Proibido nesta fase: `src/pdf/**`, `src/color/**`, `src/document/**` (o `metadata()` genérico já
existe), `src/prepress/marks.*`, `ObjectType`, `PageBoxType`, `DocumentSerializer`, qualquer
spec/teste/evidência, `cut_mark.h`, e qualquer outro trecho de `cut_mark.cpp`.

### D7 — Contrato C++17/Qt

```cpp
#pragma once
#include "../document/document_model.h"
#include "../document/undo.h"
#include <QJsonObject>
#include <QRectF>
#include <QVector>
#include <optional>

namespace ArtPlus {

// D2 — parâmetros. Unidades de UI/persistência em mm no nome do campo;
// geometria em pontos de documento (mesma unidade de PageBoxes).
struct BearerBarParams {
    PageBoxType referenceBox = PageBoxType::TrimBox; // PDF+ PageBox=3 (A1)
    double widthMm       = 5.0;   // ctor legado 0x18001E3C0 (A2)
    double extendMm      = 0.0;   // PDF+ +0x6A0 / legado +0x6A0
    double offsetMm      = 0.0;   // PDF+ +0x698 / legado +0x6A8
    int    locationMask  = 0x3;   // 1=l 2=r 4=t 8=b (R4)
    int    sizeReference = 0;     // 0..8 (R5), opaco (A3)
    int    direction     = 1;     // 0=vertical 1=horizontal (A3)
    Color  color = Color::rgb(168.0f / 255.0f, 164.0f / 255.0f, 164.0f / 255.0f); // A5

    // R5 / 0x180131DC0 — regras 1–3; não muta sizeReference em v1.
    static bool sizeReferenceCoherent(int sizeReference, int direction);

    bool operator==(const BearerBarParams& o) const;  // qFuzzyCompare nos doubles
    bool operator!=(const BearerBarParams& o) const;
};

class BearerBarManager;

class BearerBarEditCommand : public Command { /* espelho de CutMarkEditCommand: D1 item 4 */ };

class BearerBar {
public:
    BearerBar();
    explicit BearerBar(const BearerBarParams& params);

    const BearerBarParams& params() const;
    void setParams(const BearerBarParams& params);   // só grava; não regenera

    ObjectId objectId() const;                       // grupo no documento; 0 = nunca aplicada
    QString  displayName() const;                    // "Bearer Bar Mark N"
    int      displayIndex() const;

    QRectF resolveReferenceBox(const Document& doc, PageId page) const;
    // Uma faixa por bit de locationMask, índice crescente de lado (D3).
    QVector<QRectF> buildGeometry(const QRectF& referenceBoxPt) const;

    ObjectId applyTo(Document& doc, PageId page, int displayIndex); // 0 = nada criado
    bool regenerate(Document& doc, PageId page);                    // substitui, não acumula

    QJsonObject toMetadata() const;
    static std::optional<BearerBar> fromMetadata(const QJsonObject& json, const Document& doc);
private:
    friend class BearerBarManager;
    BearerBarParams m_params;
    ObjectId m_groupId = 0;
    int m_displayIndex = 0;
};

class BearerBarManager {
public:
    explicit BearerBarManager(Document& document);
    void setUndoStack(UndoStack* stack);             // padrão StepRepeatEngine/CutMarkManager

    BearerBar* addBearerBar(PageId page);            // R1/R2: defaults de D2, sem modal
    BearerBar* find(ObjectId groupId);
    const QVector<BearerBar>& bearerBars() const;    // ordem de criação

    bool setParams(ObjectId groupId, const BearerBarParams& params); // UMA ação de undo
    bool loadFromMetadata();
    bool saveToMetadata() const;
    QJsonObject toMetadata() const;                  // só a fatia "bearerBars"
private:
    friend class BearerBarEditCommand;
    void onMarkRestored(ObjectId groupId, const BearerBarParams& params,
                        int displayIndex, bool present);
    int  nextDisplayIndex() const;
    PageId pageOf(const BearerBar& mark) const;
    void pruneDetached();

    Document* m_document = nullptr;
    UndoStack* m_undoStack = nullptr;
    QVector<BearerBar> m_bars;
};

} // namespace ArtPlus
```

Contrato de erro: sem página/camada, caixa vazia, `locationMask == 0` ou `widthMm <= 0` ⇒
`applyTo == 0` e `addBearerBar == nullptr`, nada criado, nenhuma mensagem inventada (E5/E6/E9;
E8 fora de v1). `setParams` com id desconhecido ou caixa vazia ⇒ `false` (estado anterior
preservado). `loadFromMetadata` sem a chave ⇒ `true` com zero marcas.

Undo: `BearerBarEditCommand` captura a subárvore (grupo + `PathObject`s) por ponteiros
`shared_ptr` (ids preservados, como em `cut_mark.cpp:485-525`) e restaura verbatim; o
`BearerBarManager` ressincroniza parâmetros em `onMarkRestored`. Criação e edição = **uma**
ação cada.

### D8 — Testes mínimos (para o agente `testes`)

`tests/unit/test_bearer_bar.cpp`, alvo `bearer_bar_test`, `kBearerBarTolPt = 0.03`:

1. **Defaults (R2)** — `addBearerBar` ⇒ `widthMm == 5.0`, `extendMm == 0`, `offsetMm == 0`,
   `locationMask == 3`, `sizeReference == 0`, `direction == 1`, `referenceBox == TrimBox`,
   nome `Bearer Bar Mark 1`.
2. **Topologia (R4/R6)** — página 210 × 297 mm (`595,276 × 841,89 pt`), `locationMask = 3`,
   `extend = offset = 0` ⇒ exatamente 2 faixas: `[−5, 0] × [0, H]` mm e `[W, W+5] × [0, H]` mm
   (±0,03 pt); nenhum ponto de preenchimento estritamente dentro da caixa de referência.
3. **Bitmask (R4)** — `0xC` ⇒ 2 faixas horizontais (topo/base); `0x5` ⇒ 2 faixas
   (esquerda/topo); `0x1` ⇒ 1 faixa (esquerda); `0x0` ⇒ 0 faixas e nenhum objeto criado, sem
   erro (E6). Comparar lado por lado, não só a contagem.
4. **Extend (R6)** — `extendMm = 2` ⇒ cada faixa cresce 2 mm em cada ponta no eixo do lado
   (±0,03 pt); dimensão perpendicular inalterada.
5. **Regeneração (R7)** — duas regenerações com os mesmos parâmetros ⇒ contagem de objetos da
   marca constante (não acumula) e conteúdo substituído; parâmetros alterados ⇒ geometria
   reflete o novo valor.
6. **Caixa de referência (A1)** — Trim ≠ Media ⇒ faixas usam Trim; trocar para `MediaBox`
   desloca as faixas para as bordas da Media Box.
7. **Persistência (D4)** — salvar `.artplus` e recarregar ⇒ mesmos parâmetros e mesmas faixas;
   chave `metadata.DocumentMarks.bearerBars` presente; documento sem a chave carrega com zero
   bearer bars.
   7b. **Regressão de escritor único** — com 1 Cut Mark + 1 Bearer Bar: salvar, recarregar e
   salvar de novo ⇒ ambas as listas preservadas (falha se `saveToMetadata` de um manager
   apagar a chave do outro).
8. **Coerência `SizeReference` (R5)** — `sizeReferenceCoherent`: `(1..4, 0) == true`,
   `(1..4, 1) == false`, `(5..8, 1) == true`, `(5..8, 0) == false`, `(0, 0) == true`,
   `(0, 1) == true`, `(9, 1) == false`.

## Mapeamento binário → ArtPlus

| Função (Ghidra) | Classe/método ArtPlus | Observação |
|---|---|---|
| `0x18012FAA0` (ctor `PDFPlusBearerBar`) | `BearerBarParams` defaults + `BearerBar::BearerBar()` | `Offset 0`, `Extend 0`, `Direction 1`, `PageBox 3`→`TrimBox` (A1), `SizeReference 0` |
| `0x18001E3C0` (ctor `BearerBar` legado) | `BearerBarParams::widthMm = 5.0`, `locationMask = 3` | espessura v1 vem daqui (A2) |
| `0x18016F250` (fábrica PDF+) | `BearerBarManager::addBearerBar` | construção + inserção, sem modal |
| `0x18001FDB0` / `FUN_18001EDF0` | `BearerBar::buildGeometry` (faixa por lado) | topologia provada; retângulo `FUN_18001EDF0` confiança média ⇒ `TODO(F-003)` (A4) |
| `0x180132A70` (`regenerateMarkImp` PDF+) | `BearerBar::regenerate` | v1: faixas retas; transformação/anchors ⇒ F-003a |
| `0x180131C40` (`UpdateDirection`) | `BearerBarParams::direction` (opaco) | derivação de anchors ⇒ F-003a (A3) |
| `0x180131DC0` (`UpdateSizeReference`) | `BearerBarParams::sizeReferenceCoherent` | predicado implementado; correção silenciosa ⇒ F-003b |
| `0x180131E30` (`ValidateAnchors`) | — | F-003a (A3) |
| `0x180131FE0` (`getMarkType`) | `BearerBar::toMetadata().kind = "DynamicObject_PDFPlusBearerBar"` | string de persistência; sem `ObjectType` novo |
| `0x18001F690` (`getMarkType` legado) | — | F-003b |
| `0x18001F870` (parser legado) | — | F-003b (`DM-BearerBarParams`, E1) |
| `0x180020950` (writer legado) | — | F-003b (R8) |
| `0x180179DA0` (parser `Position`/`Offset`) | — | F-003b (R10/R11, E2) |
| `0x180170C20` (`Extends`/`SizeReference`) | `sizeReferenceCoherent` (nomes) + tabela D2 | leitura do dicionário ⇒ F-003b |
| `0x1800AA260` / `0x1800AA3C0` / `0x1800AA520` (setters) | `BearerBarManager::setParams` | uma ação de undo; setters reais com UI ⇒ F-003b |
| `0x18001E350` (clone) / `0x1801326E0` (`postProcessClonedMark`) | cópia implícita de `BearerBarParams` | `Elementary` clone; `ReferenceID` ⇒ F-003b |
| `0x180111410` (criação legada `memcmp`) | — | F-003b |

## Alternativas descartadas

1. **Reutilizar `CutMark`/`CutMarkManager` com campos extras.** Descartada: parâmetros e
   topologia de geometria disjuntos (D1 itens 1–2), `applyTo` exige 8 segmentos
   (`cut_mark.cpp:198`) e o `operator==` de `CutMarkParams` passaria a comparar campos sem
   efeito, quebrando a detecção de "nada mudou" (`cut_mark.cpp:364`). Um `enum MarkKind`
   dentro de um manager único ainda deixaria os dois `buildGeometry` no mesmo tipo.
2. **Generalizar `CutMarkEditCommand` em um comando parametrizado agora.** Descartada nesta
   fase: exige alterar `cut_mark.h` (API aprovada por ADR-001, auditada em
   `docs/audit/F-001-audit.md` e coberta por `tests/unit/test_cut_mark.cpp`) sem destravar
   nada de F-003. Custo/risco maior que a duplicação declarada; gatilho de revisão em D1.
3. **`MarksEngine`/`MarkCutMarkSettings` como base da bearer bar.** Descartada: `MarksEngine`
   aplica presets estáticos em lote, sem identidade, sem regeneração por marca e sem undo
   (`marks.h:143-282`); `MarkCutMarkSettings` (offset `2.0`, `length 5.0`, `weight 0.25`,
   cor `red`) é de **outra** marca e seus defaults não têm relação com R2. Ver D1 e ADR-001
   alternativa 1.
4. **Persistir só a geometria (paths) sem parâmetros.** Descartada: R7/R8 exigem regeneração a
   partir de parâmetros; sem eles, mudar `Location`/`Offset` viraria edição manual de paths e
   quebraria undo único e o roundtrip F-003b (mesma razão de ADR-001 alternativa 2).
5. **Criar `ObjectType::DynamicMark` no núcleo.** Adiada com o mesmo argumento de ADR-001
   alternativa 3: atinge `TypeTag`, exportador, seleção e todos os `switch` sobre `ObjectType`,
   e v1 é `Group` + `Path` puros com parâmetros em metadado.
6. **Bump de `DocumentMarks.version` para `2`.** Descartada: nenhum leitor ramifica por
   versão e a adição é retrocompatível; bump sem necessidade criaria incompatibilidade
   artificial com `.artplus` já gravados (D4).
7. **Duplicar o objeto `DocumentMarks` sob uma chave irmã (`metadata.BearerBar`) para evitar
   tocar `cut_mark.cpp`.** Descartada: fragmenta o namespace do módulo previsto em
   `docs/architecture/DocumentMarks.md` §6.2 e a correção de mesclagem é de 5 linhas (D4).
8. **Calibrar geometria pela captura (14 px ≈ 5,25 mm).** Descartada: duas escalas
   contraditórias (2,762 vs 3,686 px/mm; spec §8 itens 9–10). O default `5 mm` vem do ctor.

## Consequências

Arquivos afetados: ver D6.

- **Módulo `DocumentMarks`**: passa a ter duas marcas dinâmicas com managers irmãos. A
  consolidação em `DynamicMarksManager` (prevista em `DocumentMarks.md` §8) fica **adiada**
  com gatilho explícito (D1). Enquanto isso, `BearerBarEditCommand` duplica ~80 linhas de
  `CutMarkEditCommand` — duplicação declarada, não acidental.
- **`src/prepress/cut_mark.cpp` entra na lista de edição** (uma função) por causa da colisão de
  escritor no objeto `DocumentMarks`. Sem isso há perda silenciosa de dados ao salvar.
  Contrato que atravessa módulos ⇒ **exige revisão do orquestrador** antes de `implementacao`
  (AGENTS.md §6.8). Risco: regressão em F-001 ⇒ teste 7b cobre.
- **Núcleo do documento**: nenhuma mudança. `metadata()`/`setMetadata()` já existem
  (`document_model.h:431-433`).
- **Fidelidade visual**: espessura vem do ctor legado, não do PDF+; a geometria PDF+ real
  (transformação de `FormPaint`, anchors, escala) não é modelada. `TODO(F-003)` em
  `widthMm`, `extend` e `direction`. O teste valida topologia e posição, **não** paridade
  pixel a pixel com `CAP-20261119-02`.
- **Cor**: cinza provisório; `Registration`/separação de tinta exige decisão de cor
  (`src/color` é proibido aqui). `TODO(F-003)`.
- **UI**: só a ação. R1 (linha no painel acima de `Cut Mark 1`) fica parcialmente atendida —
  bloqueio compartilhado com F-001 até existir painel `Marks`.
- **Numeração**: `Bearer Bar Mark N` com contador **independente** de `Cut Mark N`
  (`displayIndex = max+1` por tipo), persistido. Sequência com 2+ bearer bars não observada
  (A10).
- **Undo**: ids do grupo podem mudar após undo/redo (mesma limitação de ADR-001 D6);
  aceitável em v1, correção futura preservando ids no clone.
- **Sequenciamento**: primeira passada de `implementacao` entrega só código de produção
  (`bearer_bar.*` + edições de D6); `bearer_bar_test`/`BearerBarTests` entram na segunda
  passada, depois que `testes` criar o arquivo.

## Ambiguidades registradas (não decididas em silêncio)

| # | Ambiguidade (spec) | Efeito nesta ADR |
|---|---|---|
| A1 | `PageBox = 3` = `TrimBox` é inferência (§8 item 3); a captura ancorou na Media Box visualmente (§6) | v1 usa `TrimBox` (ctor `0x18012FAA0` grava `3`; UI mostra `Trim Box`). Se falso, muda só `referenceBox` default e o mapeamento em F-003b |
| A2 | PDF+ não tem espessura numérica: largura vem do `FormPaint` escalado (§8 item 1) | `widthMm = 5` é default **legado**; v1 usa como espessura da faixa; **proibido** exportar como chave PDF+ |
| A3 | `Direction` 0/1 e `ValidateAnchors` (§8 itens 6–7) | campos opacos; predicado de coerência existe, correção não é aplicada |
| A4 | Modelo do retângulo legado: `offset` soma à espessura ou desloca a faixa (§8 item 8) | v1: face interna a `offset`, espessura `width`; idêntico ao alternativo com `offset = 0` |
| A5 | Natureza do filete vermelho `255,79,79` (§8 item 5) e default `Registration` (§3) | não modelado; cor provisória cinza `TODO(F-003)` |
| A6 | Referência por objeto (`ObjectID`, E8) (§8 item 11 para E8) | fora de v1; `ReferenceKind::Mark` entra em F-003b |
| A7 | `width <= 0`, `extend < 0`, `offset < 0` (§8 item 8, "limites não testados") | guarda declarada só para `width <= 0`; negativos passam direto |
| A8 | Guarda `0,01 mm` (E7) pressupõe cálculo de escala do PDF+ | não modelada; ausência declarada, não equivalência |
| A9 | `IsVertical` legado (v1) (§8 item 14) | só leitura legada ⇒ F-003b |
| A10 | Ordem no painel (§8 item 12) e numeração com 2+ marcas | manager guarda ordem de criação; inversão é do painel; contador por tipo |
| A11 | Revalidação contra v24.03.10017 (versão-alvo) (§8 item 13) | não executada; nenhuma decisão depende dela |
| A12 | Origem do literal `Add Bearer Bar Mark` (§8 item 15) | rótulo de UI do ArtPlus; não se cita endereço |
| A13 | Escalas contraditórias da captura (§8 itens 9–10) | nenhum parâmetro calibrado por pixel |

Se qualquer ambiguidade for resolvida com evidência contrária a D2/D3, o delta fica restrito a
`bearer_bar.*` (parâmetros/geometria); contrato de manager e chave de persistência não mudam.

## Estado

aprovado (orquestrador, 2026-11-19)
