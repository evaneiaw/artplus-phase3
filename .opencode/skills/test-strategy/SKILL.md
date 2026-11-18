---
name: Test Strategy
description: Projeta e escreve testes do ArtPlus — unitários, integração, golden e regressão — com Qt Test/CTest, fixtures de PDF, tolerâncias explícitas e rastreabilidade F-###. Use ao criar teste para uma spec, escolher o tipo certo, montar fixture ou julgar se a cobertura prova a funcionalidade.
metadata:
  version: "1"
---

# Estratégia de testes

## Pirâmide do projeto

| Tipo | Onde | Prova |
|---|---|---|
| Unitário | `tests/unit/` | geometria, parsing, serialização, undo, cor |
| Integração | `tests/integration/` | import → edição → export ponta a ponta |
| Golden | `tests/golden/` (quando existir) | saída igual à referência, com tolerância |
| Visual | `tools/visual_compare/` | render vs. ArtPro+ real |
| Regressão | `tests/regression/` | bug corrigido não volta |

Um teste que não falha quando a regra é removida não prova nada.

## Regras de design

- Nome do teste e comentário de topo citam `F-###`.
- Um teste = uma regra da spec; não teste três regras no mesmo caso.
- Entrada e saída explícitas; sem dependência de estado global, ordem ou relógio.
- Determinismo: locale fixo, fonte fixa, seed fixo, sem `QDateTime::currentDateTime` no esperado.
- Tolerância declarada para geometria (`1e-3 mm`), cor (DeltaE) e pixel (skill visual).
- Golden nasce de captura do `comportamento` em `rag/evidence/F-###/behavior/`; nunca invente o esperado.
- Fixture de PDF nova entra em `tests/fixtures/` com origem e hash registrados.
- Teste de exceção verifica a mensagem/tipo, não só "não crashou".

## Como rodar

```powershell
$cmake = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe\cmake-4.4.4-windows-x86_64\bin\cmake.exe'
& $cmake --build H:\Desktop\ArtPlus\build --preset release-mingw
& $cmake -E chdir H:\Desktop\ArtPlus ctest --preset unit-tests
```

Para um teste específico:

```powershell
& $cmake -E chdir H:\Desktop\ArtPlus ctest --preset unit-tests -R PdfRoundTrip --output-on-failure
```

Se `0xc0000135`, exporte `PATH=C:\Qt\6.8.3\mingw_64\bin` antes do `ctest`.

## Cobertura útil

Não persiga linha executada. Persiga **regra da spec exercitada**:

| Regra da spec | Teste | Prova |
|---|---|---|
| offset 0 / 3 / negativo | `test_trim_marks_offset` | valor e geometria |
| entrada vazia | `test_trim_marks_empty` | erro esperado |
| unidade pt | `test_trim_marks_pt` | conversão |

Se uma regra não tem teste, ela está descoberta — reporte na entrega.

## Regras

- Nunca edite `src/` para o teste passar; relate `arquivo:linha`.
- Nunca altere o esperado sem evidência que justifique.
- Não use `skip`/`xfail` sem issue e motivo registrado.
- Não copie saída do próprio ArtPlus como golden sem comparar com o original — isso valida o bug.
- Relatório final: comando, pass/fail por teste, tempo e o que não foi coberto.

## Referências

- `references/test-design.md` — matriz de casos, template Qt Test e exemplos.
- `AGENTS.md` §5 — comandos canônicos.
