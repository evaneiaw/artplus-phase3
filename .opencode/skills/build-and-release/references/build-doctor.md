# Build doctor

## Árvore de decisão

1. `cmake --version` → é o Kitware 4.4.4? Se não, use caminho absoluto.
2. `build/CMakeCache.txt`:
   - `CMAKE_CXX_COMPILER` contém `POSIX.MSVCRT`?
   - `CMAKE_PREFIX_PATH` é `C:/Qt/6.8.3/mingw_64`?
3. Falha no configure → toolchain/preset.
4. Falha no compile → código/header.
5. Falha no link → CRT, Qt, símbolo ausente.
6. Falha no run/ctest → DLL, PATH, plugin.

## Erros e correção

| Erro | Causa | Correção |
|---|---|---|
| `undefined reference to QApplication` | UCRT × MSVCRT | preset MSVCRT |
| `0xc0000135` | DLL Qt ausente | `PATH=C:\Qt\6.8.3\mingw_64\bin` |
| `Could not find Qt6` | `CMAKE_PREFIX_PATH` errado | `C:/Qt/6.8.3/mingw_64` |
| `No such file g++.exe` | winget atualizou | descobrir GCC dinamicamente |
| `AUTOMOC` sem símbolo | header fora dos sources | adicionar ao target |
| `QJsonValueRef` | conversão implícita | copiar para `QJsonValue` |
| `Permission denied` no link | exe em execução | feche o ArtPlus |

## Comandos

```powershell
$cmake = 'C:\Users\Evanei\AppData\Local\Microsoft\WinGet\Packages\Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe\cmake-4.4.4-windows-x86_64\bin\cmake.exe'
& $cmake -S H:\Desktop\ArtPlus -B H:\Desktop\ArtPlus\build --preset mingw-msvcrt
& $cmake --build H:\Desktop\ArtPlus\build --preset release-mingw -- -j4
& $cmake -E chdir H:\Desktop\ArtPlus ctest --preset unit-tests --output-on-failure
```

## Coleta para relatório

```powershell
& $cmake --build $build --preset release-mingw *> H:\Desktop\ArtPlus\docs\testing\build.log
Get-Content H:\Desktop\ArtPlus\docs\testing\build.log -Tail 80
Select-String -Path H:\Desktop\ArtPlus\build\CMakeCache.txt -Pattern 'CMAKE_CXX_COMPILER:|CMAKE_PREFIX_PATH:|CMAKE_BUILD_TYPE:'
```
