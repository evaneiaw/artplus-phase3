---
description: Audita uma entrega e emite veredito com evidência
agent: auditor
---

Funcionalidade: $ARGUMENTS

Audite conforme a skill `evidence-verification`.

```
ALVO: $ARGUMENTS
SPEC: F-### + evidência + código + teste
SAÍDA: docs/audit/F-###-audit.md
ACEITE: veredito com comando reproduzível, saída colada, cadeia spec→evidência→código→teste e divergências específicas
LIMITE: uma auditoria; não corrigir nada
CONTEXTO: verifique o manifest, o hash do binário e os arquivos citados antes de confiar
```

Faça build limpo do zero e rode `ctest` você mesmo. Sem comando + saída = REPROVADO. Não edite nada fora de `docs/audit/`. Reauditoria gera `F-###-audit-2.md` e cita o que mudou.
