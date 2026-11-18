---
description: Captura o comportamento de uma ferramenta no ArtPro+ real
agent: comportamento
---

Ferramenta: $ARGUMENTS

Capture o comportamento observável conforme a skill `artpro-behavior-capture`.

```
ALVO: $ARGUMENTS
SPEC: F-### ou "criar"
SAÍDA: tools/reference_capture/F-###/{input,output,shots}, docs/behavior/<Ferramenta>.md, rag/evidence/F-###/behavior/
ACEITE: passos reais, texto literal de UI, campos (default/unidade/faixa), saída observada, erro e captura antes/depois, com CAP-AAAAMMDD-nn
LIMITE: uma sessão de GUI; no máximo uma variação de borda além do caso nominal
CONTEXTO: copie a prova de tools/test_documents; nunca abra o original
```

Regras: use `list_open_windows`/`get_elements`/`click_element` antes de coordenada; uma sessão por vez; bloqueio (HASP/crash) vira screenshot + registro; não edite `src/`, `tests/`, specs nem artefatos de Ghidra/REA.
