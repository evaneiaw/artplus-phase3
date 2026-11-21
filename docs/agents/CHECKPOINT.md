# CHECKPOINT — checkpoint operacional do ArtPlus

Arquivo único de retomada. O `orquestrador` atualiza este arquivo **após cada etapa**
e ao fim da sessão. Retomada lê este arquivo e continua **sem repetir trabalho**.

## Estado atual

| Campo | Valor |
|---|---|
| Última tarefa concluída | FASE G (auditoria) + FASE G.1 (correções P1–P12) |
| Tarefa atual | Aguardando autorização para a FASE H (teste de autonomia / validação em sessão nova) |
| Próximas tarefas | FASE H: validar `default_agent` e shell/git do `orquestrador` em **sessão nova**; testar `/auto` (delegação, checkpoint, retomada). Depois FASE I (commit/push). |
| Especialistas acionados | `rea` (versões), `testes` (check_version + scripts), `documentacao` (specs), `arquitetura` (ADRs) |
| Arquivos modificados | `AGENTS.md`, `opencode.jsonc`, `.opencode/agents/{orquestrador,comportamento,documentacao,ghidra,rea}.md`, `.opencode/commands/auto.md`, `.opencode/skills/**`, `docs/agents/{REFERENCE_VERSION,AUTOPILOT,CHECKPOINT,CONFLITOS}.md`, `docs/phases/STATUS.md`, `docs/reverse-engineering/{INVENTORY,VERSION_CONFIRMATION}.md`, `ROADMAP.md`, `FEATURE_MATRIX.csv`, `docs/features/F-00{1..6,8}.md`, `docs/decisions/*`, `docs/architecture/*`, `scripts/{check_version,status,auto_extract,dossier}.py`, `rag/index/*`, `rag/metadata/*` |
| Estado do build | **não alterado** nesta tarefa (sem mudança de código-fonte) |
| Estado dos testes | **não alterado** nesta tarefa |
| Auditorias | **FASE G concluída** — relatório P1–P12; **G.1** corrigiu P1–P12 (itens de validação em nova sessão seguem pendentes) |
| Bloqueios | F-007 (`bloqueado` — comportamento `BLO-003`); F-004 e F-005 (ressalvas de evidência) |
| Último commit | **nenhum nesta tarefa** (commit/push não autorizado ainda) |
| Próxima ação executável | Iniciar **sessão nova** e confirmar: (a) agente padrão = `orquestrador` (`default_agent`); (b) shell/git do orquestrador exposto; (c) `/progress` executa. Depois `/auto` controlado. |

## Validações pendentes (exigem sessão nova — não presumir)

1. **`default_agent` (P11)** — confirmar que a 1.2.27 reconhece `default_agent: "orquestrador"`; se não, cairá em `build`.
2. **Shell/git do `orquestrador` (P1)** — confirmar que a permissão `bash:{git...}` expõe a ferramenta de terminal; se não, commit/push fica delegado ao `testes`.
3. **`/progress` (P8)** — confirmar se a substituição `!`python scripts/status.py`` funciona sob o `orquestrador`.

## Correções da FASE G.1 (resumo)

- P2: `maxSteps` 80→200 + disciplina de checkpoint/retomada (AGENTS §6 regra 10; AUTOPILOT §8).
- P6: estado `bloqueado` no INVENTORY + regra de não redistribuição (AUTOPILOT §2/§6).
- P3/P4/P5: F-006→Spec; F-007→Step & Repeat/Bloqueado (evidência); F-004→Aprovado com Ressalvas.
- P7: git do orquestrador restrito a status/add/commit/push-master/log/diff/rev-parse.
- P9: limite de 3 subagentes documentado como obrigatório sem enforcement automático.
- P1/P8/P10/P11: mantidos como validação em sessão nova / sem ação especulativa.

## Como retomar

1. `git status` / `git log -1` para confirmar estado.
2. Ler `docs/agents/REFERENCE_VERSION.md`, `docs/agents/CHECKPOINT.md`, `docs/phases/STATUS.md`.
3. `python scripts/check_version.py` (UTF-8: `$env:PYTHONUTF8='1'`) → deve seguir APROVADO.
4. FASE H: validar `default_agent` + shell/git + `/auto` em sessão nova.
5. FASE I: commit/push (delegado ao `testes` ou em sessão com shell).
