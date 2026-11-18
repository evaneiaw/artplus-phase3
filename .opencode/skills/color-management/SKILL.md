---
name: Color Management
description: Gerencia cor de prepress no ArtPlus — perfis ICC, conversões CMYK/RGB/Lab/Gray, tintas spot e DeviceN, simulação de prova, DeltaE e separações. Use ao implementar conversão de cor, importar PDF com tintas, validar overprint ou comparar cor contra o ArtPro+.
metadata:
  version: "1"
---

# Gerenciamento de cor

## Código atual

| Arquivo | Conteúdo |
|---|---|
| `src/color/color_manager.h` | `ColorManager` (singleton): perfis, conversões, spot, DeviceN, simulação, DeltaE |
| `src/color/color_profile.h` | parser ICC: header, tags, device class, PCS, iluminante |
| `src/color/color_transform.h` | transformação entre espaços/perfis |

`Color` (em `document_model.h`): `type` = CMYK/Spot/RGB/Lab/Gray/Pattern/None, `components` (0..1), `spotName`, `tint`, `profileName`.

Não há CMM externo linkado no `CMakeLists.txt`. Se propuser ICC/lcms2, exige ADR e licença.

## Conceitos que não podem ser confundidos

| Conceito | Regra |
|---|---|
| Device CMYK | 4 componentes, sem nome de tinta, dependente do perfil |
| Spot | tinta nomeada + `tint`; alternativo CMYK/RGB para prova |
| DeviceN | lista de colorantes + valores; usado por PDF com múltiplas tintas |
| Separação | canal de saída (processo ou spot), derivado das tintas usadas |
| Overprint | interação de tintas na saída, não é opacidade |
| Simulação | perfil de destino para prever a impressão; não altera o dado |
| DeltaE | distância perceptual; tolerância típica 1–2 para “match” |

`QColor` serve para UI; **nunca** como verdade de cor de prepress.

## Regras

- Componentes sempre normalizados 0..1; converta para 0..255 só na borda de exibição.
- Não descarte `spotName`/`tint` ao importar, copiar ou serializar.
- Conversão declara perfil de origem, destino e `RenderingIntent`.
- Comparação de cor usa `ColorManager::colorDifference` com fórmula e tolerância explícitas; nunca `==` em float.
- Overprint e transparência são propriedades separadas; não simule overprint com alpha.
- Amostra de separação é derivada; não guarde cópia que possa divergir.
- Perfil ICC é dado binário; valide assinatura/tamanho antes de parsear.
- Resultado de conversão sem perfil de origem definido é inferência: registre.

## Evidência

Toda regra de cor precisa de:

- captura do ArtPro+ mostrando valores de entrada/saída (skill `artpro-behavior-capture`);
- PDF de referência em `rag/evidence/F-###/behavior/`;
- valor esperado com tolerância DeltaE e condições (perfil, intent, iluminante).

Sem isso, a spec de cor é chute.

## Testes

- Unitário: conversão ida/volta dentro da tolerância; spot preserva nome/tint.
- Golden: amostra de cor/saída comparada com o PDF do original.
- Borda: tinta sem alternativo, tint 0 e 1, DeviceN com 1 e N colorantes, perfil inválido.

## Limites

- Não invente nome de perfil ou intent que o ArtPro+ não usa.
- Não trate cor como RGB em nenhum caminho de prepress.
- Não altere o modelo de `Color` sem ADR.

## Referências

- `references/color-glossary.md` — termos, fórmulas e checklist de validação.
