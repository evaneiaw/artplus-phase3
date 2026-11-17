import os
import sys
from pathlib import Path
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4
from reportlab.lib.colors import Color, CMYKColor, CMYKColorSep
from reportlab.lib.units import mm

out_dir = Path(r"H:\Desktop\ArtPlus\tests\fixtures")
out_dir.mkdir(parents=True, exist_ok=True)

A4W, A4H = A4

def path_of(name):
    return str(out_dir / name)

# ---------- 1. Text PDF ----------
c = canvas.Canvas(path_of("test_text.pdf"), pagesize=A4)
c.setTitle("ArtPlus Text Test")
c.setAuthor("ArtPlus QA")
c.setFillColorRGB(1, 0, 0)
c.setFont("Helvetica-Bold", 24)
c.drawString(72, A4H - 72, "ArtPlus Text Test")
c.setFont("Times-Roman", 12)
c.setFillColorRGB(0, 0, 0)
c.drawString(72, A4H - 110, "The quick brown fox jumps over the lazy dog 0123456789")
c.setFont("Courier", 10)
c.drawString(72, A4H - 130, "Courier sample line")
c.showPage()
c.save()

# ---------- 2. Vector PDF (CMYK + spot) ----------
c = canvas.Canvas(path_of("test_vector.pdf"), pagesize=A4)
c.setTitle("ArtPlus Vector Test")
# CMYK filled rect
c.setFillColorCMYK(0.2, 0.8, 0.1, 0.0)
c.rect(50, 700, 150, 80, fill=1, stroke=0)
# CMYK stroked path with curve
c.setStrokeColorCMYK(0.9, 0.1, 0.9, 0.0)
c.setLineWidth(3)
p = c.beginPath()
p.moveTo(50, 650)
p.curveTo(120, 700, 200, 600, 280, 650)
c.drawPath(p, fill=0, stroke=1)
# spot color fill
try:
    spot = CMYKColorSep(0.2, 0.9, 0.2, 0.0, spotName="PANTONE 485 C", density=100)
    c.setFillColor(spot)
    c.rect(50, 500, 100, 60, fill=1, stroke=0)
    c.setFillColorRGB(0, 0, 0)
    c.setFont("Helvetica", 10)
    c.drawString(50, 480, "Spot: PANTONE 485 C")
except Exception as e:
    print("spot failed:", e)
# dashed line
c.setDash(6, 4)
c.setStrokeColorRGB(0, 0.5, 0)
c.setLineWidth(2)
c.line(50, 450, 350, 450)
c.setDash()
# polygon
c.setFillColorCMYK(0.1, 0.1, 0.8, 0.0)
p = c.beginPath()
p.moveTo(400, 600)
p.lineTo(480, 750)
p.lineTo(560, 600)
p.close()
c.drawPath(p, fill=1, stroke=1)
c.showPage()
c.save()

# ---------- 3. Images PDF ----------
from PIL import Image
img_path = out_dir / "sample_image.png"
img = Image.new("RGB", (120, 90), (200, 30, 30))
for x in range(120):
    for y in range(90):
        img.putpixel((x, y), (x % 256, y % 256, (x * y) % 256))
img.save(img_path)

c = canvas.Canvas(path_of("test_images.pdf"), pagesize=A4)
c.setTitle("ArtPlus Images Test")
c.drawImage(str(img_path), 72, 600, width=240, height=180)
c.setFillColorRGB(0, 0, 0)
c.setFont("Helvetica", 12)
c.drawString(72, 560, "Embedded raster image 240x180pt")
c.showPage()
c.save()

# ---------- 4. Multi-page with clipping + transparency ----------
c = canvas.Canvas(path_of("test_multipage.pdf"), pagesize=A4)
c.setTitle("ArtPlus Multipage Test")
# Page 1: text + vector
c.setFillColorRGB(0.1, 0.1, 0.7)
c.rect(72, 700, 120, 50, fill=1, stroke=0)
c.setFillColorRGB(0, 0, 0)
c.setFont("Helvetica-Bold", 28)
c.drawString(72, 660, "Page 1 of 3")
c.showPage()
# Page 2: clipping path (circle clip)
c.saveState()
p = c.beginPath()
p.circle(200, 500, 80)
c.clipPath(p, stroke=0, fill=0)
c.setFillColorCMYK(0.0, 0.7, 0.9, 0.0)
c.rect(50, 350, 400, 400, fill=1, stroke=0)
c.restoreState()
c.setFillColorRGB(0, 0, 0)
c.setFont("Helvetica", 14)
c.drawString(72, 300, "Page 2: clipped orange square")
c.showPage()
# Page 3: transparency (alpha)
c.setFillColorRGB(0, 0, 0)
c.setFont("Helvetica", 14)
c.drawString(72, 250, "Page 3: transparent overlays")
c.setFillAlpha(0.5)
c.setFillColorCMYK(0.9, 0.0, 0.2, 0.0)
c.rect(100, 500, 200, 150, fill=1, stroke=0)
c.setFillColorCMYK(0.0, 0.9, 0.3, 0.0)
c.rect(200, 450, 200, 150, fill=1, stroke=0)
c.setFillAlpha(1)
c.showPage()
c.save()

# ---------- 5. Spot/CMYK inks PDF ----------
c = canvas.Canvas(path_of("test_cmyk_inks.pdf"), pagesize=A4)
c.setTitle("ArtPlus CMYK and Spot Test")
c.setFillColorCMYK(0.0, 1.0, 1.0, 0.0)
c.rect(50, 700, 120, 60, fill=1, stroke=0)
c.setFillColorCMYK(0.0, 0.0, 1.0, 0.0)
c.rect(50, 600, 120, 60, fill=1, stroke=0)
c.setFillColorCMYK(1.0, 0.0, 0.0, 0.0)
c.rect(50, 500, 120, 60, fill=1, stroke=0)
c.setFillColorCMYK(0.0, 0.0, 0.0, 1.0)
c.rect(50, 400, 120, 60, fill=1, stroke=0)
try:
    spot = CMYKColorSep(0.0, 0.9, 0.9, 0.0, spotName="MySpot", density=100)
    c.setFillColor(spot)
    c.rect(250, 500, 120, 60, fill=1, stroke=0)
    c.setFillColorRGB(0, 0, 0)
    c.setFont("Helvetica", 10)
    c.drawString(250, 480, "Spot: MySpot")
except Exception as e:
    print("spot failed:", e)
c.showPage()
c.save()

print("Test PDFs generated in", out_dir)
for p in sorted(out_dir.glob("*.pdf")):
    print(" ", p.name, p.stat().st_size, "bytes")
