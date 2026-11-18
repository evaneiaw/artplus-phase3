---
name: Qt C++ Development
description: Implementa o ArtPlus em C++17 e Qt 6.8.3 seguindo a arquitetura existente — layout src/<área>, CMake Kitware, MinGW POSIX MSVCRT, sinais/slots, ownership Qt e unidades explícitas. Use ao escrever código, ajustar CMakeLists, compilar incremental ou diagnosticar erro de toolchain.
metadata:
  version: "1"
---

# Desenvolvimento C++17 / Qt 6.8.3

## Stack real

| Item | Valor |
|---|---|
| C++ | 17 (`CMAKE_CXX_STANDARD 17`) |
| Qt | 6.8.3 `mingw_64` em `C:/Qt/6.8.3/mingw_64` |
| Compilador | WinLibs POSIX.**MSVCRT** GCC em `...\BrechtSanders.WinLibs.POSIX.MSVCRT_...\mingw64\bin\g++.exe` |
| CMake | Kitware 4.4.4 (não o do WinLibs) |
| Preset | `mingw-msvcrt` → `release-mingw` |
| Testes | Qt Test + CTest (`--preset unit-tests`) |

CMake absoluto:

```powershell
$cmake = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe\cmake-4.4.4-windows-x86_64\bin\cmake.exe'
& $cmake --preset mingw-msvcrt
& $cmake --build --preset release-mingw
```

`cmake` no PATH é o WinLibs **UCRT**. Misturar com o Qt MSVCRT corrompe heap e produz `undefined reference to QApplication`.

## Onde escrever

- Header e fonte no mesmo diretório de área: `src/<área>/<classe>.{h,cpp}`.
- Novo `.cpp` entra no grupo correto do `CMakeLists.txt` (`DOCUMENT_SOURCES`, `PDF_SOURCES`, `PREPRESS_SOURCES`, `COLOR_SOURCES`, `AUTOMATION_SOURCES`, `UI_SOURCES`).
- `ArtPlus` recebe todos os grupos; testes recebem apenas o que precisam.
- Não crie subprojeto nem `find_package` sem ADR.

## Convenções Qt

- `QObject` com parent para ownership de UI; `std::shared_ptr`/`unique_ptr` no domínio.
- Sinais/slots para eventos de UI; chamada direta para regra de domínio.
- `QString` na borda de UI/IO; `std::string`/`QByteArray` no parser PDF.
- `tr("...")` em texto visível; texto de log e evidência em português técnico.
- Não deixe exceção atravessar slot Qt: trate e reporte na borda.
- Prefira `const&` e `QStringView`; evite cópia de `QImage` em laço.
- `QTransform` para matriz; `QPainterPath` para geometria de desenho.
- Use `qFuzzyCompare`/tolerância explícita; nunca `==` em `double` de coordenada.

## Ciclo

1. Leia `docs/decisions/ADR-###` e `docs/features/F-###`. Confirme os arquivos que pode tocar.
2. Leia o código vizinho e siga layout, nomes e tratamento de erro.
3. Implemente em incremento pequeno e **compile a cada incremento**.
4. Se falhar por toolchain (não por código), pare e devolva: quem conserta é `testes`.
5. Entregue:

```
F-### / <módulo>
ARQUIVOS: <caminho>
BUILD: <comando> -> <sucesso/falha> + últimas linhas
FALTA: <o que ficou fora e por quê>
RISCOS: <o que pode quebrar>
```

## Proibições

- Editar `tests/`, `docs/`, `rag/`, `CMakePresets.json`, `opencode.jsonc`.
- Stub silencioso: use `TODO(F-###)`, erro explícito ou não entregue.
- Código morto, `#if 0`, `catch(...)` para "compilar".
- Campo sem unidade quando a spec tem unidade.
- Ajustar o ADR por conta própria quando ele conflita com o código: pare e reporte.

## Referências

- `references/toolchain-pitfalls.md` — UCRT×MSVCRT, `0xc0000135`, `QJsonValueRef`, paths e diagnóstico.
- `AGENTS.md` §5 — comandos canônicos.
