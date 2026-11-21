---
description: Painel de status da extracao (dossies, funcoes, fila, relatorio)
agent: orquestrador
---

Status lido de `dossiers/*/manifest.json`, `docs/phases/OVERNIGHT-*.md` e `logs/auto/extract-detached.log`.

```
!`python H:\Desktop\ArtPlus\scripts\status.py --text`
```

O painel acima é o status atual. Responda apenas —.

> Se a substituição ``!`…` `` não executar (o agente `orquestrador` pode não ter shell exposto nesta sessão), peça ao subagente `testes` para rodar `python scripts/status.py --text` e use a saída.

Dashboard com auto-refresh: `docs/status/dashboard.html` (abrir no navegador).
JSON consumivel por script: `logs/auto/status.json`. Atualizacao continua: `python scripts/status.py --watch 2`.
