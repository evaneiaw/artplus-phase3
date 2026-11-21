#!/usr/bin/env python3
"""Painel de status da extracao deterministica do ArtPlus.

So le artefatos ja produzidos; nunca roda Ghidra nem chama a fila.

Fontes:
  dossiers/*/manifest.json          -> modulo, funcoes, estado, lacunas
  docs/phases/OVERNIGHT-*.md        -> fila planejada, inicio/fim
  logs/auto/extract-detached.log    -> progresso real ([i/N], relatorio:)

Saidas:
  logs/auto/status.json             (--json)
  docs/status/dashboard.html        (--html, dark, refresh 3s)

Exemplos:
  python scripts/status.py --text
  python scripts/status.py --json
  python scripts/status.py --html
  python scripts/status.py --watch 2
  python scripts/status.py --watch 2 --text
"""
from __future__ import annotations

import argparse
import html
import json
import os
import sys
import time
from datetime import datetime
from pathlib import Path

BAR_FULL = "\u2588"      # bloco cheio
BAR_EMPTY = "\u2591"     # bloco claro
BAR_WIDTH = 28

POSIX_HINT = "python scripts/status.py --text"


# --------------------------------------------------------------------------- #
# console
# --------------------------------------------------------------------------- #
def setup_console() -> bool:
    """UTF-8 no stdout + code page 65001 quando existe console.

    Retorna True se a barra Unicode pode ser impressa.
    """
    try:
        import ctypes

        ctypes.windll.kernel32.SetConsoleOutputCP(65001)
    except Exception:
        pass
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8", errors="replace")
        except Exception:
            pass
    try:
        BAR_FULL.encode(sys.stdout.encoding or "ascii")
        return True
    except Exception:
        return False


# --------------------------------------------------------------------------- #
# coleta
# --------------------------------------------------------------------------- #
def rel(root: Path, path: Path) -> str:
    try:
        return path.resolve().relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()


