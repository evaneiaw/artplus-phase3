# PDF → modelo ArtPlus

## Mapeamento

| PDF | Modelo |
|---|---|
| Page + boxes | `Page`, `PageBoxes` (pt) |
| Content stream | `PathObject`/`TextObject`/`ImageObject` |
| Form XObject | grupo/objeto com `QTransform` composta |
| `/SMask` | `OpacityMaskObject` (Alpha/Luminosity) |
| `W`/`W*` | `ClippingMaskObject` (`evenOdd`) |
| `/GS` com `/ca`,`/CA`,`/BM` | `FillStroke::fillAlpha/strokeAlpha` + blend |
| `k`/`K` | `Color::cmyk` |
| `scn` com nome | `Color::spot` |
| `cs` DeviceN | `Color` DeviceN (quando suportado) |
| `Tj`/`TJ` | `TextObject` |
| `Do` de imagem | `ImageObject` |

## Armadilhas

1. **Matriz**: Form XObject tem `/Matrix`; a transformação final é `parent * form`. Não substitua.
2. **Ordem**: `q`/`Q` empilham estado; ignore e a cor/clip vaza para o próximo objeto.
3. **Clipping**: `W` só vale até o `Q`; não descarte.
4. **Transparência**: grupo com `/K` (knockout) não é o mesmo que opacidade.
5. **Cor**: não converta para RGB no import; preservar CMYK/Spot é requisito de prepress.
6. **Fonte**: `Tf` referencia recurso de fonte; sem embedding, o texto pode não renderizar igual.
7. **Unidades**: PDF é 1/72"; o modelo também. Não converta para mm no import.
8. **Rotação**: `/Rotate` é da página, não do conteúdo; trate separado.
9. **Stream**: decodifique conforme `/Filter`; não assuma Flate sempre.
10. **Raw offset**: preserve para reexportar objetos não editados.

## Export

- Reemita objeto não editado a partir de `rawOffset`.
- Objeto novo: escreva content stream coerente com o estado (`q`/`Q`).
- Não reencode imagem já JPEG sem necessidade.
- Flate via `Qt6::ZlibPrivate`.
- Valide com roundtrip e comparação visual.
