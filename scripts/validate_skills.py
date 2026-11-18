#!/usr/bin/env python3
"""Validate ArtPlus skills, commands and agents.

Checks:
- SKILL.md frontmatter with name + description, kebab-case ID, references exist
- command frontmatter with description + existing agent, body non-empty
- agent frontmatter with description + valid mode
- stale absolute paths

Exit code 0 when there are no errors.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ID_RE = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*$")
REF_RE = re.compile(r"`(references/[^`]+)`")
STALE = (r"C:\RE-Tools", r"C:\ghidra")
VALID_MODES = {"primary", "subagent", "all"}


def split_frontmatter(text: str):
    if not text.startswith("---"):
        return None
    end = text.find("\n---", 3)
    if end < 0:
        return None
    return text[3:end]


def fm_value(frontmatter: str, key: str):
    match = re.search(rf"^{re.escape(key)}:\s*(.+?)\s*$", frontmatter, re.MULTILINE)
    return match.group(1).strip().strip('"').strip("'") if match else None


def load_text(path: Path, errors: list[str], label: str):
    try:
        return path.read_text(encoding="utf-8", errors="strict")
    except UnicodeDecodeError as exc:
        errors.append(f"{label}: UTF-8 invalido ({exc})")
    except OSError as exc:
        errors.append(f"{label}: nao foi possivel ler ({exc})")
    return None


def check_skills(root: Path, errors: list[str]):
    skills_dir = root / ".opencode" / "skills"
    if not skills_dir.is_dir():
        errors.append("skills: diretorio .opencode/skills ausente")
        return []
    rows = []
    for folder in sorted(p for p in skills_dir.iterdir() if p.is_dir()):
        skill_file = folder / "SKILL.md"
        label = f"skill {folder.name}"
        if not skill_file.is_file():
            errors.append(f"{label}: sem SKILL.md")
            continue
        text = load_text(skill_file, errors, label)
        if text is None:
            continue
        frontmatter = split_frontmatter(text)
        if frontmatter is None:
            errors.append(f"{label}: frontmatter ausente ou nao fechado")
            continue
        name = fm_value(frontmatter, "name")
        description = fm_value(frontmatter, "description")
        if not name:
            errors.append(f"{label}: falta name")
        if not description or len(description) < 40:
            errors.append(f"{label}: description ausente ou curta")
        if not ID_RE.match(folder.name):
            errors.append(f"{label}: ID fora do padrao kebab-case")
        refs = sorted(set(REF_RE.findall(text)))
        missing = [
            ref for ref in refs
            if not (folder / ref.replace("/", "/")).is_file()
        ]
        for ref in missing:
            errors.append(f"{label}: reference ausente {ref}")
        for stale in STALE:
            if stale in text:
                errors.append(f"{label}: caminho obsoleto {stale}")
        rows.append((folder.name, len(text.splitlines()), len(refs)))
    return rows


def check_commands(root: Path, agent_ids: set[str], errors: list[str], warnings: list[str]):
    commands_dir = root / ".opencode" / "commands"
    if not commands_dir.is_dir():
        warnings.append("commands: .opencode/commands ausente")
        return []
    rows = []
    for command_file in sorted(commands_dir.glob("*.md")):
        label = f"command /{command_file.stem}"
        text = load_text(command_file, errors, label)
        if text is None:
            continue
        frontmatter = split_frontmatter(text)
        if frontmatter is None:
            errors.append(f"{label}: frontmatter ausente")
            continue
        description = fm_value(frontmatter, "description")
        agent = fm_value(frontmatter, "agent")
        if not description:
            errors.append(f"{label}: falta description")
        if agent and agent not in agent_ids:
            errors.append(f"{label}: agent '{agent}' nao existe em .opencode/agents")
        if not agent:
            warnings.append(f"{label}: sem agent (roda no agente atual)")
        body = text.split("\n---", 3)[-1].strip() if "\n---" in text else ""
        if not body:
            errors.append(f"{label}: corpo vazio")
        if "$ARGUMENTS" not in body:
            warnings.append(f"{label}: sem $ARGUMENTS")
        rows.append((command_file.stem, agent or "-", len(body.splitlines())))
    return rows


def check_agents(root: Path, errors: list[str], warnings: list[str]):
    agents_dir = root / ".opencode" / "agents"
    if not agents_dir.is_dir():
        warnings.append("agents: .opencode/agents ausente")
        return set(), []
    ids = set()
    rows = []
    for agent_file in sorted(agents_dir.glob("*.md")):
        label = f"agent {agent_file.stem}"
        text = load_text(agent_file, errors, label)
        if text is None:
            continue
        frontmatter = split_frontmatter(text)
        if frontmatter is None:
            errors.append(f"{label}: frontmatter ausente")
            continue
        description = fm_value(frontmatter, "description")
        mode = fm_value(frontmatter, "mode")
        if not description:
            errors.append(f"{label}: falta description")
        if mode not in VALID_MODES:
            errors.append(f"{label}: mode invalido '{mode}'")
        if not ID_RE.match(agent_file.stem):
            errors.append(f"{label}: ID fora do padrao kebab-case")
        ids.add(agent_file.stem)
        rows.append((agent_file.stem, mode or "-"))
    return ids, rows


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate ArtPlus skills/commands/agents")
    parser.add_argument("--root", default=str(Path(__file__).resolve().parents[1]),
                        help="project root (default: parent of scripts/)")
    args = parser.parse_args()
    root = Path(args.root).resolve()

    errors: list[str] = []
    warnings: list[str] = []

    agent_ids, agents = check_agents(root, errors, warnings)
    skills = check_skills(root, errors)
    commands = check_commands(root, agent_ids, errors, warnings)

    print(f"root: {root}")
    print(f"agents: {len(agents)} | skills: {len(skills)} | commands: {len(commands)}")
    if skills:
        print("\nSKILLS")
        for name, lines, refs in skills:
            print(f"  {name:32} {lines:>4} linhas  {refs:>2} refs")
    if commands:
        print("\nCOMMANDS")
        for name, agent, lines in commands:
            print(f"  /{name:31} {agent:18} {lines:>3} linhas")
    if warnings:
        print("\nAVISOS")
        for warning in warnings:
            print(f"  - {warning}")
    print(f"\nERROS: {len(errors)}")
    for error in errors:
        print(f"  - {error}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
