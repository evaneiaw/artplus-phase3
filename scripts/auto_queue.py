#!/usr/bin/env python3
"""Choose the next modules for an overnight extraction run.

Ranking follows docs/agents/AUTOPILOT.md: finish in-progress work, prioritize
UI/document modules, skip third-party libraries and huge executables.

Examples:
  python scripts/auto_queue.py --limit 6
  python scripts/auto_queue.py --limit 10 --json
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
from pathlib import Path

try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass

THIRD_PARTY = (
    "qt", "opencv", "icu", "xerces", "openimageio", "openexr", "zlib", "boost",
    "aws", "d3d", "api-ms", "vcruntime", "msvcp", "ucrt", "libcrypto", "libssl",
    "python", "tbb", "jpeg", "png", "tiff", "freetype", "harfbuzz", "hunspell",
    "pcre", "sqlite", "curl", "expat", "xml", "jasper", "webp", "avif", "brotli",
    "zstd", "lzma", "crypto", "ssl", "winhttp", "dbghelp", "dxgi", "dwrite",
    "ole32", "shell32", "user32", "gdi32", "advapi32", "kernel32", "ntdll",
)

UI_HINTS = (
    "document", "artpro", "pdf", "marks", "mark", "color", "trap", "step",
    "impose", "packag", "plate", "layer", "separ", "ink", "preflight",
    "screen", "halftone", "action", "job", "ticket", "output", "proof",
)

CORE_BONUS = {
    "documentmarks.dll": 500,
    "pdfobjectactions.dll": 450,
    "ndlmodellocal.dll": 350,
    "artpro+.exe_libfnp.dll": 350,
    "documentflexotools.dll": 300,
    "documenttrap.dll": 250,
    "documentedit.dll": 250,
    "documenteditseparations.dll": 200,
    "artprocore.dll": 200,
    "colormanipulationlayer.dll": 150,
}


def load_rows(root: Path):
    path = root / "rag" / "metadata" / "modules.csv"
    if not path.is_file():
        return []
    rows = []
    with path.open(encoding="utf-8-sig", errors="replace", newline="") as handle:
        for row in csv.DictReader(handle):
            name = (row.get("name") or "").strip()
            if name:
                rows.append(row)
    return rows


def has_export(root: Path, name: str) -> bool:
    base = root / "rag" / "ghidra" / "exports"
    if not base.is_dir():
        return False
    stem = Path(name).stem.lower()
    for path in base.iterdir():
        if path.is_dir() and (path.name.lower() == name.lower()
                              or Path(path.name).stem.lower() == stem):
            return True
    return False


def has_dossier(root: Path, name: str) -> bool:
    return (root / "dossiers" / Path(name).stem / "DOSSIER.md").is_file()


def score(root: Path, row):
    name = row.get("name", "")
    lower = name.lower()
    try:
        size = int(row.get("size_bytes") or 0)
    except ValueError:
        size = 0
    mb = size / (1024 * 1024)
    status = (row.get("status") or "nao-tratado").strip()
    value = 0
    reasons = []
    if status != "nao-tratado":
        value += 1000
        reasons.append(f"em andamento ({status})")
    if any(hint in lower for hint in UI_HINTS):
        value += 500
        reasons.append("UI/documento")
    if lower.startswith(("document", "pdf", "artpro", "ndl", "color", "trap")):
        value += 200
        reasons.append("nucleo")
    bonus = CORE_BONUS.get(lower)
    if bonus:
        value += bonus
        reasons.append("modulo central")
    if has_export(root, name):
        value += 400
        reasons.append("export Ghidra existente")
    if mb < 0.05:
        value -= 2000
        reasons.append("tamanho nulo")
    elif mb < 0.3:
        value -= 500
        reasons.append("muito pequeno")
    elif mb < 1:
        value += 100
        reasons.append("<1 MB")
    elif mb <= 8:
        value += 300 + int(mb * 20)
        reasons.append("1-8 MB")
    elif mb <= 12:
        value += 300
        reasons.append("8-12 MB")
    elif mb <= 20:
        value += 50
        reasons.append("12-20 MB")
    else:
        value -= 100
        reasons.append(">20 MB")
    for bad in ("support", "test", "hook", "stub", "dummy"):
        if bad in lower:
            value -= 300
            reasons.append(f"apoio ({bad})")
    value -= int(mb * 5)
    if lower.endswith(".exe") and mb > 20:
        value -= 2000
        reasons.append("exe grande")
    return value, reasons


def main() -> int:
    parser = argparse.ArgumentParser(description="Rank modules for the overnight run")
    parser.add_argument("--root", default=str(Path(__file__).resolve().parents[1]))
    parser.add_argument("--limit", type=int, default=6)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--include-done", action="store_true",
                        help="include modules that already have a dossier")
    args = parser.parse_args()
    root = Path(args.root).resolve()

    candidates = []
    for row in load_rows(root):
        name = row.get("name", "")
        lower = name.lower()
        if any(part in lower for part in THIRD_PARTY):
            continue
        if not args.include_done and has_dossier(root, name):
            continue
        value, reasons = score(root, row)
        candidates.append({
            "name": name,
            "status": row.get("status", ""),
            "size_mb": round(int(row.get("size_bytes") or 0) / (1024 * 1024), 1),
            "export": has_export(root, name),
            "score": value,
            "reasons": reasons,
        })

    candidates.sort(key=lambda item: (-item["score"], item["size_mb"], item["name"].lower()))
    selected = candidates[:args.limit]

    if args.json:
        print(json.dumps(selected, ensure_ascii=False, indent=2))
    else:
        for item in selected:
            print(item["name"])
    return 0


if __name__ == "__main__":
    sys.exit(main())
