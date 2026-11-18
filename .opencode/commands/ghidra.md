---
description: Analisa módulo ou função no Ghidra 12.2 headless
agent: ghidra
---

Alvo: $ARGUMENTS

Analise o alvo conforme a skill `ghidra-analysis`.

```
ALVO: $ARGUMENTS
SPEC: F-### ou "criar"
SAÍDA: docs/reverse-engineering/<Modulo>_FUNCTIONS.md, rag/evidence/F-###/ghidra/, rag/ghidra/exports/
ACEITE: funções com endereço, assinatura, xrefs, strings vizinhas, vtable/RTTI quando houver, confiança e motivo
LIMITE: no máximo um import grande; declare o que ficou fora
CONTEXTO: procure projeto .rep/.gpr e relatório existente antes de importar; não reimporte
```

Regras: `JAVA_HOME` = JDK 25; Ghidra em `H:\Desktop\ArtPlus\tools\ghidra_12.2_PUBLIC`; import em background com stdin vazio; pseudocódigo via REA CLI ou script, nunca screenshot. Evidência no formato `GHIDRA-<Modulo>-<endereco>`. Não edite `src/`, `tests/`, specs nem `INVENTORY.md`.