def load_dossiers(root: Path) -> list[dict]:
    items: list[dict] = []
    for manifest in sorted((root / "dossiers").glob("*/manifest.json")):
        try:
            data = json.loads(manifest.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            continue
        if not isinstance(data, dict):
            continue
        identity = data.get("identity") or {}
        stem = str(data.get("stem") or manifest.parent.name)
        dossier = manifest.parent / "DOSSIER.md"
        try:
            size = int(identity.get("size_bytes") or 0)
        except (TypeError, ValueError):
            size = 0
        try:
            functions = int(data.get("function_count") or 0)
        except (TypeError, ValueError):
            functions = 0
        items.append({
            "module": str(data.get("module") or stem),
            "stem": stem,
            "function_count": functions,
            "size_bytes": size,
            "sha256": str(identity.get("sha256") or ""),
            "status": str(identity.get("status") or "sem estado"),
            "kind": str(identity.get("kind") or ""),
            "verdict": str(data.get("verdict") or ""),
            "gap_count": len(data.get("gaps") or []),
            "feature_count": len(data.get("features") or []),
            "evidence_static": len((data.get("evidence") or {}).get("static") or []),
            "evidence_behavioral": len((data.get("evidence") or {}).get("behavioral") or []),
            "export_count": len(data.get("exports") or []),
            "blockers": len(data.get("blockers") or []),
            "generated": str(data.get("generated") or ""),
            "dossier": dossier.is_file(),
            "dossier_rel": rel(root, dossier),
        })
    items.sort(key=lambda item: (-item["function_count"], item["module"].lower()))
    return items


def load_report(root: Path) -> dict:
    info = {"name": "", "path": "", "started": "", "finished": "",
            "modules": [], "ghidra": "", "exists": False}
    phases = root / "docs" / "phases"
    if not phases.is_dir():
        return info
    files = list(phases.glob("OVERNIGHT-*.md"))
    if not files:
        return info
    path = sorted(files, key=lambda p: (p.name, p.stat().st_mtime))[-1]
    info.update({"exists": True, "name": path.name, "path": rel(root, path)})
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return info
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("- Inicio:"):
            info["started"] = stripped.split(":", 1)[1].strip()
        elif stripped.startswith("- Ghidra:"):
            info["ghidra"] = stripped.split(":", 1)[1].strip()
        elif stripped.startswith("Fim:"):
            info["finished"] = stripped.split(":", 1)[1].strip()
        elif stripped.startswith("|") and "---" not in stripped:
            cells = [cell.strip() for cell in stripped.strip("|").split("|")]
            if len(cells) < 4 or cells[0].lower() in ("modulo", "módulo"):
                continue
            module = cells[0]
            link = ""
            if "](" in cells[3]:
                link = cells[3].split("](", 1)[1].rstrip(")")
            info["modules"].append({
                "module": module,
                "sha": cells[1].strip("`"),
                "ghidra": cells[2],
                "dossier": link,
            })
    return info


def load_log(root: Path) -> dict:
    info = {"path": "logs/auto/extract-detached.log", "exists": False,
            "total": 0, "done": 0, "current": "", "finished": False,
            "result_lines": 0, "tail": ""}
    path = root / "logs" / "auto" / "extract-detached.log"
    if not path.is_file():
        return info
    info["exists"] = True
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return info
    total = 0
    last_index = 0
    last_module = ""
    results = 0
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("[") and "]" in stripped:
            head = stripped[1:stripped.index("]")]
            if "/" in head:
                index_part, total_part = head.split("/", 1)
                try:
                    last_index = int(index_part)
                    total = int(total_part)
                except ValueError:
                    pass
                last_module = stripped[stripped.index("]") + 1:].strip()
        elif stripped.startswith("| ") and "`" in stripped:
            results += 1
        elif stripped.startswith("relatorio:"):
            info["finished"] = True
        elif stripped == "orcamento de tempo atingido":
            info["finished"] = True
    info.update({
        "total": total,
        "done": min(results, total) if total else results,
        "result_lines": results,
        "current": last_module if results < last_index else "",
        "last_index": last_index,
        "tail": text.strip().splitlines()[-1] if text.strip() else "",
    })
    return info


def build_status(root: Path) -> dict:
    dossiers = load_dossiers(root)
    report = load_report(root)
    log = load_log(root)
    total_functions = sum(item["function_count"] for item in dossiers)
    with_dossier = sum(1 for item in dossiers if item["dossier"])
    with_exports = sum(1 for item in dossiers if item["export_count"])
    blockers = sum(item["blockers"] for item in dossiers)

    total = log["total"] or len(report["modules"]) or len(dossiers)
    done = log["done"] if log["total"] else len(report["modules"])
    done = max(0, min(done, total)) if total else 0

    if log["current"]:
        phase = f"processando {log['current']}"
    elif log["finished"] or (total and done >= total):
        phase = "concluido"
    elif total:
        phase = "aguardando"
    else:
        phase = "sem fila"

    return {
        "generated_at": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "root": root.as_posix(),
        "totals": {
            "dossiers": len(dossiers),
            "dossiers_with_file": with_dossier,
            "functions": total_functions,
            "with_ghidra_exports": with_exports,
            "open_gaps": sum(item["gap_count"] for item in dossiers),
            "blockers": blockers,
        },
        "queue": {
            "total": total,
            "done": done,
            "current": log["current"],
            "phase": phase,
            "finished": bool(log["finished"]),
            "source": log["path"],
            "log_exists": log["exists"],
        },
        "report": report,
        "dossiers": dossiers,
    }


# --------------------------------------------------------------------------- #
# texto
# --------------------------------------------------------------------------- #
def human(number: int) -> str:
    return f"{number:,}".replace(",", ".")


def bar(done: int, total: int, width: int = BAR_WIDTH, unicode_ok: bool = True) -> str:
    if total <= 0:
        filled = 0
    else:
        filled = int(round(width * min(done, total) / total))
    full = BAR_FULL if unicode_ok else "#"
    empty = BAR_EMPTY if unicode_ok else "-"
    return full * filled + empty * (width - filled)


def render_text(status: dict, unicode_ok: bool = True) -> str:
    totals = status["totals"]
    queue = status["queue"]
    report = status["report"]
    line = "=" * 84
    out = [
        line,
        "  ArtPlus - extracao deterministica (ArtPro+ v24.03.10017)",
        f"  atualizado {status['generated_at']}",
        line,
    ]
    percent = (100.0 * queue["done"] / queue["total"]) if queue["total"] else 0.0
    out.append(f"  {bar(queue['done'], queue['total'], unicode_ok=unicode_ok)}"
               f"  {queue['done']}/{queue['total']}  ({percent:5.1f}%)")
    out.append(f"  fase: {queue['phase']}")
    out.append("")
    out.append(f"  dossies: {totals['dossiers']:>3}   "
               f"funcoes: {human(totals['functions']):>12}   "
               f"com export Ghidra: {totals['with_ghidra_exports']:>3}   "
               f"lacunas: {totals['open_gaps']:>3}   "
               f"bloqueios: {totals['blockers']}")
    relatorio = report["name"] or "nenhum"
    out.append(f"  relatorio: {relatorio}"
               f"   inicio: {report['started'] or '-'}   fim: {report['finished'] or '-'}")
    out.append("")

    header = f"  {'MODULO':<28}{'FUNCOES':>10}  {'ESTADO':<26}{'DOSSIER':<8}"
    out.append(header)
    out.append("  " + "-" * 80)
    for item in status["dossiers"]:
        estado = item["status"]
        if item["export_count"]:
            estado += " +ghidra"
        if item["feature_count"]:
            estado += f" +{item['feature_count']}F"
        out.append(f"  {item['module'][:27]:<28}{human(item['function_count']):>10}  "
                   f"{estado[:25]:<26}{'ok' if item['dossier'] else 'falta':<8}")
    out.append("  " + "-" * 80)
    out.append(f"  {len(status['dossiers'])} modulos listados | "
               f"{totals['dossiers_with_file']} dossies no disco")
    out.append(line)
    return "\n".join(out)


# --------------------------------------------------------------------------- #
# JSON
# --------------------------------------------------------------------------- #
def _atomic_write(path: Path, text: str) -> None:
    """Escrita atomica: leitores (dashboard, watcher) nunca veem arquivo parcial."""
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + ".tmp")
    tmp.write_text(text, encoding="utf-8")
    os.replace(tmp, path)


