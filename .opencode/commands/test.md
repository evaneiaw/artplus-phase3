---
description: Escreve e roda os testes de um F-###
agent: testes
---

Funcionalidade: $ARGUMENTS

Escreva e execute os testes conforme a skill `test-strategy`.

```
ALVO: $ARGUMENTS
SPEC: F-### + ADR-### + código implementado
SAÍDA: tests/**, docs/testing/**
ACEITE: cada regra da spec com teste rastreável, tolerância explícita, ctest passando e saída colada
LIMITE: um F-###; não altere src
CONTEXTO: golden nasce de rag/evidence/F-###/behavior/; leia CMakeLists para o target correto
```

Rode build e `ctest --preset unit-tests` com `PATH=C:\Qt\6.8.3\mingw_64\bin`. Se o código estiver errado, relate `arquivo:linha` e devolva; não "conserte" o teste. Não edite `src/`, specs, evidência nem `CMakeLists.txt`. Relate comando, pass/fail por teste, tempo e o que não foi coberto.
