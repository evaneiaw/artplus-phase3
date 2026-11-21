# ADR-001 — Cut Mark (Trim Marks, F-001)

## Contexto

A spec `docs/features/F-001.md` (status `spec`, confiança `media`) descreve a marca de corte
criada por `Marks > + > Add Cut Mark`: comando direto, sem diálogo, default `Attach to = Trim Box`
e offset `0,00 mm`, com linhas finas paralelas às bordas nos quatro cantos, prolongadas para fora
da caixa de referência, persistida como `DynamicObject_PDFPlusCutMark`.

Evidência estática que sustenta esta ADR:

| Evidência | O que fixa |
|---|---|
| `GHIDRA-DocumentMarks-0x18013A8B0` | ctor: `PageBox=3`, seletor de referência `0`, `Orientation=1`, `Repetition=2`, `LineTypes=[0]`, offset `0` |
| `GHIDRA-DocumentMarks-0x18013CB50` | `GetReferenceBox`: `PageBox` → `Page::GetPageBox`; ramo por objeto via `GetPDFPlusBoundingBox`; retângulo vazio em falha |
| `GHIDRA-DocumentMarks-0x18013D160` | offset como variant: `0xFF` vazio, `0` quantidade, `1` fórmula |
| `GHIDRA-DocumentMarks-0x18013EE90` | `regenerateMarkImp` (`[29]`): apaga conteúdo, resolve offset, resolve caixa, cor + bbox, `InsertNewObjectOnTop` em transação |
| `GHIDRA-DocumentMarks-0x18017C5D0` | parser PDF+: `Position`, `Reference`, `Orientation`, `Repetition`, `LineTypes`; mensagens de erro |
| `GHIDRA-DocumentMarks-0x1800ADB10` | `SetCutMarkOffset`: edição de offset via `ArtDoc::ActionList::DoAction` (undoable) |
| `GHIDRA-DocumentMarks-0x18013E3B0` | `getMarkType()` = `DynamicObject_PDFPlusCutMark` |
| `CAP-20261119-01` | criação sem modal; `Attach to: Trim Box`; offset `0,00 mm` + botão `fx`; saída visual nos cantos |
| `GHIDRA-Document-0x1800FAE70` | `GetPageBox` indexa o array de caixas por inteiro cru + tabela de fallback |

Código atual relevante (`src/`, leitura apenas nesta etapa):

- `src/prepress/marks.h/.cpp` — `MarksEngine` (aplicação em lote de presets) e `MarkCutMarkSettings`
  (`offset = 2.0`, `length = 5.0`, `weight = 0.25`, `color = red`, `Style {Single, Double, Corner}`).
  Todos os métodos e `MarksUtils` são stubs que devolvem vazio/`0`.
- `src/document/document_model.h` — `Document`, `Page`, `PageBoxes`, `PageBoxType`
  (`MediaBox, CropBox, BleedBox, TrimBox, ArtBox`), `Layer`, `GroupObject`, `PathObject`, `Color`,
  `FillStroke`.
- `src/document/undo.h` — `Command`/`UndoStack`; precedente de engine que recebe pilha:
  `StepRepeatEngine::setUndoStack` e `ActionListEngine::setUndoStack`.
- `src/document/document_serializer.*` — `.artplus` JSON; `ObjectType` fechado
  (`Path, Text, Image, Group, ClippingMask, OpacityMask`), sem metadados por objeto.
- `src/pdf/pdf_importer.cpp:347-351` — as cinco caixas são copiadas iguais quando o PDF não as
  define; coordenadas de documento estão em pontos PDF (72 dpi).
- `src/ui/main_window.*` — menu `Marks...` e toolbar abrem um diálogo genérico
  (`main_window.cpp:683, 736, 999`); não existe painel `Marks` nem barra `Dynamic Marks`.

### Lacunas da spec que esta ADR não resolve

