# Receitas gh / git

## Inspeção antes do commit

```powershell
git -C H:\Desktop\ArtPlus status --short --branch
git -C H:\Desktop\ArtPlus diff --stat
git -C H:\Desktop\ArtPlus diff --cached --stat
git -C H:\Desktop\ArtPlus log --oneline -5
```

## Commit seletivo

```powershell
git -C H:\Desktop\ArtPlus add .opencode/skills AGENTS.md
git -C H:\Desktop\ArtPlus commit -m "docs(skills): cria suite de skills dos 9 agentes" -m "26 skills com frontmatter, references e mapeamento por agente."
```

Nunca use `git add -A` com árvore suja de outra sessão.

## Branch e PR

```powershell
git -C H:\Desktop\ArtPlus switch -c feature/F-025-trim-marks
git -C H:\Desktop\ArtPlus push -u origin feature/F-025-trim-marks
gh pr create --repo evaneiaw/artplus-phase3 --base master --fill
gh pr checks --watch
gh pr view --web
```

## Release

```powershell
gh release create v0.1.0 --repo evaneiaw/artplus-phase3 --generate-notes --title "ArtPlus v0.1.0"
gh release list --repo evaneiaw/artplus-phase3
```

## Recuperação

| Situação | Comando |
|---|---|
| Desfazer stage | `git restore --staged <arquivo>` |
| Descartar alteração local | `git restore <arquivo>` |
| Ver último commit | `git show --stat HEAD` |
| Reverter commit publicado | `git revert <sha>` |
| Atualizar branch | `git fetch origin; git rebase origin/master` |
| Remover branch local | `git branch -d <branch>` |

Nunca `git reset --hard` com trabalho não commitado de outra sessão. Nunca `push --force` em `master`.

## Diagnóstico de artefato grande

```powershell
git -C H:\Desktop\ArtPlus ls-files -s | Sort-Object { [int]($_ -split ' ')[3] } -Descending | Select-Object -First 10
```

Se um binário entrou no histórico, não basta apagar no commit seguinte: registre e planeje limpeza com `git filter-repo` fora do horário de trabalho.
