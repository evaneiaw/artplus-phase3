---
description: Executa o pipeline completo sem pedir decisões
agent: orquestrador
---

Escopo: $ARGUMENTS

Siga `docs/agents/AUTOPILOT.md` e o bloco "Modo autônomo" do seu prompt.
Versão de referência: ArtPro+ **v24.03.10017** (ver `docs/agents/REFERENCE_VERSION.md`).

Interpretação dos argumentos:
- vazio: execute a **próxima tarefa elegível** (§2 da política) e **continue**;
- `F-###` ou nome de módulo: **priorize** esse alvo;
- `--extract`: investigação, evidência, spec e dossiê — **sem implementação**;
- `--static`: triagem estática, **sem GUI**;
- `--max N`: no máximo N módulos nesta execução (limite solicitado pelo usuário, **não** uma regra padrão de encerramento).

Se `--static` estiver presente:
1. despache `rea` para triagem leve: `diec`, `strings64`, `scripts/inspect_pe.py` e exports existentes. **Proibido** `rea analyze`/`decompile` com Ghidra neste modo. **Não crie `manifest.json` nem IDs de evidência sem um `F-###` válido**; registre a triagem em `docs/reverse-engineering/`;
2. peça ao `testes` para gerar o dossiê;
3. se não houver export Ghidra e o módulo tiver até 12 MB, despache `ghidra` e peça o dossiê de novo. O `ghidra` grava só em `rag/ghidra/exports/` e `docs/reverse-engineering/`; não invente `manifest.json` sem `F-###`;
4. não acione `comportamento`, `documentacao`, `arquitetura`, `implementacao` nem `auditor`;
5. registre lacunas no dossiê e siga.

Execute o pipeline por funcionalidade: `rea` + `ghidra` + `comportamento`
(paralelo quando possível) → `documentacao` → `arquitetura` → `implementacao`
→ `testes` → `auditor`.

Não faça perguntas entre etapas. Escale apenas nos cinco casos da política.
Auditoria reprovada volta **uma vez** com a lacuna exata; a segunda reprovação vira
bloqueio registrado e o ciclo segue.

Continue para a próxima tarefa elegível — **não há limite padrão** de 1 módulo /
90 minutos / 6 ciclos. `--max N` é o único limite de quantidade, quando solicitado.

Ao concluir cada feature auditada: atualize `INVENTORY.md`, `ROADMAP.md`,
`FEATURE_MATRIX.csv` e `docs/agents/CHECKPOINT.md`; peça ao `testes` para rodar
`python scripts/dossier.py --module <módulo>`.

Ao atingir um limite **real** da sessão: escreva `docs/agents/CHECKPOINT.md` +
`docs/phases/STATUS.md` e informe **exatamente como retomar** (FEITO, BLOQUEADO,
PRÓXIMO, COBERTURA, ONDE LER).
