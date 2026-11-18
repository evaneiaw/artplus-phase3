#!/usr/bin/env python3
"""Keyword retrieval over the ArtPlus RAG index and evidence.

No embeddings, no external service: searches the project's own files and
returns path:line plus a snippet. Designed for agents and humans.

Examples:
  python scripts/rag_query.py "trim marks"
  python scripts/rag_query.py "document marks" --limit 20 --json
  python scripts/rag_query.py "0x18001A2C0" --only evidence
"""
from __future__ import annotations

import argparse
import json
import re
import sys
import unicodedata
from pathlib import Path

try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass

STOPWORDS = {
    "a", "o", "as", "os", "de", "da", "do", "das", "dos", "e", "em", "no", "na",
    "nos", "nas", "para", "por", "com", "sem", "um", "uma", "que", "se", "ao",
    "the", "a", "an", "of", "to", "in", "on", "for", "with", "and", "or", "is",
}
TEXT_SUFFIXES = {".md", ".json", ".jsonl", ".csv", ".txt"}
MAX_FILE_BYTES = 2 * 1024 * 1024
SNIPPET = 240


def normalize(value: str) -> str:
    value = unicodedata.normalize("NFKD", value.lower())
    return "".join(ch for ch in value if not unicodedata.combining(ch))


def tokenize(value: str) -> list[str]:
    return [t for t in re.split(r"[^a-z0-9_]+", normalize(value)) if t and t not in STOPWORDS]


def collect_files(root: Path, only: str):
    groups = {
        "index": [root / "rag" / "index"],
        "evidence": [root / "rag" / "evidence"],
        "ghidra": [root / "rag" / "ghidra"],
        "docs": [root / "docs" / "features", root / "docs" / "behavior",
                 root / "docs" / "reverse-engineering", root / "docs" / "audit",
                 root / "docs" / "architecture", root / "docs" / "decisions"],
    }
    selected = groups if only == "all" else {only: groups.get(only, [])}
    for name, dirs in selected.items():
        for base in dirs:
            if not base.is_dir():
                continue
            for path in sorted(base.rglob("*")):
                if not path.is_file() or path.suffix.lower() not in TEXT_SUFFIXES:
                    continue
                try:
                    if path.stat().st_size > MAX_FILE_BYTES:
                        continue
                except OSError:
                    continue
                yield name, path


def read_lines(path: Path):
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return []
    return text.splitlines()


def main() -> int:
    parser = argparse.ArgumentParser(description="Keyword search over ArtPlus RAG/evidence")
    parser.add_argument("query", nargs="+", help="terms to search")
    parser.add_argument("--root", default=str(Path(__file__).resolve().parents[1]))
    parser.add_argument("--limit", type=int, default=12)
    parser.add_argument("--only", default="all",
                        choices=["all", "index", "evidence", "ghidra", "docs"])
    parser.add_argument("--json", action="store_true", help="machine-readable output")
    parser.add_argument("--context", type=int, default=SNIPPET)
    args = parser.parse_args()

    root = Path(args.root).resolve()
    terms = tokenize(" ".join(args.query))
    if not terms:
        print("query sem termos uteis", file=sys.stderr)
        return 2

    matches = []
    for group, path in collect_files(root, args.only):
        rel = path.relative_to(root).as_posix()
        filename = normalize(path.name)
        name_hits = sum(2 for term in terms if term in filename)
        for number, line in enumerate(read_lines(path), start=1):
            normalized = normalize(line)
            score = name_hits
            for term in terms:
                count = normalized.count(term)
                if count:
                    score += count * (3 if term in normalized[:120] else 1)
            if score:
                snippet = line.strip()
                if len(snippet) > args.context:
                    position = max(0, normalized.find(terms[0]) - 40)
                    snippet = snippet[position:position + args.context]
                matches.append({
                    "group": group,
                    "path": rel,
                    "line": number,
                    "score": score,
                    "snippet": snippet,
                })

    matches.sort(key=lambda item: (-item["score"], item["path"], item["line"]))
    matches = matches[: args.limit]

    if args.json:
        print(json.dumps({"terms": terms, "count": len(matches), "matches": matches},
                         ensure_ascii=False, indent=2))
        return 0

    if not matches:
        print(f"nenhum resultado para: {' '.join(terms)}")
        return 0

    print(f"termos: {' '.join(terms)} | resultados: {len(matches)}")
    current = None
    for match in matches:
        if match["path"] != current:
            current = match["path"]
            print(f"\n{match['path']}")
        print(f"  {match['line']:>5}: {match['snippet']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
