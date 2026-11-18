# Toolbox PDF

## PyMuPDF (fitz)

Metadados e estrutura:

```powershell
python -c "import fitz; d=fitz.open(r'C:\temp\a.pdf'); print(d.metadata); print('pages',d.page_count,'xrefs',d.xref_length()); print('xobjects',d.get_page_xobjects(0))"
```

Render:

```powershell
python -c "import fitz; d=fitz.open(r'C:\temp\a.pdf'); d[0].get_pixmap(dpi=144).save(r'C:\temp\a.png')"
```

Texto e imagens:

```powershell
python -c "import fitz; d=fitz.open(r'C:\temp\a.pdf'); p=d[0]; print(p.get_text()[:500]); print(len(p.get_images(full=True)))"
```

Objeto por xref:

```powershell
python -c "import fitz; d=fitz.open(r'C:\temp\a.pdf'); print(d.xref_object(1)[:800])"
```

## pypdfium2

Render independente (bom para confirmar que o problema não é do fitz):

```powershell
python -c "import pypdfium2 as p; d=p.PdfDocument(r'C:\temp\a.pdf'); d[0].render(scale=2).to_pil().save(r'C:\temp\b.png')"
```

## pdfminer.six

Texto com layout:

```powershell
python -c "from pdfminer.high_level import extract_text; print(extract_text(r'C:\temp\a.pdf')[:500])"
```

## reportlab

Gerar fixture controlada:

```powershell
python -c "from reportlab.pdfgen import canvas; c=canvas.Canvas(r'C:\temp\t.pdf'); c.drawString(72,720,'teste'); c.rect(72,600,100,50); c.save()"
```

## fonttools

```powershell
python -c "from fontTools.ttLib import TTFont; f=TTFont(r'C:\temp\f.ttf'); print(f['name'].getDebugName(1))"
```

## Diagnóstico rápido

| Sintoma | Verificar |
|---|---|
| página em branco | `/Contents` vazio, matriz, clip, cor branca |
| imagem invertida | matriz do Form XObject / `/SMask` |
| texto some | fonte não embutida, encoding, `Tj` vs `TJ` |
| cor errada | espaço de cor, `/Decode`, overprint |
| roundtrip cresce | stream reencodado sem necessidade |
