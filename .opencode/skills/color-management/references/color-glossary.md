# Glossário de cor

| Termo | Definição curta |
|---|---|
| Espaço de cor | Modelo de representação: CMYK, RGB, Gray, Lab, DeviceN |
| Perfil ICC | Caracterização de dispositivo; header + tags |
| PCS | Profile Connection Space (Lab/XYZ) |
| Rendering intent | Perceptual, RelativeColorimetric, AbsoluteColorimetric, Saturation |
| Gamut | Faixa de cores reproduzível |
| DeltaE | Distância perceptual entre cores; 2000 é o padrão atual |
| Tinta spot | Tinta nomeada, fora do CMYK |
| DeviceN | Múltiplos colorantes nomeados com valores |
| Separação | Canal de saída por tinta |
| Overprint | Impressão de uma tinta sobre outra sem remover a de baixo |
| Knockout | O oposto: a de baixo é removida |
| Simulação | Previsão com perfil de destino |
| Tint | Percentual de uma tinta spot (0..1) |

## Fórmulas úteis

- mm → pt: `pt = mm * 72 / 25.4`
- pt → mm: `mm = pt * 25.4 / 72`
- Componente 0..1 → 0..255: `round(v * 255)`

## Checklist de validação de cor

- [ ] Espaço de origem e destino declarados.
- [ ] Intent declarado.
- [ ] Spot preserva nome e tint.
- [ ] Overprint não foi simulado com alpha.
- [ ] DeltaE com fórmula e tolerância.
- [ ] Amostra de separação derivada, não armazenada.
- [ ] PDF de referência e captura do original anexados.

## Erros comuns

- Usar `QColor` como valor de prepress.
- Converter CMYK → RGB no import e perder a tinta.
- Comparar float com `==`.
- Assumir que todo PDF é RGB.
- Tratar “preto” como K=100 sem consultar a regra de preto do original.
