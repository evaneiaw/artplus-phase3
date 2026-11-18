# Ferramentas do MCP pc-control

## Assinaturas confirmadas

```text
list_open_windows() -> { result }
get_screen_info() -> { result }
get_elements(window_title, element_type?, name_filter?, max_depth=8) -> { result }
click_element(window_title, element_name, element_type?) -> { result }
mouse_click(x, y, button="left", clicks=1, hold_keys?) -> { result }
mouse_drag(start_x, start_y, end_x, end_y, button="left", duration=0.5) -> { result }
keyboard_type(text) -> { result }
keyboard_hotkey(keys: string[]) -> { result }
```

Também existem `screenshot`, `mouse_scroll`, `open_application` e `wait`; os parâmetros exatos estão no schema da ferramenta — consulte antes de chamar.

## Fluxo seguro

```text
1. list_open_windows()                     -> confirme que o ArtPro+ está aberto
2. open_application(...)                   -> se não estiver
3. wait(2)                                 -> deixe a janela estabilizar
4. get_screen_info()                       -> monitores, cursor, escala
5. screenshot(...)                         -> estado inicial
6. get_elements("ArtPro+", name_filter=)   -> descubra o controle
7. click_element("ArtPro+", "<nome>")      -> clique por nome
8. keyboard_type("3")                      -> preencha o campo focado
9. keyboard_hotkey(["ctrl","s"])           -> salve em cópia
10. screenshot(...)                        -> resultado
```

## Quando usar coordenada

Só quando o controle não aparece em `get_elements` (canvas, régua, diálogo custom). Tire screenshot antes e depois; registre `x,y` no roteiro para reproduzir.

## Recuperação

| Sintoma | Ação |
|---|---|
| `get_elements` vazio | janela errada ou app minimizado; confirme título e traga para frente |
| clique não surtiu efeito | `wait` maior, valide screenshot, confirme que o campo tem foco |
| diálogo modal bloqueando | feche com `keyboard_hotkey(["esc"])` antes de seguir |
| escala DPI diferente | recalibre com `get_screen_info`; não reutilize coordenada antiga |
| app não abre | registre bloqueio (licença HASP/crash) com screenshot |

## Regras

- Nunca dois agentes usando mouse/teclado ao mesmo tempo.
- Nunca clique em “Salvar” no arquivo original; use cópia.
- Não invente nome de ferramenta: o servidor expõe exatamente as acima.
