# STATUS — estado do projeto ArtPlus

Atualizado: 2026-11-21 · Supervisor: `orquestrador`

## Referência de versão

Esko ArtPro+ **v24.03.10017** (`C:\Esko\bg_prog_artpro+_v240`). Fonte única:
`docs/agents/REFERENCE_VERSION.md`. A v25 (`bg_prog_artpro+_v250` = 25.03.116) é
histórica e fora do fluxo ativo.

## Reestruturação de autonomia (ordem executiva)

- **FASE A (diagnóstico)**: concluída. Versões confirmadas; config OpenCode inspecionada.
- **FASE B (versão)**: concluída — `scripts/check_version.py` APROVADO (0 contaminação em ativos).
- **FASE C/D/E (contrato/agentes/skills//auto/default_agent)**: alterações gravadas.
- **FASE F (checkpoint)**: `docs/agents/CHECKPOINT.md` (+ estado `bloqueado`, retomada).
- **FASE G (auditoria)**: concluída — relatório P1–P12.
- **FASE G.1 (correções P1–P12)**: concluída (com itens de validação em sessão nova).
- **FASE H (teste de autonomia)**: **pendente** — exige sessão nova (validar `default_agent`, shell/git, `/auto`).
- **FASE I (commit/push)**: **pendente**.

## Cobertura de funcionalidades

- Auditadas: F-001…F-005 (com ressalvas registradas; F-003 sem laudo localizado).
- Spec: F-006, F-008.
- **Bloqueado**: F-007 (Step & Repeat / LayoutEngine — comportamento BLO-003).
- Ver `docs/reverse-engineering/INVENTORY.md` e `FEATURE_MATRIX.csv`.

## Validações que exigem sessão nova

- `default_agent: "orquestrador"` reconhecido pela 1.2.27.
- Shell/git do `orquestrador` exposto.
- Comando `/progress` executando a substituição de shell.

## Próxima ação

FASE H — sessão nova para validar autonomia; depois FASE I (commit/push).
