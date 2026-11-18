---
name: Evidence Verification
description: Audita se uma funcionalidade do ArtPlus realmente existe e é provável — spec completa, evidência com hash, código correspondente, build limpo, teste que cobre o F-### e comportamento igual ao original. Use antes de aprovar entrega, ao revisar manifest ou ao reauditar após correção.
metadata:
  version: "1"
---

# Verificação de evidência e auditoria

Pergunta única: **isto funciona de verdade e pode ser provado por quem não participou?**

## Checklist (reprova se faltar)

1. **Spec completa** — `docs/features/F-###.md` com entrada, UI, regras, exceções, saída, rastreabilidade e `status` coerente.
2. **Evidência real** — `rag/evidence/F-###/manifest.json`: `sha256`, `versao_ref`, `binario`, `estatico`, `comportamental`. Arquivo citado que não existe = reprovação automática.
3. **Código corresponde à spec** — leia `src/**` e confira cada regra numerada; regra sem implementação é lacuna.
4. **Sem cópia quebrada** — trecho colado do binário que quebra build/manutenção é achado; registre.
5. **Build limpo** — apague `build/`, configure e compile você mesmo.
6. **Teste roda e cobre** — `ctest --preset unit-tests`, saída anexada; teste que não toca o `F-###` não conta.
7. **Comportamento bate** — compare com `rag/evidence/F-###/behavior/` (valores, geometria, PDF). Divergência não explicada reprova.
8. **Sem stub disfarçado** — `TODO` em caminho principal, retorno vazio, `catch(...)`, botão inerte.

## Verificações objetivas

```powershell
$ev = 'H:\Desktop\ArtPlus\rag\evidence\F-025'
Get-Content "$ev\manifest.json" -Encoding UTF8
$bin = (Get-Content "$ev\manifest.json" -Encoding UTF8 | ConvertFrom-Json).binario
Get-FileHash $bin -Algorithm SHA256
Get-ChildItem $ev -Recurse | Select-Object FullName,Length
```

Hash do manifest tem que bater com o binário. Caminho que não existe = falha.

```powershell
$cmake = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe\cmake-4.4.4-windows-x86_64\bin\cmake.exe'
Remove-Item -Recurse -Force H:\Desktop\ArtPlus\build -ErrorAction SilentlyContinue
& $cmake -S H:\Desktop\ArtPlus -B H:\Desktop\ArtPlus\build --preset mingw-msvcrt
& $cmake --build H:\Desktop\ArtPlus\build --preset release-mingw
& $cmake -E chdir H:\Desktop\ArtPlus ctest --preset unit-tests
```

## Rastreabilidade

Monte a cadeia e aponte onde quebra:

```
F-025 → GHIDRA-DocumentMarks-0x18001A2C0 → src/prepress/marks.cpp:142 → tests/unit/test_editing.cpp:88 → ctest EditingTests
```

Cada elo precisa existir. Elo ausente é lacuna; elo falso é reprovação.

## Relatório (`docs/audit/F-###-audit.md`)

```markdown
# Auditoria F-### — <nome>
Data: <AAAA-MM-DD> · Auditor: auditor
Veredito: APROVADO | REPROVADO | APROVADO COM RESSALVAS

## Evidência verificada
| Item | Esperado | Encontrado | OK |

## Build
- Comando: ... · Resultado: ...

## Testes
- Comando: ... · Passaram: n/m
- Cobertura do F-###: <testes>

## Divergências
1. <arquivo:linha> — <spec> × <código>

## O que falta para aprovar
- [ ] ...
```

## Regras

- Não edite nada fora de `docs/audit/`. Nem para "corrigir um errinho".
- Não confie em relatório de outro agente: rode o comando e abra o arquivo.
- Aprovação exige comando reproduzível + saída colada. Sem isso, REPROVADO.
- Seja específico: `src/prepress/marks.cpp:142`, não "o módulo de marcas".
- Reauditoria gera `F-###-audit-2.md` e cita o que mudou.
- Não lance subagentes.

## Referências

- `references/audit-checklist.md` — checklist detalhado, red flags e modelo de laudo.
- `AGENTS.md` §4 — formato de evidência e manifest.
