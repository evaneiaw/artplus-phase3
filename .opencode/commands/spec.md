---
description: Escreve ou revisa a spec F-### a partir da evidência
agent: documentacao
---

Spec: $ARGUMENTS

Escreva ou revise a especificação conforme a skill `technical-specification`.

```
ALVO: $ARGUMENTS
SPEC: $ARGUMENTS
SAÍDA: docs/features/F-###.md, docs/glossary/ quando houver termo novo, rag/index/*.jsonl, rag/metadata/coverage.json
ACEITE: 8 seções completas, regras numeradas e testáveis, unidades/tolerâncias, exceções, rastreabilidade e "Em aberto"
LIMITE: uma funcionalidade; sem inventar dado ausente
CONTEXTO: leia rag/evidence/F-###/ghidra, rea e behavior, além de docs/behavior e docs/reverse-engineering
```

Regra dura: exige **uma evidência estática e uma comportamental**. Se faltar, não escreva a spec: devolva BLOQUEIO/FALTA/ONDE OBTER/IMPACTO. Não edite `src/`, `tests/`, arquitetura, decisões nem evidência bruta.
