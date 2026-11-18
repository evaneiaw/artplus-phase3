---
name: Marks and Barcodes
description: Cria e posiciona marcas de produção no ArtPlus — corte, registro, barra de cor, número de página, vinco, dobra, sangria, alvos e marcas customizadas. Use ao implementar MarksEngine, posicionar marca relativa às caixas ou validar saída de marcas.
metadata:
  version: "2"
---

# Marcas e códigos de barra

Código: `src/prepress/marks.h/.cpp` (`MarksEngine`).

## Tipos já modelados

`MarkCropSettings`, `MarkRegistrationSettings`, `MarkColorBarSettings`, `MarkPageNumberSettings`, `MarkCutMarkSettings`, `MarkCreaseMarkSettings`, `MarkFoldMarkSettings`, `MarkBleedMarkSettings`, `MarkColorBarPatchesSettings`, `MarkGrayBalanceSettings`, `MarkResolutionTargetSettings`, `MarkSlurGaugeSettings`, `MarkStarTargetSettings`, `MarkCustomMark`, `MarkSettings` e `MarkType`.

## Regras

1. **Unidade explícita**: offset, comprimento e espessura em `mm` na spec e no nome do campo.
2. **Referência é a caixa**: posicione relativo a trim/bleed/media, nunca a coordenada solta. Declare qual caixa.
3. **Marca fora do trim**: por padrão não invade a área de corte; exceção só com flag e evidência.
4. **Camada de marcas**: marcas vivem em camada própria, não imprimível no conteúdo, mas imprimível na saída de produção.
5. **Cor**: marcas usam a tinta definida (registration ou preta); não force RGB.
6. **Repetição por página**: ao aplicar em documento multipágina, cada página recebe sua geometria de caixa.
7. **Barcode**: simbologia, conteúdo, altura, módulo e quiet zone declarados; não desenhe barra “aproximada”.
8. **Custom mark**: conteúdo e posição vêm do usuário; valide e não execute nada embutido.

## Evidência

Captura do diálogo de cada marca (campos, default, faixa), screenshot do resultado e PDF exportado. O texto literal do menu (`Marks > Create > ...`) é obrigatório na spec.

## Testes

- Offset 0 / 3 mm / negativo → geometria e validação.
- Página com bleed diferente do media → marca segue a caixa correta.
- Documento multipágina → marcas por página.
- PDF exportado contém a marca na camada/tinta correta.

## Limites

- Não invente parâmetro que o diálogo do ArtPro+ não mostra.
- Não edite `src/document/` para posicionar marca.