Registradas aqui em vez de decididas em silêncio (spec §8): geometria exata
(comprimento/espessura/nº de segmentos, item 1), semântica de `Orientation`/`Repetition`
(itens 2 e 3), `PageBox=3 = TrimBox` (item 4), sentido/faixa do offset (item 5), fallback de
fórmula inválida (item 6), serialização de saída PDF+ (item 7), caminho legado (item 8), cor da
marca (item 9), numeração `Cut Mark N` (item 10), tabela de fallback de caixa (item 11),
rótulos do Inspector (item 13). Toda decisão abaixo que depende dessas lacunas está marcada
`TODO(F-001)` e é reversível por mudança local.

## Decisão

### D1 — Onde a funcionalidade vive: `CutMark` própria, não `MarksEngine`

Nova unidade `src/prepress/cut_mark.h/.cpp` com duas classes:

- `CutMark` — a marca (parâmetros + geometria + inserção/regeneração na página);
- `CutMarkManager` — dono da lista de marcas de um `Document`, criação a partir do comando,
  edição transacional e persistência em `.artplus`.

`MarksEngine` **não** é estendido para hospedar a marca. Motivos:

1. `MarksEngine` gera geometria estática a partir de presets (`MarkSettings` + `applyMarks`);
   não possui identidade por marca, referência por objeto, regeneração nem persistência por
   objeto. A `PDFPlusCutMark` do original é um objeto com estado (`PageBox`, `Orientation`,
   `Repetition`, `LineTypes`, offset) que sobrevive a save/load — modelo de entidade, não de
   preset (`GHIDRA-DocumentMarks-0x18013A8B0`, `0x18013EE90`).
2. R8 exige regeneração com substituição atômica do conteúdo da marca, e R5 exige que cada
   edição de offset seja **uma** ação de undo (`0x1800ADB10`). `MarksEngine` não conhece
   `UndoStack` nem substituição de subárvore; transformá-lo nisso duplicaria responsabilidades
   de `Document`/`Layer` e o papel de "aplicador de presets".
3. `MarkCutMarkSettings` não tem `attachTo`, referência por objeto nem fórmula, e seus defaults
   (`offset = 2.0 mm`, `length = 5.0`, `weight = 0.25`, `color = red`) **contradizem** o default
   observado (`0,00 mm`, `CAP-20261119-01`). Adotá-lo gravaria um default falso e ainda exigiria
   os campos novos no mesmo struct — os dois modelos não convergem.

O que é reutilizado do existente: `MarksUtils::mmToPoints` / `pointsToMm` (conversão de unidade),
`Document`/`Page`/`PageBoxes`/`PageBoxType`, `GroupObject` + `PathObject`, `Color`/`FillStroke`,
`Command`/`UndoStack`, `DocumentSerializer`, e o padrão `setUndoStack(UndoStack*)` de
`StepRepeatEngine` (`src/prepress/step_repeat.h:131`).

O enum `MarksEngine::MarkType` não é usado para a marca: além de não haver campo de tipo por
objeto no documento, o enum tem colisões (`ColorBar = 2` e `PageNumber = 2`) e dois nomes para a
mesma família (`CutMark = 3`, `TrimMark = 7`). Limpeza do enum não faz parte de F-001.

### D2 — Modelo de parâmetros

```cpp
struct CutMarkParams {
    enum class ReferenceKind { PageBox, Mark };

    ReferenceKind referenceKind = ReferenceKind::PageBox;
    PageBoxType   referenceBox  = PageBoxType::TrimBox; // PDF+ PageBox=3 (inferência, spec R2)
    ObjectId      referenceMarkId = 0;                  // ReferenceKind::Mark (spec R4)

    double  offsetMm = 0.0;        // unidade de UI; convertido para pt na geometria
    QString offsetFormula;         // fx; v1 não avalia (TODO(F-001), spec R6/E4)

    int orientation = 1;           // opaco; ctor 0x18013A8B0. TODO(F-001)
    int repetition  = 2;           // opaco; ctor 0x18013A8B0. TODO(F-001)
    QVector<int> lineTypes{0};     // ctor: vetor com 1 elemento, valor 0

    // Parâmetros de forma v1 — inferência declarada, não medidos (spec §8 item 1)
    double lengthMm      = 5.0;    // TODO(F-001)
    double strokeWidthPt = 0.25;   // TODO(F-001)
    Color  color = Color::cmyk(0.0f, 1.0f, 0.0f, 0.0f); // provisório; spec §8 item 9
};
```