def write_json(status: dict, root: Path) -> Path:
    path = root / "logs" / "auto" / "status.json"
    _atomic_write(path, json.dumps(status, ensure_ascii=False, indent=2) + "\n")
    return path


# --------------------------------------------------------------------------- #
# HTML
# --------------------------------------------------------------------------- #
def render_html(status: dict) -> str:
    totals = status["totals"]
    queue = status["queue"]
    report = status["report"]
    percent = (100.0 * queue["done"] / queue["total"]) if queue["total"] else 0.0
    filled = int(round(100 * min(queue["done"], queue["total"]) / queue["total"])) if queue["total"] else 0

    rows = []
    for item in status["dossiers"]:
        name = html.escape(item["module"])
        if item["dossier"]:
            target = os.path.relpath(str(Path(status["root"]) / item["dossier_rel"]),
                                     str(Path(status["root"]) / "docs" / "status"))
            target = Path(target).as_posix()
            cell = f'<a href="{html.escape(target)}">abrir</a>'
        else:
            cell = '<span class="miss">falta</span>'
        estado = item["status"]
        if item["export_count"]:
            estado += " +ghidra"
        if item["feature_count"]:
            estado += f" +{item['feature_count']}F"
        rows.append(
            "      <tr>"
            f"<td class=\"mod\">{name}</td>"
            f"<td class=\"num\">{human(item['function_count'])}</td>"
            f"<td>{html.escape(estado)}</td>"
            f"<td class=\"num\">{item['gap_count']}</td>"
            f"<td>{cell}</td>"
            "</tr>"
        )
    table = "\n".join(rows) if rows else '      <tr><td colspan="5">sem dossies</td></tr>'

    report_link = (f'<a href="../phases/{html.escape(report["name"])}">'
                   f'{html.escape(report["name"])}</a>') if report["exists"] else "nenhum"

    return f"""<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta http-equiv="refresh" content="3">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ArtPlus - status da extracao</title>
<style>
  :root {{ --bg:#0d1117; --card:#161b22; --line:#30363d; --fg:#e6edf3;
           --dim:#8b949e; --acc:#58a6ff; --ok:#3fb950; --warn:#d29922; }}
  * {{ box-sizing:border-box; }}
  body {{ margin:0; padding:28px; background:var(--bg); color:var(--fg);
          font:14px/1.5 "Cascadia Mono", Consolas, ui-monospace, monospace; }}
  h1 {{ font-size:19px; margin:0 0 4px; letter-spacing:.4px; }}
  .sub {{ color:var(--dim); font-size:12px; margin-bottom:22px; }}
  .cards {{ display:grid; grid-template-columns:repeat(auto-fit,minmax(180px,1fr));
            gap:14px; margin-bottom:22px; }}
  .card {{ background:var(--card); border:1px solid var(--line); border-radius:10px;
           padding:14px 16px; }}
  .card .k {{ color:var(--dim); font-size:11px; text-transform:uppercase;
              letter-spacing:1px; }}
  .card .v {{ font-size:26px; font-weight:600; margin-top:6px; }}
  .card .n {{ color:var(--dim); font-size:11px; margin-top:4px; }}
  .bar {{ height:12px; background:#21262d; border:1px solid var(--line);
          border-radius:7px; overflow:hidden; margin:8px 0 4px; }}
  .bar > i {{ display:block; height:100%; width:{filled}%;
              background:linear-gradient(90deg,#1f6feb,var(--ok)); }}
  .phase {{ color:var(--acc); }}
  table {{ width:100%; border-collapse:collapse; background:var(--card);
           border:1px solid var(--line); border-radius:10px; overflow:hidden; }}
  th,td {{ padding:9px 12px; border-bottom:1px solid var(--line); text-align:left; }}
  th {{ color:var(--dim); font-size:11px; text-transform:uppercase;
        letter-spacing:1px; font-weight:500; }}
  tr:last-child td {{ border-bottom:none; }}
  tr:hover td {{ background:#1c2129; }}
  td.num, th.num {{ text-align:right; font-variant-numeric:tabular-nums; }}
  td.mod {{ color:var(--fg); }}
  a {{ color:var(--acc); text-decoration:none; }}
  a:hover {{ text-decoration:underline; }}
  .miss {{ color:var(--warn); }}
  footer {{ color:var(--dim); font-size:11px; margin-top:18px; }}
</style>
</head>
<body>
  <h1>ArtPlus &mdash; extracao deterministica</h1>
  <div class="sub">ArtPro+ v24.03.10017 &middot; atualizado {html.escape(status['generated_at'])}
    &middot; auto-refresh 3s &middot; <code>{html.escape(POSIX_HINT)}</code></div>

  <div class="cards">
    <div class="card"><div class="k">Dossies</div>
      <div class="v">{totals['dossiers']}</div>
      <div class="n">{totals['dossiers_with_file']} no disco</div></div>
    <div class="card"><div class="k">Funcoes</div>
      <div class="v">{human(totals['functions'])}</div>
      <div class="n">{totals['with_ghidra_exports']} modulos com export Ghidra</div></div>
    <div class="card"><div class="k">Fila</div>
      <div class="v">{queue['done']}/{queue['total']}</div>
      <div class="n">{percent:.0f}% &middot; {html.escape(queue['phase'])}</div></div>
    <div class="card"><div class="k">Relatorio</div>
      <div class="v" style="font-size:14px">{report_link}</div>
      <div class="n">inicio {html.escape(report['started'] or '-')} &middot;
        fim {html.escape(report['finished'] or '-')} &middot;
        ghidra {html.escape(report['ghidra'] or '-')}</div></div>
  </div>

  <div class="bar"><i></i></div>
  <div class="sub">lacunas abertas {totals['open_gaps']} &middot;
    bloqueios {totals['blockers']} &middot;
    fase <span class="phase">{html.escape(queue['phase'])}</span></div>

  <table>
    <thead><tr><th>Modulo</th><th class="num">Funcoes</th><th>Estado</th>
      <th class="num">Lacunas</th><th>Dossie</th></tr></thead>
    <tbody>
{table}
    </tbody>
  </table>
  <footer>fonte: dossiers/*/manifest.json &middot;
    {html.escape(queue['source'])} &middot; gerado por scripts/status.py</footer>
</body>
</html>
"""


