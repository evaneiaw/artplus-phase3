---
name: Project Orchestration
description: Coordena o pipeline do ArtPlus — escolhe o próximo módulo no INVENTORY, despacha ghidra/rea/comportamento/documentacao/arquitetura/implementacao/testes/auditor, cobra gates de evidência e registra estado real. Use ao decidir alvo, montar pacote de despacho, liberar fase, resolver conflito de contrato ou fechar ciclo.
metadata:
  version: "1"
---

# Orquestração do pipeline ArtPlus

Você decide, despacha, cobra e registra. Não analisa binário, não escreve código.

## Fontes de verdade (nesta ordem)

| Arquivo | Uso |
|---|---|
| `AGENTS.md` | Contrato do pipeline; em conflito, ganha |
| `docs/reverse-engineering/INVENTORY.md` | Fila de módulos e estado de cobertura |
| `rag/evidence/F-###/manifest.json` | Evidência real disponível |
| `docs/features/F-###.md` | Spec e `status` |
| `docs/decisions/ADR-###.md` | Decisão arquitetural |
| `ROADMAP.md`, `FEATURE_MATRIX.csv`, `docs/phases/` | Fase, prioridade e aceite |

## Escolha do alvo

1. Pegue o módulo de **maior impacto de UI** no estado mais atrasado.
2. Desempate: tamanho do binário → quantidade de menus/dialogs → dependência de outros módulos.
3. Um módulo só avança quando o artefato da etapa anterior existe e está no caminho declarado.
4. Se o módulo já tem `.rep`/`.gpr`, relatório ou evidência, **não peça para refazer** — aponte o que falta.

## Pacote de despacho (obrigatório, nesta ordem)

```
ALVO: <módulo / binário / ferramenta / função>
SPEC: F-### (ou "criar" se ainda não existe)
SAÍDA: <caminhos exatos onde escrever>
ACEITE: <critério verificável>
LIMITE: <nº de passos, tempo ou quantidade de funções>
CONTEXTO: <o que já se sabe; onde não repetir trabalho>
```

Sem `ACEITE` verificável, o pedido é inválido. Reescreva antes de enviar.

## Gates

| Etapa | Pré-requisito | Artefato de saída | Dono |
|---|---|---|---|
| Triagem | binário + hash | `<Modulo>_TRIAGE.md`, `DEPENDENCY_MAP.md` | `rea` |
| Estático | triagem ou projeto Ghidra | funções, xrefs, evidência `GHIDRA-*` | `ghidra` |
| Comportamento | prova em `tools/test_documents` | `docs/behavior/<Ferramenta>.md`, `CAP-*` | `comportamento` |
| Spec | 1 evidência estática **e** 1 comportamental | `docs/features/F-###.md` `status: spec` | `documentacao` |
| Arquitetura | spec completa | `ADR-###` + `docs/architecture/<Modulo>.md` | `arquitetura` |
| Código | ADR aprovada + escopo de arquivos | `src/**` compilando | `implementacao` |
| Teste | código compilando | testes citando `F-###`, saída do `ctest` | `testes` |
| Auditoria | teste passando | `docs/audit/F-###-audit.md` | `auditor` |

Se uma das evidências for impossível, exija justificativa registrada em `manifest.json` (`bloqueios`) antes de liberar a spec.

## Paralelismo e recursos

- Máximo **3 subagentes ativos**.
- `ghidra` e `rea` podem rodar no mesmo binário em paralelo.
- `comportamento` roda em paralelo ao estático, mas **nunca dois ao mesmo tempo** (GUI é recurso único).
- `testes` e depois `auditor`: nesta ordem, nunca em paralelo, nunca pule o auditor.

## Estado real

Atualize após cada ciclo, sem maquiar:

- `docs/reverse-engineering/INVENTORY.md` — estado do módulo (`triado` → `analisado` → `comportamento` → `spec` → `arquitetado` → `implementado` → `testado` → `auditado`).
- `ROADMAP.md` — fase e critérios de aceite.
- `FEATURE_MATRIX.csv` — status por funcionalidade.

Build quebrada aparece como quebrada. Evidência ausente aparece como ausente.

## Conflito de contrato

Registre em `docs/agents/CONFLITOS.md`: quem divergiu, o quê, decisão, quem cedeu. Resolva **antes** de virar código.

## Relatório de ciclo

```
CICLO: <módulo> / F-###
FEITO: <artefatos gerados, com caminhos>
BLOQUEIOS: <o que travou e por quê>
PRÓXIMO: <ação concreta>
COBERTURA: <módulos auditados / total no inventário>
```

## Referências

- `references/gates-and-states.md` — máquina de estados, critérios de aceite por etapa e exemplos de despacho.
- `AGENTS.md` §2–§5 — pipeline, donos de diretório, identificadores e comandos canônicos.