Regras do modelo:

- **Unidades explícitas**: campos de UI em `mm` no nome (`offsetMm`, `lengthMm`); espessura em
  `pt` (`strokeWidthPt`); geometria em pontos de documento (mesma unidade de `PageBoxes`, ver
  `pdf_importer.cpp:347-351`). Conversão só em `CutMark::buildGeometry`.
- `orientation`, `repetition` e `lineTypes` são **carregados e persistidos, mas não alteram a
  geometria v1**, porque o significado de `Repetition=2` e `Orientation=1` não foi medido
  (spec §8 itens 2 e 3). Nenhum campo de UI é criado para eles: a captura não os mostrou
  (spec §3, E1/E2).
- `offsetFormula` existe no modelo para não perder o dado quando a UI `fx` chegar, mas fica
  vazio em v1. Sem avaliador de fórmula em `src/prepress` e sem semântica de fórmula inválida
  (E4: valor final após `RemoveInvalidCustomFormulasImp` é lacuna), avaliar seria inventar.

### D3 — Geometria v1

`CutMark::buildGeometry(const QRectF& referenceBoxPt)` devolve segmentos (pares de pontos):

```
off = mmToPoints(offsetMm); len = mmToPoints(lengthMm)
para cada vértice (cx, cy) e sinal externo (sx, sy) ∈ {(-1,-1),(1,-1),(-1,1),(1,1)}:
    segmento horizontal: (cx + sx*off, cy) → (cx + sx*(off+len), cy)
    segmento vertical:   (cx, cy + sy*off) → (cx, cy + sy*(off+len))
```

- 4 vértices × 2 segmentos = **8 segmentos por marca** (inferência a partir de
  `CAP-20261119-01`: "linhas finas ... paralelas às bordas, projetadas para fora";
  spec R7 e §8 item 1 marcam o número de segmentos como não medido ⇒ `TODO(F-001)`).
- Offset positivo = para fora (inferência, spec R5); com `offsetMm = 0` o segmento continua a
  própria aresta a partir do vértice, reproduzindo o "cruzando cada canto" observado.
- Cada segmento vira um `PathObject` (Move + Line), sem preenchimento, `stroke = params.color`,
  `strokeWidth = strokeWidthPt`, cap `Flat`, join `Miter`; todos filhos de um `GroupObject`
  nomeado `Cut Mark N` (R9), inserido no topo da camada de topo da página
  (`page->layers().back()`; `InsertNewObjectOnTop` evidencia ordem, não camada).
- Caixa de referência (`CutMark::resolveReferenceBox`, mapa de `0x18013CB50`):
  `PageBox` → campo correspondente de `Page::boxes()`. `Mark` → **união dos 8 segmentos
  gerados da marca referenciada** (`CutMark::markBoundsImpl`, `src/prepress/cut_mark.cpp`),
  **não** `DocumentObject::boundingBox()`: `GroupObject::boundingBox()` é stub no núcleo
  (`document_model.cpp:140-142`) e devolveria retângulo vazio.
  - Limitação do núcleo, não escolha de domínio: enquanto `GroupObject::boundingBox()` não for
    implementado (ou o ramo 3 de `0x18013CB50` não for medido), a caixa por segmentos é a única
    leitura possível.
  - Consequência de offset: a caixa derivada cobre `offsetMm + lengthMm` além do vértice em
    cada lado (os segmentos vão de `off` a `off+len` para fora), então ela **não** é o
    `GetPDFPlusBoundingBox` do original — não há medição que sustente equivalência. Marcar como
    divergência de contrato conhecida, não como paridade.
  - Caixa ou objeto inexistente ⇒ `QRectF()` ⇒ `applyTo` devolve `0` e não cria nada (E5/E6).
- `ReferenceKind::Mark` aceita **apenas outra Cut Mark** em v1 (o dropdown lista as caixas e a
  própria marca, spec R4); apontar para objeto arbitrário não é exposto pela UI v1.
