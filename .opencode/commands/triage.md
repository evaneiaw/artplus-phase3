---
description: Triagem estática de um binário do ArtPro+
agent: rea
---

Alvo: $ARGUMENTS

Faça a triagem estática do binário informado conforme a skill `rea-reverse-engineering`.

```
ALVO: $ARGUMENTS
SPEC: criar ou F-### existente
SAÍDA: docs/reverse-engineering/<Modulo>_TRIAGE.md, rag/evidence/F-###/rea/, rag/sources/
ACEITE: SHA-256 + versão, identidade DIE, imports/exports, strings de UI, RTTI, dependências e alvos para o Ghidra, com confiança
LIMITE: um binário; não inicie import no Ghidra
CONTEXTO: verifique DEPENDENCY_MAP.md e relatórios existentes antes de refazer
```

Regras: use a CLI REA validada (`rea-side`); não afirme comportamento por string; registre bloqueios; não edite `src/`, `tests/`, specs nem roadmap. Ao terminar, informe caminhos gerados e o que ficou fora.
