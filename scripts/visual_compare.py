#!/usr/bin/env python3
"""Compare two renderings and produce metrics, heatmap and a report.

Uses Pillow, NumPy and scikit-image (already installed in this environment).

Examples:
  python scripts/visual_compare.py ref.png cand.png --out tools/visual_compare/F-025
  python scripts/visual_compare.py ref.png cand.png --max-pct 0.5 --min-ssim 0.99
  python scripts/visual_compare.py ref.png cand.png --mask 0,0,200,40 --mask 1800,0,120,60
"""
from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

try:
    import numpy as np
    from PIL import Image
    from skimage.metrics import structural_similarity
except ImportError as exc:  # pragma: no cover
    print(f"dependencia ausente: {exc}", file=sys.stderr)
    sys.exit(3)


def load_rgb(path: Path) -> np.ndarray:
    with Image.open(path) as image:
        return np.asarray(image.convert("RGB"), dtype=np.float64)


def parse_mask(value: str):
    parts = [int(part) for part in value.split(",")]
    if len(parts) != 4:
        raise argparse.ArgumentTypeError("mascara deve ser x,y,w,h")
    return tuple(parts)


def main() -> int:
    parser = argparse.ArgumentParser(description="Visual regression compare")
    parser.add_argument("reference")
    parser.add_argument("candidate")
    parser.add_argument("--out", default="tools/visual_compare/compare")
    parser.add_argument("--threshold", type=int, default=8, help="delta per channel (0-255)")
    parser.add_argument("--max-pct", type=float, default=0.1, help="max %% pixels above threshold")
    parser.add_argument("--min-ssim", type=float, default=0.995)
    parser.add_argument("--mask", action="append", type=parse_mask, default=[],
                        help="ignore region x,y,w,h (repeatable)")
    parser.add_argument("--no-diff", action="store_true")
    args = parser.parse_args()

    ref_path, cand_path = Path(args.reference), Path(args.candidate)
    for path in (ref_path, cand_path):
        if not path.is_file():
            print(f"arquivo nao encontrado: {path}", file=sys.stderr)
            return 2

    reference, candidate = load_rgb(ref_path), load_rgb(cand_path)
    if reference.shape != candidate.shape:
        print(f"dimensoes diferentes: {reference.shape} vs {candidate.shape}", file=sys.stderr)
        return 2

    delta = np.abs(reference - candidate)
    valid = np.ones(delta.shape[:2], dtype=bool)
    for x, y, width, height in args.mask:
        valid[max(0, y):y + height, max(0, x):x + width] = False

    per_pixel = delta.max(axis=2)
    examined = per_pixel[valid]
    pct_over = float((examined > args.threshold).mean() * 100.0) if examined.size else 0.0
    max_delta = float(examined.max()) if examined.size else 0.0
    mean_delta = float(examined.mean()) if examined.size else 0.0

    gray_ref = reference.mean(axis=2)
    gray_cand = candidate.mean(axis=2)
    ssim = float(structural_similarity(gray_ref, gray_cand, data_range=255))

    passed = pct_over <= args.max_pct and ssim >= args.min_ssim

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    metrics = {
        "reference": str(ref_path),
        "candidate": str(cand_path),
        "generated": datetime.now(timezone.utc).isoformat(),
        "width": int(reference.shape[1]),
        "height": int(reference.shape[0]),
        "threshold": args.threshold,
        "max_pct": args.max_pct,
        "min_ssim": args.min_ssim,
        "pct_over_threshold": round(pct_over, 4),
        "max_delta": round(max_delta, 3),
        "mean_delta": round(mean_delta, 4),
        "ssim": round(ssim, 6),
        "masks": [list(item) for item in args.mask],
        "verdict": "PASS" if passed else "FAIL",
    }
    (out_dir / "metrics.json").write_text(
        json.dumps(metrics, ensure_ascii=False, indent=2), encoding="utf-8")

    if not args.no_diff:
        heat = np.clip(per_pixel * 4.0, 0, 255).astype(np.uint8)
        heat[~valid] = 0
        Image.fromarray(heat).save(out_dir / "diff.png")

    report = [
        "# Regressao visual",
        "",
        f"- Referencia: `{ref_path}`",
        f"- Candidato: `{cand_path}`",
        f"- Dimensoes: {metrics['width']}x{metrics['height']}",
        f"- Pixels acima de {args.threshold}/255: **{pct_over:.3f}%** (limite {args.max_pct}%)",
        f"- Delta maximo: {max_delta:.1f}",
        f"- Delta medio: {mean_delta:.2f}",
        f"- SSIM: {ssim:.6f} (minimo {args.min_ssim})",
    ]
    if args.mask:
        report.append(f"- Mascaras: {', '.join(str(m) for m in args.mask)}")
    report += ["", f"## Veredito: {'APROVADO' if passed else 'REPROVADO'}", ""]
    (out_dir / "report.md").write_text("\n".join(report), encoding="utf-8")

    print(json.dumps(metrics, ensure_ascii=False))
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