- Cor provisória magenta CMYK (não cria separação nova, coerente com spec §6), marcada
  `TODO(F-001)` até a leitura de `ApplyPDFPlusColor` / item 9.

Tolerância declarada: comparação de coordenadas em pontos com `±0,03 pt` (≈ `0,01 mm`), o mesmo
critério da spec (R3/R7). Contagem de segmentos é comparada exatamente.

### D4 — Persistência: `.artplus` v1 agora, PDF+ em F-001a

**Entra no `.artplus` agora** (root, chave nova e opcional):

```json
"metadata": {
  "DocumentMarks": {
    "version": 1,
    "cutMarks": [
      { "groupId": 41, "name": "Cut Mark 1", "displayIndex": 1, "kind": "DynamicObject_PDFPlusCutMark",
        "reference": { "kind": "PageBox", "box": "TrimBox" },
        "offsetMm": 0.0, "offsetFormula": "", "orientation": 1, "repetition": 2,
        "lineTypes": [0], "lengthMm": 5.0, "strokeWidthPt": 0.25 }
    ]
  }
}
```

- A geometria já é persistida sem código novo, porque é `GroupObject` + `PathObject`
  (`document_serializer.cpp`, tags `Group`/`Path`).
- Os parâmetros entram por meio de um par de acessores **genéricos** em `Document`
  (`QJsonObject metadata() const; void setMetadata(const QJsonObject&)`), serializados como
  `root["metadata"]`. O núcleo do documento continua sem conhecer prepress; a chave
  `DocumentMarks` é escrita/lida por `CutMarkManager`.
- Na carga, `CutMarkManager::loadFromMetadata()` valida `groupId` contra `Document::objects()`;
  registro órfão é descartado (não há marca sem grupo). `referenceMarkId` ausente ⇒ 0.
- Compatibilidade: `format`/`version` do `.artplus` **não mudam**; leitores antigos ignoram a
  chave. Documentos sem metadados carregam com zero marcas.

**Fica para F-001a (roundtrip PDF+):**

| Item | Por que fica |
|---|---|
| Escrita das chaves `DynamicObject_PDFPlusCutMark`, `Position`, `Reference`, `Orientation`, `Repetition`, `LineTypes`, `PDFPlusCutMark_Offset`, `DocumentMarks_CutMark_ReferenceID`, `PDFPlusCutMark_OriginalReferenceName` | a serialização de saída nunca foi observada (`Save As` falhou; spec §8 item 7) |
| Parser `0x18017C5D0` e validação E1–E3 (`Cut Mark has invalid Orientation/Repetition value`) | pertence ao importador PDF+ e à spec R11 |
| Mapeamento nome↔inteiro de `Reference`/`PageBox` (incl. `AllStructuralDesignStations`) | o índice `3 = TrimBox` é inferência (spec §8 item 4) |
| `FUN_180172550` (leitor que constrói `PDFPlusCutMark`) | acoplado ao importador (`src/pdf`), não ao gerador |

**Fica para F-001b:** caminho legado `AIDynamicMarks-CutMark` (`0x180111410`,
`0x180052340`, `0x180052CA0`, dicionário `DM-CutMarkParams`), que posiciona símbolos clonados e
tem semântica própria (spec R12, §8 item 8).

**Fica para F-001c:** fórmula `fx` (`0x18013D160` seletor `1`, `0x18013ED6E`), incluindo nome
`PDFPlusCutMark_Offset` e fallback de fórmula inválida (`0x18013CEF0`).

**Fica para F-002 — Mark Sets (a especificar):** aplicação por mark set
(`TaskMarkSets::ApplyMarksFromSet` `0x18024D3E0`, `ApplyRelevantMarkSets` `0x18024D460`;
spec R13), incluindo `CopyMarksToPageWithLog` e `MakeFormsFromMarks`. A marca gerada por F-001 é
copiável, mas a cópia/primeira página como mecanismo de mark set é funcionalidade própria e tem
ID de continuação (`F-002`); nenhuma parte de R13 entra em F-001.

