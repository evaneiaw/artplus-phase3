#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_version.py — verificação de consistência da versão de referência do ArtPlus.

Fonte única da verdade: docs/agents/REFERENCE_VERSION.md
    Produto  : Esko ArtPro+
    Versão   : 24.03.10017  (pasta C:\\Esko\\bg_prog_artpro+_v240)

Regra absoluta: nenhum documento, agente, comando, skill, script ou evidência ATIVA
pode usar outra versão como referência do ArtPlus.

Este script:
  - varre os arquivos ativos em busca de referências PROIBIDAS (v25 / v250) e
    referências DIVERGENTES (24.0 / 24.03, shorthand sem o build .10017);
  - varre os arquivos HISTÓRICOS e apenas informa, sem reprovar;
  - imprime caminho:linha para cada ocorrência;
  - sai com código 1 se houver contaminação em arquivo ativo.

Este arquivo NÃO é varrido (está em SELF_SKIP) para que o verificador não gere
falso positivo por conter os padrões que procura. Os padrões são construídos por
concatenação para evitar tokens literais proibidos no próprio código.

Uso:
    python scripts/check_version.py
    python scripts/check_version.py --strict   (falha também em divergente)
"""

import os
import re
import sys
import fnmatch

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

CANONICAL = "24.03.10017"
CANONICAL_DIR = "bg_prog_artpro+_v240"

def _pat(*parts):
    return re.compile("".join(parts))

FORBIDDEN = [
    _pat("_v", "250"),
    _pat(r"\bv", "250", r"\b"),
    _pat(r"\bv", "25", r"\b"),
    _pat(r"versao_ref:", r"\s*[\"']?", "25"),
    _pat(r"\bversao:", r"\s*[\"']?", "25", r"\.", "0"),
    _pat(r"[Vv]ers", "[ãa]", r"o:\s*", "25", r"\.", "0"),
    _pat(r"ArtPro\+", r"\s*v?", "25", r"\b"),
    _pat(r"ArtProPlus_", "25", r"\.", "03"),
    _pat(r"25", r"\.", "03", r"\.", "x"),
]

DIVERGENT = [
    _pat(r"versao_ref:", r"\s*[\"']?", "24", r"\.", "0", r"(?![\d.])"),
    _pat(r"versao_ref:", r"\s*[\"']?", "24", r"\.", "03", r"(?!\.)"),
    _pat(r"\bversao:", r"\s*[\"']?", "24", r"\.", "0", r"(?![\d.])"),
    _pat(r"\bversao:", r"\s*[\"']?", "24", r"\.", "03", r"(?!\.)"),
]

ACTIVE_GLOBS = [
    "AGENTS.md",
    "opencode.json",
    "opencode.jsonc",
    "ROADMAP.md",
    "FEATURE_MATRIX.csv",
    ".opencode/**",
    "docs/agents/**",
    "docs/features/**",
    "docs/decisions/**",
    "docs/architecture/**",
    "docs/environment/**",
    "docs/ROADMAP.md",
    "scripts/**",
]

HISTORICAL_GLOBS = [
    "docs/reverse-engineering/**",
    "rag/**",
    "docs/audit/**",
    "docs/EXECUTION_LOG.md",
    "docs/status/**",
    "logs/**",
    "docs/behavior/**",
]

# Estes arquivos documentam a própria regra de versão e podem citar v25/v250
# como exemplo do que é PROIBIDO; o verificador deve ignorá-los.
META_DOCS = [
    "docs/agents/REFERENCE_VERSION.md",
    "docs/agents/CHECKPOINT.md",
    "docs/agents/CONFLITOS.md",
    "docs/agents/BLOQUEIOS.md",
]

SKIP_PREFIXES = [
    os.path.join(REPO, ".git"),
    os.path.join(REPO, "build"),
    os.path.join(REPO, "node_modules"),
    os.path.join(REPO, ".opencode", "node_modules"),
    os.path.join(REPO, "tools", "ghidra_projects"),
    os.path.join(REPO, "tools", "ghidra_12.2_PUBLIC"),
    os.path.join(REPO, "dossiers"),
    os.path.join(REPO, "Testing"),
    os.path.join(REPO, "ghidra_project"),
    os.path.join(REPO, "ghidra_project2"),
    os.path.join(REPO, ".scratchpad"),
]

SELF = os.path.abspath(__file__)

TEXT_EXTS = {".md", ".json", ".jsonc", ".jsonl", ".csv", ".py", ".txt", ".html",
             ".yaml", ".yml", ".ps1", ".cmake", ".cpp", ".h", ".hpp", ".ini"}


def classify(abspath):
    rel = os.path.relpath(abspath, REPO).replace("\\", "/")
    if rel in META_DOCS:
        return None
    for g in ACTIVE_GLOBS:
        if fnmatch.fnmatch(rel, g):
            return "active"
    for g in HISTORICAL_GLOBS:
        if fnmatch.fnmatch(rel, g):
            return "historical"
    return None


def iter_files():
    for root, dirs, files in os.walk(REPO):
        dirs[:] = [d for d in dirs if d not in (".git", "build", "node_modules",
                                                "__pycache__")]
        for name in files:
            ap = os.path.join(root, name)
            if ap == SELF:
                continue
            if any(ap.startswith(p) for p in SKIP_PREFIXES):
                continue
            if os.path.splitext(name)[1].lower() not in TEXT_EXTS:
                continue
            yield ap


def scan():
    errors = []
    warnings = []
    historical = []
    for ap in iter_files():
        cat = classify(ap)
        if cat is None:
            continue
        try:
            with open(ap, "r", encoding="utf-8", errors="ignore") as f:
                lines = f.readlines()
        except OSError:
            continue
        rel = os.path.relpath(ap, REPO)
        for idx, line in enumerate(lines, 1):
            low = line.lower()
            if ("histor" in low) or ("histór" in low) or ("fora do fluxo ativo" in low):
                continue
            for rx in FORBIDDEN:
                if rx.search(line):
                    entry = (rel, idx, line.strip())
                    if cat == "active":
                        errors.append(entry)
                    else:
                        historical.append(entry)
                    break
            for rx in DIVERGENT:
                if rx.search(line):
                    warnings.append((rel, idx, line.strip()))
                    break
    return errors, warnings, historical


def main():
    strict = "--strict" in sys.argv
    errors, warnings, historical = scan()

    print("=" * 72)
    print("check_version.py — consistência da versão de referência")
    print("Canônico:", CANONICAL, "| Pasta:", CANONICAL_DIR)
    print("=" * 72)

    if errors:
        print(f"\n[ERRO] {len(errors)} referência(s) PROIBIDA(s) em arquivos ATIVOS:")
        for rel, ln, txt in errors:
            print(f"  {rel}:{ln}  {txt}")
    else:
        print("\n[OK] Nenhuma referência proibida (v25/v250) em arquivos ativos.")

    if warnings:
        print(f"\n[AVISO] {len(warnings)} referência(s) DIVERGENTE(s) (shorthand "
              f"24.0/24.03) — normalizar para {CANONICAL}:")
        for rel, ln, txt in warnings:
            print(f"  {rel}:{ln}  {txt}")

    if historical:
        print(f"\n[INFO] {len(historical)} ocorrência(s) em arquivos HISTÓRICOS "
              "(fora do fluxo ativo — preservados para rastreabilidade):")
        seen = set()
        for rel, ln, txt in historical:
            if rel not in seen:
                print(f"  {rel}:{ln}  {txt}")
                seen.add(rel)

    print("\n" + "-" * 72)
    fail = len(errors) > 0 or (strict and len(warnings) > 0)
    if fail:
        print("RESULTADO: REPROVADO")
        return 1
    print("RESULTADO: APROVADO")
    return 0


if __name__ == "__main__":
    sys.exit(main())
