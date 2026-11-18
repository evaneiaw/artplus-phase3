#!/usr/bin/env python3
"""Inspect a PDF with PyMuPDF: metadata, pages, XObjects, fonts, images and text.

Examples:
  python scripts/pdf_inspect.py tests/fixtures/test_vector.pdf
  python scripts/pdf_inspect.py a.pdf --text --objects
  python scripts/pdf_inspect.py a.pdf --render 144 --out build/pdf_render
  python scripts/pdf_inspect.py a.pdf --json
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

try:
    import fitz  # PyMuPDF
except ImportError as exc:  # pragma: no cover
    print(f"dependencia ausente: {exc}", file=sys.stderr)
    sys.exit(3)

try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass


def page_info(page) -> dict:
    return {
        "number": page.number + 1,
        "rect": [round(value, 2) for value in (page.rect.x0, page.rect.y0,
                                               page.rect.x1, page.rect.y1)],
        "rotation": page.rotation,
        "xobjects": len(page.get_xobjects()),
        "images": len(page.get_images(full=True)),
        "fonts": len(page.get_fonts(full=True)),
        "text_chars": len(page.get_text()),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Inspect a PDF")
    parser.add_argument("pdf")
    parser.add_argument("--text", action="store_true", help="print extracted text")
    parser.add_argument("--objects", action="store_true", help="print xref objects")
    parser.add_argument("--render", type=int, metavar="DPI", help="render pages to PNG")
    parser.add_argument("--out", default="build/pdf_render")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    path = Path(args.pdf)
    if not path.is_file():
        print(f"arquivo nao encontrado: {path}", file=sys.stderr)
        return 2

    document = fitz.open(path)
    if document.needs_pass:
        print("PDF criptografado: senha necessaria", file=sys.stderr)
        return 2

    report = {
        "path": str(path),
        "page_count": document.page_count,
        "metadata": {key: value for key, value in (document.metadata or {}).items() if value},
        "pages": [page_info(page) for page in document],
    }

    if args.render:
        out_dir = Path(args.out)
        out_dir.mkdir(parents=True, exist_ok=True)
        rendered = []
        for page in document:
            target = out_dir / f"page-{page.number + 1:03d}.png"
            page.get_pixmap(dpi=args.render).save(target)
            rendered.append(str(target))
        report["rendered"] = rendered

    if args.text:
        report["text"] = [page.get_text() for page in document]

    if args.objects:
        limit = min(document.xref_length(), 25)
        report["objects_sample"] = {
            str(xref): document.xref_object(xref)[:400] for xref in range(1, limit)
        }

    if args.json:
        print(json.dumps(report, ensure_ascii=False, indent=2))
        return 0

    print(f"arquivo: {report['path']}")
    print(f"paginas: {report['page_count']}")
    for key, value in report["metadata"].items():
        print(f"  {key}: {value}")
    for page in report["pages"]:
        print(f"  pagina {page['number']}: rect={page['rect']} rot={page['rotation']} "
              f"xobjects={page['xobjects']} imagens={page['images']} "
              f"fontes={page['fonts']} chars={page['text_chars']}")
    if args.text:
        for index, text in enumerate(report["text"], start=1):
            print(f"\n--- texto pagina {index} ---\n{text[:2000]}")
    if args.objects:
        print("\n--- objetos (amostra) ---")
        for xref, body in report["objects_sample"].items():
            print(f"\n{xref}: {body}")
    if args.render:
        print(f"\nrenderizado em: {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
