# Captura e métricas visuais

## Protocolo

1. Abra o mesmo documento no ArtPro+ e no ArtPlus.
2. Maximize; mesma resolução e escala DPI.
3. Mesmo zoom (100% ou ajustar à página), mesma posição.
4. Esconda cursor, régua e barras com hora/zoom.
5. Capture a janela inteira (não só o canvas) para pegar painéis.
6. Salve `ref/shot-01.png` e `artplus/shot-01.png` com as mesmas dimensões.
7. Se as dimensões diferirem, recapture; não redimensione.

## Diff

```powershell
python -c "from PIL import Image; import numpy as np; a=np.asarray(Image.open(r'ref\shot-01.png').convert('RGB'),float); b=np.asarray(Image.open(r'artplus\shot-01.png').convert('RGB'),float); print(a.shape,b.shape); d=np.abs(a-b); print('max',d.max(),'mean',round(d.mean(),3),'pct>8',round((d.max(2)>8).mean()*100,3))"
```

Heatmap:

```powershell
python -c "from PIL import Image; import numpy as np; a=np.asarray(Image.open(r'ref\shot-01.png').convert('RGB'),float); b=np.asarray(Image.open(r'artplus\shot-01.png').convert('RGB'),float); d=np.abs(a-b).max(2); Image.fromarray((np.clip(d*4,0,255)).astype('uint8')).save(r'diff\shot-01-diff.png')"
```

SSIM:

```powershell
python -c "from PIL import Image; import numpy as np; from skimage.metrics import structural_similarity as s; a=np.asarray(Image.open(r'ref\shot-01.png').convert('L')); b=np.asarray(Image.open(r'artplus\shot-01.png').convert('L')); print('ssim',round(s(a,b,data_range=255),5))"
```

## Laudo

```markdown
# Regressão visual F-###
Data: ...
Referência: ref/shot-01.png (ArtPro+ 25.0)
Candidato: artplus/shot-01.png (ArtPlus build ...)
Dimensões: 1920x1080 = 1920x1080
Pixels > 8/255: 0,42%
Delta máximo: 64
SSIM: 0,993

## Diferenças
1. Marca superior esquerda 1 px deslocada — geometria; tolerância 0.
2. Antialiasing no texto — aceitável.

Veredito: REPROVADO (item 1)
```

## Máscaras

Aplique máscara apenas em região dinâmica comprovada (cursor, relógio). Registre retângulo e motivo no laudo. Máscara não pode cobrir a funcionalidade testada.

## Tolerância

Comece pelos valores da skill. Ajuste só com justificativa técnica e registro; nunca para “passar”.
