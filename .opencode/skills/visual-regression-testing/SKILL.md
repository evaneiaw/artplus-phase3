---
name: Visual Regression Testing
description: Compara renderizações do ArtPro+ real e do ArtPlus — captura padronizada, alinhamento, diff por pixel/SSIM, máscaras de regiões dinâmicas e tolerância declarada. Use ao validar UI, marcas, trapping, step & repeat ou export visual contra o original.
metadata:
  version: "1"
---

# Regressão visual ArtPro+ × ArtPlus

## Padronização da captura

Sem isso, o diff mede ruído:

- mesmo documento de entrada e mesmo zoom;
- mesma resolução de tela e escala DPI (`get_screen_info`);
- mesmo perfil de cor, unidades e tema do app;
- mesma janela maximizada e mesma posição;
- fontes instaladas e iguais;
- cursor fora da área capturada;
- barras de status/régua mascaradas se mostrarem hora, zoom ou coordenada.

Capture o ArtPro+ primeiro (referência) e o ArtPlus depois, sem mudar nada no ambiente.

## Estrutura

```
tools/visual_compare/F-###/
├── ref/          shot do ArtPro+ (PNG)
├── artplus/      shot do ArtPlus (PNG)
├── diff/         heatmap e diff amplificado
└── report.md     métricas, máscaras e veredito
```

## Métricas

```powershell
python -c "from PIL import Image; import numpy as np; a=np.asarray(Image.open(r'ref.png').convert('RGB'),float); b=np.asarray(Image.open(r'artplus.png').convert('RGB'),float); d=np.abs(a-b); print('shape',a.shape,b.shape); print('max',d.max(),'mean',d.mean()); print('pct>8', (d.max(2)>8).mean()*100)"
```

SSIM (scikit-image instalado):

```powershell
python -c "from PIL import Image; import numpy as np; from skimage.metrics import structural_similarity as s; a=np.asarray(Image.open(r'ref.png').convert('L')); b=np.asarray(Image.open(r'artplus.png').convert('L')); print('ssim', s(a,b,data_range=255))"
```

Relate sempre: dimensões iguais, `%` de pixels acima do limiar, delta máximo, SSIM.

## Tolerância

| Área | Limiar inicial | Justificativa |
|---|---|---|
| UI estática | 0,1% pixels > 8/255 e SSIM ≥ 0,995 | antialiasing varia |
| Geometria de marca | 0,5% e deslocamento ≤ 1 px | arredondamento de traço |
| Cor de prova | DeltaE ≤ 2 | percepção |
| Texto | 0% na caixa do texto, SSIM ≥ 0,99 | fonte deve ser idêntica |

Tolerância é decisão registrada na spec, não número escolhido para passar.

## Classificação da diferença

1. **Geometria** — posição, tamanho, rotação, espessura.
2. **Cor** — tom, opacidade, overprint.
3. **Tipografia** — fonte, kerning, hinting.
4. **Antialiasing** — aceitável dentro da tolerância.
5. **Conteúdo ausente/extra** — sempre falha.

Diferencie antes de aceitar. Heatmap sem inspeção não é veredito.

## Regras

- Nunca compare capturas de ambientes diferentes.
- Nunca ajuste tolerância para esconder diferença de geometria.
- Máscara só para região comprovadamente dinâmica; registre a região e o motivo.
- Preserve os PNGs originais; o diff é derivado.
- Um `F-###` por pasta; capturas nomeadas `shot-01.png`.

## Referências

- `references/capture-and-metrics.md` — protocolo de captura, scripts e laudo modelo.
- `tools/visual_compare/` — destino dos artefatos.
