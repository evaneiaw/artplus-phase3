---
name: Build and Release
description: Compila e empacota o ArtPlus em Release com CMake Kitware, MinGW POSIX MSVCRT e Qt 6.8.3 — build doctor, diagnóstico de 0xc0000135, ctest, windeployqt e smoke de inicialização. Use antes de testar, ao quebrar o build, ao gerar artefato ou ao preparar release.
metadata:
  version: "1"
---

# Build e release

## Build doctor

```powershell
$cmake = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe\cmake-4.4.4-windows-x86_64\bin\cmake.exe'
Remove-Item -Recurse -Force H:\Desktop\ArtPlus\build -ErrorAction SilentlyContinue
& $cmake -S H:\Desktop\ArtPlus -B H:\Desktop\ArtPlus\build --preset mingw-msvcrt
Select-String -Path H:\Desktop\ArtPlus\build\CMakeCache.txt -Pattern 'CMAKE_CXX_COMPILER:|CMAKE_PREFIX_PATH:'
& $cmake --build H:\Desktop\ArtPlus\build --preset release-mingw
```

Checklist:

1. `CMAKE_CXX_COMPILER` = `...BrechtSanders.WinLibs.POSIX.MSVCRT...\g++.exe` (não UCRT).
2. `CMAKE_PREFIX_PATH` = `C:/Qt/6.8.3/mingw_64`.
3. `cmake` do PATH é WinLibs UCRT — use o Kitware absoluto.
4. Preset com hash do winget morre quando o pacote atualiza: conserte com descoberta dinâmica, nunca com outro hash fixo.
5. `0xc0000135` = DLL do Qt não encontrada: `PATH=C:\Qt\6.8.3\mingw_64\bin` no ambiente do teste.
6. `undefined reference to QApplication` = mistura UCRT/MSVCRT ou Qt de arquitetura errada.

## Testes

```powershell
& $cmake --build H:\Desktop\ArtPlus\build --preset release-mingw
& $cmake -E chdir H:\Desktop\ArtPlus ctest --preset unit-tests
```

Anexe a saída real. `ctest` verde com teste que não toca o `F-###` não conta.

## Empacotamento Release

```powershell
$dist = 'H:\Desktop\ArtPlus\dist\ArtPlus'
Remove-Item -Recurse -Force $dist -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $dist | Out-Null
Copy-Item H:\Desktop\ArtPlus\build\ArtPlus.exe $dist
& 'C:\Qt\6.8.3\mingw_64\bin\windeployqt.exe' --release --compiler-runtime --no-translations --dir $dist "$dist\ArtPlus.exe"
Compress-Archive -Path "$dist\*" -DestinationPath H:\Desktop\ArtPlus\dist\ArtPlus-0.1.0-win64.zip -Force
```

`dist/` não entra no git. Versão vem de `project(ArtPlus VERSION ...)` no `CMakeLists.txt`.

## Smoke de inicialização

```powershell
$p = Start-Process "$dist\ArtPlus.exe" -PassThru
Start-Sleep -Seconds 5
if ($p.HasExited) { "FALHOU: exit=$($p.ExitCode)" } else { "OK: janela abriu"; $p.CloseMainWindow() | Out-Null; Start-Sleep 1; if (-not $p.HasExited) { $p.Kill() } }
```

Registre: exit code, tempo até janela, mensagem de erro do console (se houver), DLL faltando.

## Definition of done

- [ ] Build limpo do zero, sem aviso de toolchain.
- [ ] `ctest --preset unit-tests` passando; saída colada.
- [ ] `ArtPlus.exe` abre e fecha sem crash.
- [ ] `windeployqt` copiou Qt6Core/Gui/Widgets/Qml e plugins necessários.
- [ ] Zip gerado com versão no nome.
- [ ] Nada de `build/` ou `dist/` no commit.

## Regras

- Não edite `src/` (é do agente `implementacao`).
- Não "conserte" teste mudando o esperado.
- Não publique release com auditoria pendente.

## Referências

- `references/build-doctor.md` — árvore de diagnóstico, erros comuns e correções.
- `references/release-checklist.md` — checklist completo de artefato e smoke.
