---
description: Escreve a ADR e a visão de módulo a partir da spec
agent: arquitetura
---

Spec: $ARGUMENTS

Produza a decisão arquitetural conforme a skill `cpp-architecture-adr`.

```
ALVO: $ARGUMENTS
SPEC: $ARGUMENTS
SAÍDA: docs/decisions/ADR-###-<slug>.md, docs/architecture/<Modulo>.md
ACEITE: contexto com F-### e evidência, decisão, mapa função→classe, contrato C++17/Qt, alternativas, consequências, arquivos afetados e estado
LIMITE: uma ADR por funcionalidade; não implementar
CONTEXTO: leia a spec, ADRs existentes e src/ apenas para reutilizar o que já existe
```

Regras: reutilize classe existente ou justifique a nova; unidade no tipo; contrato entre módulos exige ADR própria; dependência nova exige ADR e auditoria de licença. Se a spec estiver ambígua, devolva apontando o parágrafo — não decida sozinho. Não edite `src/`, `tests/`, specs nem evidência.
