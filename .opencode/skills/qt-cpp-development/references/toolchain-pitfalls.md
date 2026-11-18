# Armadilhas de toolchain

## UCRT × MSVCRT

- Qt 6.8.3 `mingw_64` é compilado com MSVCRT.
- O `g++` do PATH pode ser WinLibs **UCRT**.
- Misturar corrompe heap e gera `undefined reference to QApplication`, crash no start e `0xc0000135`.
- Solução: preset `mingw-msvcrt` aponta para `BrechtSanders.WinLibs.POSIX.MSVCRT`.

## Dois CMake

| Origem | Caminho | Uso |
|---|---|---|
| Winget | `...\Kitware.CMake_...\cmake-4.4.4-windows-x86_64\bin\cmake.exe` | correto |
| WinLibs | `...\mingw64\bin\cmake.exe` | não usar |

O preset fixa o hash do winget; quando o pacote atualiza, o caminho morre. Conserte com descoberta dinâmica.

## Dois Python

`python` no PATH = 3.12 (`...\Programs\Python\Python312`). Existe um 3.14 em `C:\Python314`. Fixe o interpretador em script de análise.

## `0xc0000135` no ctest

DLL do Qt não encontrada. O CMake define `PATH=C:/Qt/6.8.3/mingw_64/bin` em vários testes, mas não em todos. Para rodar local:

```powershell
$env:PATH = "C:\Qt\6.8.3\mingw_64\bin;$env:PATH"
ctest --preset unit-tests
```

## `QJsonValueRef` e conversão implícita

Já houve falha de compilação por `QJsonValueRef` → `QJsonValue`. Ao ler JSON:

```cpp
const QJsonValue v = obj.value("campo");   // copie para QJsonValue
```

Não passe `obj["campo"]` para função que espera `QJsonValue` por valor sem conversão explícita.

## Autorun/MOC

`CMAKE_AUTOMOC/AUTORCC/AUTOUIC` estão ligados. Se um `Q_OBJECT` novo não gerar símbolo, confirme que o header está na lista de sources do target (AUTOMOC varre os sources listados).

## Paths longos e espaço

`H:\Desktop\ArtPlus` não tem espaço; `C:\Program Files\Eclipse Adoptium\...` tem. Sempre quote com `& '...'`.

## Log de build

Redirecione e leia o fim:

```powershell
& $cmake --build $build --preset release-mingw *> build.log
Get-Content build.log -Tail 60
```