**Fica para o dono da UI (spec de painel):** painel `Marks`, barra `Dynamic Marks`, dropdown
`Attach to:` com crosshair, guia vermelha do Trim Box e rótulos (R1 UI, R3 guia, §3). O `src/ui`
atual só tem diálogo genérico `Marks...`; um painel é funcionalidade de UI própria e não cabe
nesta ADR. v1 entrega o **comando** e o **modelo**; a entrada visual no painel depende do painel.

### D5 — Escopo de arquivos para `implementacao`

Novos:

- `src/prepress/cut_mark.h`
- `src/prepress/cut_mark.cpp`

Fora do escopo desta primeira passada de `implementacao` (dono: `testes`):
`tests/unit/test_cut_mark.cpp` e o alvo `cut_mark_test` / `add_test(NAME CutMarkTests ...)`.
`implementacao` **não** pode referenciar `tests/unit/test_cut_mark.cpp` no `CMakeLists.txt`
antes de o arquivo existir; o alvo de teste entra numa **segunda passada** de `implementacao`,
depois que `testes` criar o arquivo com os casos de D7.

Editados (lista fechada):

- `src/document/document_model.h` / `src/document/document_model.cpp` — `metadata()` +
  `setMetadata()`; nada mais.
- `src/document/document_serializer.cpp` — ler/gravar `root["metadata"]` (omitir se vazio).
- `src/ui/main_window.h` / `src/ui/main_window.cpp` — ação `Add Cut Mark` no menu/toolbar
  `Marks` (`setupMenusAndToolbars`, região de `main_window.cpp:683,736`), handler sem modal,
  chamando `CutMarkManager::addCutMark(Document::currentPage()->id())` e empilhando undo.
- `CMakeLists.txt` — **somente** `src/prepress/cut_mark.*` na lista de fontes. O alvo
  `cut_mark_test` + `add_test(NAME CutMarkTests ...)` fica para a segunda passada (ver acima).
- `src/prepress/marks.h` e `src/prepress/marks.cpp` — **somente comentário** em
  `MarkCutMarkSettings` e `MarksEngine::addCutMarks` indicando `TODO(F-001)` /
  "superseded by CutMark (ADR-001)". Nenhuma mudança de assinatura.

Proibido nesta fase: alterar `ObjectType`, `PageBoxType`, `DocumentSerializer` além de `metadata`,
`MarksEngine` além do comentário, `src/pdf/**` (F-001a), qualquer spec/teste/evidência.

### D6 — Contrato C++17/Qt

```cpp
namespace ArtPlus {

struct CutMarkParams { /* D2 */ };

class CutMark {
public:
    CutMark();
    explicit CutMark(const CutMarkParams& params);

    const CutMarkParams& params() const;
    void setParams(const CutMarkParams& params); // só grava; não regenera

    ObjectId objectId() const;          // grupo no documento; 0 = nunca aplicada
    QString  displayName() const;       // "Cut Mark N"
    int      displayIndex() const;

    QRectF resolveReferenceBox(const Document& doc, PageId page) const;
    QVector<QLineF> buildGeometry(const QRectF& referenceBoxPt) const;

    // Cria a subárvore; devolve o id do grupo, 0 em caixa vazia/página inválida.
    ObjectId applyTo(Document& doc, PageId page, int displayIndex);
    // Apaga a subárvore atual e recria com os parâmetros correntes.
    bool regenerate(Document& doc, PageId page);

    QJsonObject toMetadata() const;
    static std::optional<CutMark> fromMetadata(const QJsonObject& json, const Document& doc);
private:
    CutMarkParams m_params;
    ObjectId m_groupId = 0;
    int m_displayIndex = 0;
};

class CutMarkManager {
public:
    explicit CutMarkManager(Document& document);
    void setUndoStack(UndoStack* stack);        // padrão StepRepeatEngine

    CutMark* addCutMark(PageId page);           // R1: defaults TrimBox / 0,00 mm
    CutMark* find(ObjectId groupId);
    const QVector<CutMark>& cutMarks() const;

    // Regenera e empilha UMA ação de undo.
    bool setParams(ObjectId groupId, const CutMarkParams& params);

    bool loadFromMetadata();
    bool saveToMetadata() const;
    QJsonObject toMetadata() const;
private:
    Document* m_document = nullptr;
    UndoStack* m_undoStack = nullptr;
    QVector<CutMark> m_marks;
};

} // namespace ArtPlus
```

