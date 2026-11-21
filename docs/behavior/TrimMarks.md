## Trim Marks

- **Captura:** `CAP-20261119-01`
- **Versão:** 25.0 (App v250 — `C:\Esko\bg_prog_artpro+_v250\ArtPro+.app\Contents\Windows\ArtPro+.exe`)
- **Nomenclatura:** na UI o recurso se chama **Cut Mark** (item `Add Cut Mark`, instância `Cut Mark 1`). No projeto/feature o mesmo recurso é rastreado como **Trim Marks** (`F-001`).
- **Caminho na UI:** painel `Marks` → botão `+` / dropdown de criação → item **`Add Cut Mark`**.
  - Itens irmãos no mesmo menu: `Add Repeat for Grid Mark`, `Add Bearer Bar Mark`, `Add Dimensions Mark`, `Add External File Mark`, `Add Position Mark`, `Add Input File Mark`.
- **Entrada:** `tools/reference_capture/F-001/input/test_vector.pdf`
  - Documento: `test_vector.pdf @ 97,52%` (após criação, título ganha `*`)
  - Media Box observada: **210,00 x 297,00 mm**
  - Document Profile: **ISO Coated v2 300% (ECI)**
  - Separações: **Cyan (1) / Magenta (2) / Yellow (3) / Black (4) / PANTONE 485 C (5)**
  - input intacto; SHA-256 `ADFD91B4E95BCE3708DB37055885C355880068EE53A1472AB1DC82E0820640D7`

### Criação (sem diálogo modal)
1. `Marks` → `+` → `Add Cut Mark`.
2. **Não abre diálogo modal.** A marca é criada direto no documento.
3. Resultado imediato:
   - o título da janela ganha `*` (`test_vector.pdf @ 97,52%* – ArtPro+`);
   - a barra de contexto no topo vira **`Dynamic Marks | Cut Mark 1`**;
   - a linha **`Cut Mark 1`** aparece no painel `Marks`.

### Campos do diálogo
Não há diálogo de criação. Os parâmetros ficam expostos depois, na barra de contexto do **Dynamic Marks** / painel de propriedades (o "Inspector"). Valores observados para `Cut Mark 1`:

| Campo | Tipo | Default observado | Unidade | Opções / faixa |
|---|---|---|---|---|
| `Attach to:` | dropdown + botão de alvo (crosshair) | **`Trim Box`** | — | `Media Box`, `Crop Box`, `Bleed Box`, `Trim Box`, `Art Box`, `Cut Mark 1` |
| offset (2º campo) | campo numérico com botão `fx` | **`0,00 mm`** | **mm** | não medida nesta sessão |

- O seletor `Trim Box` também aparece na barra de contexto quando a marca está ativa (`Dynamic Marks`).
- Ao fixar em `Trim Box`, uma **guia vermelha** é desenhada sobre a borda do Trim Box.

### Saída observada (visual)
- A marca desenha **linhas finas rosadas/vermelhas** cruzando cada canto da página, paralelas às bordas, projetadas **para fora** do Trim Box (leve deslocamento além do canto).
- A moldura da página/Trim Box aparece em **laranja**; a linha de corte (cut mark) é a linha rosada fina que cruza os cantos.
- Geometria aproximada confirmada nos recortes dos cantos: topo-esquerdo (`crop-corner-tl.png`) e inferior-direito (`crop-corner-br.png`).
- Objetos de arte do documento permanecem intactos; a marca é um item separado listado em `Marks`.

### Erros e limites (lacunas registradas)
- **NÃO reproduzido:** a mensagem `Cut Mark has invalid Orientation value`. Nenhum campo **Orientation** apareceu no inspetor/barra de contexto observada. Registrado como lacuna — não inferir comportamento.
- **Falha do Save As (não repetir):** ao tentar salvar/exportar, o diálogo `Save File` apresentou o erro literal **`O nome do arquivo não é válido.`**
  - `output/` ficou **vazio**; **nenhum PDF foi gerado**.
  - O PDF de saída permanece **lacuna**; o `input/test_vector.pdf` original não foi sobrescrito nem corrompido.
- **Ctrl+I:** nesta sessão `Ctrl+I` (`View > Open Next Inspector`) não reabriu nenhum painel de Inspector visível. Os dados de `Attach to` / offset acima vêm da barra de contexto do Dynamic Marks observada em `shot-09` / recorte `shot-16-inspector-attach.png`. Registrado como lacuna de captura nova.

### Capturas
`tools/reference_capture/F-001/shots/` (copiadas também para `rag/evidence/F-001/behavior/`):

- `shot-01-initial.png` — estado inicial do documento
- `shot-02-file-menu.png` — menu File
- `shot-03.png` — estado antes de abrir
- `shot-04-open-dialog.png` — diálogo de abertura
- `shot-05-opened.png` — PDF aberto
- `shot-06-marks-menu.png` — menu de criação de Marks (`Add Cut Mark`)
- `shot-07-after-add-cutmark.png` — após `Add Cut Mark` (sem modal; `Dynamic Marks | Cut Mark 1`)
- `shot-08-dblclick.png` — duplo clique na marca
- `shot-09-mark-tab.png` — `Cut Mark 1` ativo; barra com `Attach to = Trim Box`
- `shot-16-inspector-attach.png` — recorte do campo `Attach to: Trim Box`
- `crop-corner-tl.png` — recorte canto superior-esquerdo (linha rosada cruzando o canto)
- `crop-corner-br.png` — recorte canto inferior-direito (linhas rosadas cruzando o canto)
- `crop-marks-panel.png` — recorte da barra de ferramentas do painel Marks
- `shot-17a..17h` — sequência do Save As (falha `O nome do arquivo não é válido.`; `output/` vazio)