def write_html(status: dict, root: Path) -> Path:
    path = root / "docs" / "status" / "dashboard.html"
    _atomic_write(path, render_html(status))
    return path


# --------------------------------------------------------------------------- #
# main
# --------------------------------------------------------------------------- #
def main() -> int:
    parser = argparse.ArgumentParser(description="Painel de status da extracao")
    parser.add_argument("--root", default=str(Path(__file__).resolve().parents[1]))
    parser.add_argument("--text", action="store_true", help="painel no terminal")
    parser.add_argument("--json", action="store_true",
                        help="grava logs/auto/status.json e imprime o JSON")
    parser.add_argument("--html", action="store_true",
                        help="grava docs/status/dashboard.html")
    parser.add_argument("--watch", type=float, default=0.0, metavar="SEGUNDOS",
                        help="loop infinito: regrava JSON+HTML a cada N s")
    args = parser.parse_args()
    root = Path(args.root).resolve()
    unicode_ok = setup_console()

    if args.watch and args.watch <= 0:
        parser.error("--watch exige N > 0")
    if not (args.text or args.json or args.html or args.watch):
        args.text = True

    cycle = 0
    while True:
        cycle += 1
        status = build_status(root)
        if args.json or args.watch:
            path = write_json(status, root)
            if args.json:
                print(path.read_text(encoding="utf-8"), flush=True)
        if args.html or args.watch:
            path = write_html(status, root)
            if args.html and not args.watch:
                print(f"dashboard: {rel(root, path)}", flush=True)
        if args.text:
            if args.watch:
                print("\033[2J\033[H", end="")
            print(render_text(status, unicode_ok=unicode_ok), flush=True)
        if not args.watch:
            break
        try:
            time.sleep(args.watch)
        except KeyboardInterrupt:
            break
    return 0


if __name__ == "__main__":
    sys.exit(main())