Contrato de erro: sem página corrente, sem `Page`, ou caixa de referência vazia ⇒ `applyTo`
devolve `0`, `addCutMark` devolve `nullptr`, nada é criado e nenhuma mensagem é inventada
(E6/E9 não observados). `setParams` com id desconhecido ⇒ `false`.

Undo: `CutMark::regenerate` captura a subárvore atual por `clone()` (já virtual em
`DocumentObject`), substitui e empilha um único `CutMarkEditCommand` (definido em
`src/prepress/cut_mark.h`, derivado de `Command`) que restaura o snapshot anterior. `Document`
e `undo.h` não ganham comandos novos. Limitação declarada: `ObjectId` do grupo pode mudar após
undo/redo; `CutMarkManager` reindexa pelo novo id ao aplicar o comando.

### D7 — Testes mínimos (para o agente `testes`)

`tests/unit/test_cut_mark.cpp`, alvo `cut_mark_test`, tolerância `kCutMarkTolPt = 0.03`:

1. **Defaults (R2/R9)** — `addCutMark` ⇒ `referenceBox == TrimBox`, `offsetMm == 0.0`,
   `orientation == 1`, `repetition == 2`, `lineTypes == {0}`, nome `Cut Mark 1`.
2. **Topologia (R7)** — página 210 × 297 mm, Trim = Media ⇒ 8 segmentos, 2 por vértice, nos
   quatro cantos; nenhum ponto estritamente dentro da caixa.
3. **Offset (R5)** — `offsetMm = 3.0` ⇒ cada endpoint desloca 3,00 mm para fora no eixo do
   segmento (±0,03 pt); undo único volta a 0 e à geometria original.
4. **Caixa de referência (R3)** — Trim ≠ Media ⇒ geometria usa Trim; trocar para `MediaBox`
   desloca os vértices para a Media Box.
5. **Regeneração (R8)** — duas regenerações seguidas ⇒ número de objetos da marca constante
   (não acumula), conteúdo substituído.
6. **Referência por objeto (R4/E5)** — `referenceKind = Mark` com id existente usa a caixa
   derivada dos 8 segmentos da marca referenciada (`CutMark::markBoundsImpl`); id inexistente
   ⇒ `resolveReferenceBox` vazio e `applyTo == 0`.
7. **Persistência (R10 v1)** — salvar `.artplus`, recarregar ⇒ mesmo `offsetMm`, `referenceBox`,
   `orientation`, `repetition`, `lineTypes` e mesmos 8 segmentos; chave `metadata.DocumentMarks`
   presente; documento sem a chave carrega com 0 marcas.

## Mapeamento binário → ArtPlus

| Função (Ghidra/endereço) | Classe/método ArtPlus | Observação |
|---|---|---|
| `0x18013A8B0` (ctor `PDFPlusCutMark`) | `CutMarkParams` defaults + `CutMark::CutMark()` | `PageBox=3`→`TrimBox` inferido; offset 0 |
| `0x18013CB50` (`GetReferenceBox`) | `CutMark::resolveReferenceBox` | ramo objeto usa `CutMark::markBoundsImpl` (união dos 8 segmentos) |
| `0x18013D160` (`SetOffset`) | `CutMarkParams::offsetMm` + `setParams` | seletor fórmula adiado (F-001c) |
| `0x18013BDC0` (resolução do offset) | `CutMark::buildGeometry` (offset numérico) | fórmula `TODO(F-001)` |
| `0x18013EE90` (`regenerateMarkImp`) | `CutMark::regenerate` + `buildGeometry` | apaga, resolve, insere no topo, 1 transação |
| `0x18013ED6E` (`getPositionRefBBox`) | `CutMark::resolveReferenceBox` | v1 sem `Position`/transform extra |
| `0x1800ADB10` (`SetCutMarkOffset`) | `CutMarkManager::setParams` | undo único |
| `0x1800ADCA0`, `0x1800AE100`, `0x1800AD540` (orientation/repetition/lineTypes) | `CutMarkParams` (opaco) | sem efeito geométrico v1; UI inexistente |
| `0x18013E3B0` (`getMarkType`) | `CutMark::toMetadata().kind` = `"DynamicObject_PDFPlusCutMark"` | string de persistência; sem `ObjectType` novo |
| `0x18017C5D0`, `0x180172550` (parser/import PDF+) | `CutMarkParams::fromPdfPlusDict` (não implementado) | F-001a |
| `0x180111410`, `0x180052340`, `0x180052CA0` (legado) | — | F-001b |
| `0x18024D3E0` (`ApplyMarksFromSet`) | — (não implementado) | **F-002 — Mark Sets (a especificar)**; spec R13 |
| `0x18013CEF0` (fórmula inválida) | — | F-001c |

