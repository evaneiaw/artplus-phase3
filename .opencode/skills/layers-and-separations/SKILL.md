---
name: Layers and Separations
description: Gerencia camadas, tintas, separações e overprint no ArtPlus — visibilidade, lock, printable, ordem, cor de camada, spot e canais de saída. Use ao implementar painel de camadas, separação de cores, overprint ou exportação por tinta.
metadata:
  version: "2"
---

# Camadas e separações

Código: `Layer` e `Document` em `src/document/document_model.*`; cor em `src/color/`.

## Modelo

`Layer`: `name`, `visible`, `locked`, `printable`, `color` (cor de UI), `opacity`, ordem de objetos.
`Document`: ordem de camadas (`layerOrder`) e mapa de objetos.
`Color`: CMYK/Spot/DeviceN/tint; separação é derivada das tintas usadas.

## Regras

1. **Ordem importa**: objeto pertence a uma camada e a camada tem ordem; render e export seguem essa ordem.
2. **`printable=false`** exclui a camada da saída, mas mantém no documento e na UI.
3. **`visible=false`** afeta exibição, não o dado; export por padrão respeita printable, não visible (confirme na spec).
4. **Cor de camada** é identificação visual, não tinta.
5. **Separação** é calculada: processo (C/M/Y/K) + cada spot usado. Não crie canal para tinta ausente.
6. **Spot** preserva nome e `tint`; conversão para processo é alternativa de prova, não substituição.
7. **Overprint** é propriedade de pintura, não de camada; não confunda com opacidade.
8. **Camada bloqueada** rejeita edição com erro explícito; não ignore silenciosamente.
9. Ao remover camada, objetos precisam ser removidos ou realocados atomicamente.

## Evidência

Captura do painel de camadas e do diálogo de separações do ArtPro+; PDF com tintas spot; valores de overprint. Sem isso, não invente default.

## Testes

- Criar/remover/reordenar camada → ordem e objetos consistentes.
- Camada não imprimível → ausente na saída, presente no documento.
- Duas tintas spot iguais por nome → um canal; nomes diferentes → dois.
- Undo de mudança de camada restaura ordem e vínculos.

## Limites

- Não coloque regra de separação em `src/ui/`.
- Não altere `Layer` sem ADR.
