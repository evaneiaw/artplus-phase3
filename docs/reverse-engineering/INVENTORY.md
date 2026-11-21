# INVENTORY.md — fila de módulos e estado de cobertura

Fonte única da verdade de versão: `docs/agents/REFERENCE_VERSION.md` — Esko ArtPro+ **v24.03.10017**.

Estado do pipeline: `triado` → `analisado` → `comportamento` → `spec` →
`arquitetado` → `implementado` → `testado` → `auditado`.

Estado terminal alternativo: **`bloqueado`** — feature registrada em
`docs/agents/BLOQUEIOS.md`. Não é redistribuída automaticamente até o bloqueio
ser resolvido (ver `AUTOPILOT.md` §2 item 6 e §6).

Cada feature declara: **estado**, **próximo dono**, **próxima ação** e
**evidência de conclusão**.

| # | Feature | Módulo/Binário | Estado | Próximo dono | Próxima ação | Evidência de conclusão |
|---|---|---|---|---|---|---|
| 1 | Cut Mark (F-001) | DocumentMarks.dll | auditado | orquestrador | arquivar / dossiê | `docs/audit/F-001-audit-2.md`; CutMarkTests 12/12 |
| 2 | Mark Sets (F-002) | DocumentMarks.dll | auditado | implementacao | completar stubs (cache + aplicação real) | `docs/audit/F-002-audit.md`; ADR-003 |
| 3 | Bearer Bar (F-003) | DocumentMarks.dll | auditado | auditor | **confirmar/gerar laudo** (nenhum `F-003-audit.md` em `docs/audit/`) | `docs/features/F-003.md`; ADR-002; `src/prepress/bearer_bar.*` |
| 4 | Selection Mark (F-004) | DocumentMarks.dll | auditado (ressalvas) | comportamento | CAP comportamental (bloqueado por HASP) | `docs/audit/F-004-audit.md` — **APROVADO COM RESSALVAS** (`auditado_bloqueio_ambiental`) |
| 5 | Open Document (F-005) | Document.dll | auditado (ressalvas) | rea | corrigir SHA-256 do manifest | `docs/audit/F-005-audit.md` |
| 6 | Save Document (F-006) | Document.dll | spec | arquitetura | ADR + implementação | `docs/features/F-006.md` (`status: spec`); ADR-006 |
| 7 | Step & Repeat (F-007) | LayoutEngine.dll | **bloqueado** (comportamento) | documentacao | spec com evidência estática + resolver CAP (BLO-003) | `rag/evidence/F-007/manifest.json`; `docs/reverse-engineering/LayoutEngine_GHIDRA.md`; `docs/agents/BLOQUEIOS.md` BLO-003 |
| 8 | Color Management (F-008) | ColorManipulationLayer.dll | spec | arquitetura | ADR + implementação | `docs/features/F-008.md` (`status: spec`) |

> **Nota F-007 (identidade verificada):** `rag/evidence/F-007/manifest.json`
> identifica `LayoutEngine.dll` (funções `GenerateLayout`, `GenerateClassicSolution`,
> `Nest`) e `docs/reverse-engineering/LayoutEngine_GHIDRA.md` declara "Espec: F-007".
> Portanto F-007 = **Step & Repeat / Layout**, **não** "Zoom/Pan". "Zoom/Pan" é
> outra funcionalidade (View) e não recebeu número `F-###` ainda.

## Cobertura por estado

- **auditado**: 5 (F-001…F-005)
- **spec**: 2 (F-006, F-008)
- **bloqueado**: 1 (F-007 — comportamento)
- **Total rastreado**: 8
