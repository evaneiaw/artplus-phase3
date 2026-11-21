# Conflitos de contrato

Registro de divergências entre especialistas. Formato: data, agentes, tema, decisão, quem cedeu.

## 2026-11-21 — Versão de referência (v24 × v25)

- **Divergiu**: `AGENTS.md`, `docs/agents/AUTOPILOT.md`, `.opencode/agents/*`,
  `.opencode/skills/*`, `docs/features/*`, `scripts/*` usavam a v25
  (`bg_prog_artpro+_v250` = 25.03.116) como referência.
- **O quê**: a ordem executiva de reestruturação estabelece a v24.03.10017
  (`bg_prog_artpro+_v240`) como única referência válida.
- **Decisão**: v24.03.10017 é a referência canônica (`docs/agents/REFERENCE_VERSION.md`).
  Registros derivados da v25 ficam **históricos e fora do fluxo ativo**.
- **Quem cedeu**: as referências à v25 nos arquivos ativos (corrigidas/marcadas como históricas).
  Hashes e resultados históricos **não** foram alterados para simular v24.

## 2026-11-21 — Permissão de edição de configuração (bootstrap)

- **Divergiu**: o frontmatter do `orquestrador` (`permission.edit/write: "*": deny`)
  impedia editar `AGENTS.md`, `opencode.jsonc` e `.opencode/**`.
- **O quê**: a reestruturação (FASE C/D/E) exige que o orquestrador edite esses arquivos.
- **Decisão**: o usuário autorizou a expansão da permissão (edição manual de
  `.opencode/agents/orquestrador.md` em 2026-11-21).
- **Quem cedeu**: o bloco de permissão restritivo (substituído por lista explícita de allow).