## Alternativas descartadas

1. **Reutilizar `MarkCutMarkSettings`/`MarksEngine::addCutMarks` sem abstração nova.**
   Descartada. `MarkCutMarkSettings` não modela `attachTo`, referência por objeto nem fórmula;
   seus defaults (`offset 2.0 mm`, `length 5.0`, `weight 0.25`, `color red`) não têm evidência e
   o offset 2.0 mm contradiz `0,00 mm` observado; e um `Settings` não tem identidade no
   documento, então não há como representar `Attach to: Cut Mark 1` (R4), regeneração por marca
   (R8) nem persistência por marca (R10). Só a conversão de unidade e o estilo de geometria são
   aproveitados.
2. **Marca como grupo de paths sem parâmetros (só geometria).** Descartada: R8 exige regenerar
   a partir de parâmetros; sem eles, mudar `Attach to:`/offset seria edição manual de paths e
   quebraria o undo único e a exportação PDF+ (F-001a precisa dos parâmetros).
3. **Novo `ObjectType::DynamicMark` no núcleo do documento.** Adiada, não rejeitada em
   definitivo. Exigiria tocar `TypeTag`, `PdfExporter`, seleção, bounding boxes e todo `switch`
   sobre `ObjectType`, com ADR própria de document core; a geometria v1 é `Path` puro, e os
   parâmetros cabem em metadados genéricos. Revisitar quando F-001a importar objetos dinâmicos
   reais.
4. **Guardar parâmetros em `name`/`transform`/`fillStroke` do grupo.** Descartada: sem esquema,
   quebra ao renomear, e serializa lixo em campos visíveis.
5. **Avaliar a fórmula `fx` v1 com `ScriptEngine`.** Descartada para v1: inverte a direção de
   dependência (`src/automation` já conhece prepress via `registerMarksEngine`) e a semântica de
   fórmula inválida (E4) e do nome `PDFPlusCutMark_Offset` não foi lida. Vai para F-001c.
6. **Persistir as chaves PDF+ já em v1.** Descartada por falta de evidência de escrita
   (spec §8 item 7): geraria formato inventado e um roundtrip falso-verde.

## Consequências

Arquivos afetados: ver D5. Impacto:

- **Núcleo do documento**: ganha apenas `metadata()`/`setMetadata()` e a chave `root["metadata"]`.
  É contrato que atravessa módulos — requer revisão do orquestrador antes de `implementacao`.
- **`MarksEngine`**: permanece intacto (só comentário). Não há migração de dados: nenhum
  documento existente tem marcas dinâmicas.
- **Sequenciamento de fases**: a primeira passada de `implementacao` entrega apenas código de
  produção (`cut_mark.*` + as edições de D5) e as fontes no `CMakeLists.txt`; o alvo
  `cut_mark_test` / `add_test(NAME CutMarkTests ...)` entra numa **segunda passada**, depois que
  `testes` criar `tests/unit/test_cut_mark.cpp`. Referenciar arquivo de teste inexistente no
  CMake quebraria o configure — proibido.
