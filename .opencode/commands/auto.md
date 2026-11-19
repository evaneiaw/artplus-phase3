---
description: Executa o pipeline completo sem pedir decisões
agent: orquestrador
---

Escopo: $ARGUMENTS

Siga `docs/agents/AUTOPILOT.md` e o bloco "Modo autônomo" do seu prompt.

Interpretação dos argumentos:
- vazio: escolha o alvo pelo §2 da política e diga em uma linha;
- `--extract`: pare após evidência + spec + dossiê, sem implementar;
- `--max N`: no máximo N módulos;
- qualquer outro texto: módulo ou `F-###` alvo.

Execute o pipeline por funcionalidade: `rea` + `ghidra` + `comportamento`
(paralelo quando possível) → `documentacao` → `arquitetura` → `implementacao`
→ `testes` → `auditor`.

Não faça perguntas entre etapas. Escale apenas nos cinco casos da política.
Auditoria reprovada volta uma vez com a lacuna exata; a segunda reprovação vira
bloqueio registrado e o ciclo segue.

Ao concluir cada feature auditada: atualize `INVENTORY.md`, `ROADMAP.md` e
`FEATURE_MATRIX.csv`; peça ao `testes` para rodar
`python scripts/dossier.py --module <módulo>`.

Ao fim do orçamento: escreva `docs/phases/STATUS.md` e responda em 5 linhas
(FEITO, BLOQUEADO, PRÓXIMO, COBERTURA, ONDE LER).
