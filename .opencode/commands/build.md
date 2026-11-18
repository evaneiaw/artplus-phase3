---
description: Build doctor e testes do estado atual do projeto
agent: testes
---

Execute o build doctor e a suíte conforme a skill `build-and-release`.

Alvo opcional: $ARGUMENTS (se informado, limite o build a esse alvo; senão, projeto inteiro).

```
ALVO: estado atual do repositório
SPEC: não se aplica
SAÍDA: docs/testing/build.log, docs/testing/ctest.log
ACEITE: configure com toolchain correto, build Release e ctest com saída real
LIMITE: não alterar src
CONTEXTO: build atual conhecido precisa de mingw-msvcrt e CMake Kitware absoluto
```

Passos: apague/valide `build/`, confirme `CMAKE_CXX_COMPILER` POSIX.MSVCRT e `CMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64`, compile e rode `ctest` com `PATH=C:\Qt\6.8.3\mingw_64\bin`. Se falhar, diagnostique por etapa (configure/compile/link/run) e devolva o erro exato; não edite `src/` para passar. Relate comando, resultado e tempo.