- **Risco de fidelidade visual**: comprimento, espessura, cor e número de segmentos são
  inferência (`TODO(F-001)`). O teste valida topologia/posição, **não** paridade pixel a pixel
  com `crop-corner-tl.png`/`crop-corner-br.png`. Se a leitura de `0x18013EE90` contradizer D3,
  muda `buildGeometry` e as constantes de `CutMarkParams`, sem tocar em persistência.
- **Caixa de referência por marca (revisão de auditoria, 2026-11-19)**: `ReferenceKind::Mark`
  mede a marca referenciada pela **união dos seus 8 segmentos**
  (`CutMark::markBoundsImpl`), porque `GroupObject::boundingBox()` é stub no núcleo
  (`document_model.cpp:140-142`). Consequência: com `offsetMm ≠ 0` a caixa cresce
  `offset + length` por lado, e **não existe paridade medida** com `GetPDFPlusBoundingBox`
  (`0x18013CB50` ramo 3) — é divergência de contrato declarada, não equivalência. Correção
  futura: trocar a fonte da caixa dentro de `resolveReferenceBox` quando o core implementar
  `boundingBox()` ou o ramo 3 for medido, sem mudar API nem persistência.
- **Risco de undo**: identidade do grupo pode mudar após undo/redo (D6). Aceitável para v1,
  documentado; se o painel exigir id estável, a correção é preservar ids no `clone()` do grupo.
- **Lacuna de UI**: sem painel `Marks`, R1 só é atendida parcialmente (comando existe, sem
  entrada visual). O aceite de F-001 completo fica bloqueado até a spec de painel.
- **Deviation conhecida**: `PageBoxes` sempre tem as cinco caixas (importador iguala caixas
  ausentes, `pdf_importer.cpp:347-351`), então E7 (tabela `DAT_1802EF6D8`) não é modelada; o
  efeito observável é o mesmo (usa a caixa substituta), mas a tabela exata segue desconhecida.
- **`Attach to: Cut Mark N`**: aceito apenas entre Cut Marks em v1; referência por objeto
  arbitrário fica fora do dropdown (spec R4 lista as caixas e a própria marca).

## Ambiguidades registradas (não decididas)

| # | Ambiguidade (spec) | Efeito nesta ADR |
|---|---|---|
| A1 | `PageBox = 3` = `TrimBox` é inferência (§8 item 4) | modelo usa `PageBoxType::TrimBox`; se falso, muda o default e o mapeamento em F-001a |
| A2 | `Orientation = 1` / `Repetition = 2` sem semântica (§8 itens 2,3) | campos opacos; sem efeito geométrico; UI não expõe |
| A3 | Sentido/faixa do offset não medidos (§8 item 5) | positivo = para fora (inferência R5); teste só cobre offset ≥ 0 |
| A4 | Fórmula e fórmula inválida (§8 item 6, E4) | campo existe, não avaliado; F-001c |
| A5 | Cor da marca (§8 item 9) | magenta CMYK provisório; `TODO(F-001)` |
| A6 | Geometria exata (§8 item 1) | 8 segmentos e defaults `5.0 mm`/`0.25 pt` marcados `TODO(F-001)` |
| A7 | Numeração `Cut Mark N` (§8 item 10) | `displayIndex = max+1` por documento; renomear/excluir não tratados |
| A8 | Mensagens E1–E3 | só existem no parser PDF+ ⇒ F-001a |
| A9 | E9 (sem página corrente) | sem mensagem; `applyTo` devolve 0 |

Se qualquer ambiguidade acima for resolvida com evidência contrária a D2/D3, esta ADR volta a
`proposto` e o delta fica restrito a `cut_mark.*` (parâmetros/geometria) — persistência e
contrato de manager não mudam.

## Estado

aprovado (orquestrador, 2026-11-19)

Revisão 2026-11-19 (motivo: `docs/audit/F-001-audit.md`): D3 corrigida para descrever a caixa
de referência por marca como união dos 8 segmentos (`CutMark::markBoundsImpl`) e a limitação
`GroupObject::boundingBox()` do núcleo; R13 passou a ter dono explícito em
**F-002 — Mark Sets (a especificar)**. As decisões D1–D7 e o contrato de código permanecem
inalterados; estado segue `aprovado`.
