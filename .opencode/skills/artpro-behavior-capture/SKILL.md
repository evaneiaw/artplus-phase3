---
name: ArtPro+ Behavior Capture
description: Opera o ArtPro+ real (v250/v240) com o MCP pc-control e registra comportamento observável — passos, diálogos, campos, saída, erros e capturas CAP-AAAAMMDD-nn. Use ao capturar uma ferramenta na GUI, reproduzir um fluxo do original ou provar o que o clone precisa fazer.
metadata:
  version: "1"
---

# Captura de comportamento do ArtPro+

Você é a fonte de verdade sobre o que o ArtPro+ faz de fato. Sem captura, a spec é chute.

## Ambiente

```
App v250: C:\Esko\bg_prog_artpro+_v250\ArtPro+.app\Contents\Windows\ArtPro+.exe
App v240: C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows\ArtPro+.exe
Provas:   H:\Desktop\ArtPlus\tools\test_documents
Capturas: H:\Desktop\ArtPlus\tools\reference_capture
```

## Ferramentas do MCP `pc-control`

Use nomes exatos — não invente ferramenta nem coordenada.

| Ferramenta | Para que serve |
|---|---|
| `list_open_windows()` | ver janelas, títulos e processos antes de agir |
| `open_application(...)` | abrir o ArtPro+ |
| `get_screen_info()` | monitores, cursor e escala DPI |
| `screenshot(...)` | capturar a tela inteira ou região |
| `get_elements(window_title, ...)` | listar controles com bounding box |
| `click_element(window_title, element_name, ...)` | clicar por nome, sem adivinhar coordenada |
| `mouse_click(x, y, button, clicks, hold_keys)` | clique por coordenada quando não há elemento |
| `mouse_drag(start_x, start_y, end_x, end_y, ...)` | arrastar/desenhar |
| `mouse_scroll(...)` | zoom e rolagem |
| `keyboard_type(text)` | digitar em campo focado |
| `keyboard_hotkey(keys)` | atalhos (ex.: `["ctrl","s"]`) |
| `wait(seconds)` | aguardar render/diálogo |

Regra: **`get_elements` + `click_element` antes de coordenada**. Coordenada só quando o controle não aparece na árvore.

## Preparação

1. Copie a prova para `tools/reference_capture/<F-###>/input/`. Nunca abra o original: o ArtPro+ pode salvar por cima.
2. Abra o app, registre versão, arquivo, perfil de cor, unidades e zoom.
3. Tire um screenshot do estado inicial (tela inteira).

## Roteiro mínimo

1. Cada passo com a ação real (clique/tecla) e o resultado imediato.
2. Não pule diálogo intermediário — o clone precisa dele.
3. Capture o texto literal de menus, botões, tooltips e mensagens de erro.
4. Registre os campos do diálogo: `nome | tipo | default | unidade | faixa aceita`.
5. Repita com variação de borda: offset 0 / 3 mm / negativo; unidade mm / pt; entrada vazia.
6. Capture a saída: arquivo resultante, PDF exportado, valores exibidos, mensagem de erro.

## Identificador e pastas

```
CAP-AAAAMMDD-nn
tools/reference_capture/F-###/input/     cópia da prova
tools/reference_capture/F-###/output/    arquivo(s) gerados
tools/reference_capture/F-###/shots/     shot-01.png, shot-02.png ...
rag/evidence/F-###/behavior/             cópia da evidência + manifest
```

Grave `docs/behavior/<Ferramenta>.md` no formato do agente `comportamento` (versão, caminho na UI, entrada, passos, campos, saída, erros, capturas).

## Regras

- Nunca declare comportamento que não viu. Ferramenta que não abriu (HASP, crash, diálogo bloqueado) vira bloqueio com screenshot.
- Uma sessão de GUI por vez; feche diálogos pendentes antes de reportar.
- Nunca edite `src/`, `tests/`, specs, roadmap ou artefatos de Ghidra/REA.
- Nunca sobrescreva prova original.
- Se a tela mudar de escala ou resolução no meio, recalibrare com `get_screen_info` antes de continuar.

## Referências

- `references/pc-control-tools.md` — assinaturas, exemplo de fluxo e recuperação de erro.
- `references/capture-protocol.md` — template de captura, nomenclatura e checklist de completude.
