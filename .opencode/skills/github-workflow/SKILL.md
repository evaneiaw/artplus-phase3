---
name: GitHub Workflow
description: Versiona e publica o ArtPlus no repositório artplus-phase3 — branches por F-###, commits com evidência, PR via gh, tags de release e higiene de artefatos grandes. Use antes de commitar, ao abrir PR, revisar diff, publicar tag ou auditar o histórico.
metadata:
  version: "1"
---

# Git e GitHub do ArtPlus

Repositório: `H:\Desktop\ArtPlus` → `https://github.com/evaneiaw/artplus-phase3`.
Conta `gh` autenticada: `evaneiaw` (escopos `repo`, `workflow`).

## Antes de qualquer commit

```powershell
git -C H:\Desktop\ArtPlus status --short --branch
git -C H:\Desktop\ArtPlus diff --stat
git -C H:\Desktop\ArtPlus diff -- . ':(exclude)build'
```

Confirme que o commit **não** contém:

- `build/`, `*.obj`, `*.exe` gerado, `CMakeCache.txt`;
- `tools/ghidra_projects/` e `tools/ghidra_12.2_PUBLIC/` (já ignorados);
- binários de `C:\Esko` (nunca entram no repositório);
- tokens, `.env`, credenciais, `opencode.jsonc` com segredo.

O `.gitignore` atual cobre apenas `build/`, `.vs/`, `*.user`, projetos Ghidra e o Ghidra local. Se um artefato novo for grande, adicione a regra **no mesmo commit** que o introduziu.

## Branches

| Tipo | Padrão | Uso |
|---|---|---|
| Estável | `master` | Sempre compila; PR obrigatório para entrar |
| Funcionalidade | `feature/F-025-trim-marks` | Spec → ADR → código → teste |
| Análise | `analysis/documentmarks` | Evidência, triagem, Ghidra |
| Correção | `fix/ctest-path` | Build/teste, sem mudar contrato |

Nunca faça push direto em `master` com build quebrada. Nunca `push --force` em branch compartilhada.

## Commit

Formato: `<tipo>(<escopo>): <resumo no imperativo>`

Tipos: `feat`, `fix`, `docs`, `test`, `refactor`, `build`, `chore`, `revert`.
Escopo: módulo ou área (`document`, `pdf`, `marks`, `build`, `skills`).

```
feat(marks): implementa trim marks com offset em mm

F-025 / DocumentMarks.dll
Evidência: GHIDRA-DocumentMarks-0x18001A2C0, CAP-20261118-03
Teste: ctest --preset unit-tests (12/12)
```

Evite o padrão histórico `continue: roadmap execution ongoing` — não diz o que mudou nem permite auditoria.

## Pull request

```powershell
git -C H:\Desktop\ArtPlus push -u origin feature/F-025-trim-marks
gh pr create --repo evaneiaw/artplus-phase3 --base master --fill
gh pr checks --watch
```

No corpo do PR: `F-###`, evidência usada, comandos de build/teste e saída, riscos e o que ficou fora. PR sem comando reproduzível não é revisável.

## Release

```powershell
gh release create v0.1.0 --repo evaneiaw/artplus-phase3 --generate-notes --title "ArtPlus v0.1.0"
```

Tag só depois de: build limpo, `ctest` passando, smoke de inicialização e `docs/audit/` do escopo aprovado.

## Higiene de histórico

- Um commit = uma mudança coerente. Não misture skills, código e evidência no mesmo commit.
- Não reescreva histórico já publicado.
- `docs/EXECUTION_LOG.md` registra a sessão; o commit registra o diff. Não duplique um no outro.
- Antes de abrir PR, rebase em `origin/master` e resolva conflito localmente.

## Referências

- `references/gh-recipes.md` — comandos de inspeção, PR, release e recuperação.
- `AGENTS.md` §3 e §6 — donos de diretório e regras de ouro.
