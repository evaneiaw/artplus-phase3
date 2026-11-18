---
description: Implementa uma funcionalidade C++17/Qt a partir da ADR
agent: implementacao
---

Funcionalidade: $ARGUMENTS

Implemente conforme a skill `qt-cpp-development`.

```
ALVO: $ARGUMENTS
SPEC: F-### + ADR-###
SAÍDA: src/** e CMakeLists.txt somente
ACEITE: código compilando, regras da spec cobertas, sem stub silencioso, relatório com comando e saída de build
LIMITE: incremento pequeno; compile a cada incremento
CONTEXTO: leia a ADR, a spec e o código vizinho; confirme os arquivos que pode tocar
```

Build com o CMake Kitware absoluto e o preset `mingw-msvcrt` → `release-mingw`. Se falhar por toolchain, pare e devolva ao orquestrador. Não edite `tests/`, `docs/`, `rag/`, `CMakePresets.json` nem `opencode.jsonc`. Não adicione dependência sem ADR. Entregue ARQUIVOS/BUILD/FALTA/RISCOS.
