# ROADMAP - ArtPlus

> **Referência funcional:** Esko ArtPro+ **v24.03.10017** (`C:\Esko\bg_prog_artpro+_v240`).
> Fonte única da verdade: `docs/agents/REFERENCE_VERSION.md`.

## Fases do Projeto

1.  **FASE 1**: Ambiente, inventário e documentação.
2.  **FASE 2**: Arquitetura, agentes, skills, RAG e dependências.
3.  **FASE 3**: Modelo central de documento e engine PDF.
4.  **FASE 4**: Visualização, seleção, edição e transformação.
5.  **FASE 5**: Layers, Separations, cores e geometria.
6.  **FASE 6**: Pré-impressão e Quality Control.
7.  **FASE 7**: Marks, Step & Repeat e produção.
8.  **FASE 8**: Action Lists e automação.
9.  **FASE 9**: Recursos avançados e distribuição.

## Estado atual (2026-11-21)

- **F-001 Cut Mark**: auditado com ressalvas — `docs/audit/F-001-audit-2.md`.
  Código em `src/prepress/cut_mark.*`, `CutMarkTests` 12/12.
- **F-002 Mark Sets**: auditado com ressalvas — `docs/audit/F-002-audit.md`.
  Spec em `docs/features/F-002.md`, ADR em `docs/decisions/ADR-003.md`, código em `src/prepress/markset_processor.*`.
  Falta: implementação completa de cache e aplicação real de marcas (stubs atuais).
- **F-005 Open Document**: aprovado com ressalvas — `docs/audit/F-005-audit.md`.
  Spec `docs/features/F-005.md`, ADR `docs/decisions/ADR-013`, código `src/document/**` + `src/pdf/**`.
  Build OK (`ArtPlus.exe` gerado, exit 0). ctest 4/6 (2 falhas por runtime 0xc0000135, não F-005).
  Correção pendente: SHA-256 no `rag/evidence/F-005/manifest.json` (despachado ao `rea`).
- **Extração contínua**: 20/196 módulos no último log, com dossiê e export Ghidra.
- Pendências de F-001: F-001a (roundtrip PDF+), F-001b (legado AI), F-001c (`fx`),
  painel `Marks`/Inspector e fidelidade visual (5 mm / 0,25 pt / magenta).
