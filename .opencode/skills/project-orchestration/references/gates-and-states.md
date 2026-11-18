# Gates e máquina de estados

## Estados do módulo (INVENTORY)

| Estado | Critério de entrada | Quem move |
|---|---|---|
| `nao-tratado` | nada feito | — |
| `triado` | identidade + superfície + dependências + SHA-256 | `rea` |
| `analisado` | funções principais com endereço, xrefs e confiança | `ghidra` |
| `comportamento` | ferramenta operada e capturada com `CAP-*` | `comportamento` |
| `spec` | `F-###` completa e com evidência | `documentacao` |
| `arquitetado` | `ADR-###` aprovada + mapeamento função → classe | `arquitetura` |
| `implementado` | `src/**` compilando | `implementacao` |
| `testado` | teste citando `F-###` passando | `testes` |
| `auditado` | laudo aprovado com comando e saída | `auditor` |

Um módulo só é `auditado` quando **todas** as funcionalidades de UI que expõe estão `auditado`.

## Gate de liberação

| Para liberar | Exija | Rejeite se |
|---|---|---|
| `ghidra` | triagem ou projeto existente | pedido duplicado de import |
| `comportamento` | prova em `tools/test_documents` | original será aberto direto |
| `documentacao` | 1 evidência estática + 1 comportamental | só string ou só screenshot |
| `arquitetura` | spec `status: spec` completa | seção “Em aberto” com regra central |
| `implementacao` | ADR aprovada + arquivos declarados | “ajusta aí” sem contrato |
| `testes` | código compilando | build quebrada por toolchain |
| `auditor` | teste passando + evidência | “funciona na minha máquina” |

## Exemplo de despacho

```
ALVO: DocumentMarks.dll / Trim Marks
SPEC: F-025
SAÍDA: rag/evidence/F-025/ghidra/, docs/reverse-engineering/DocumentMarks_FUNCTIONS.md
ACEITE: funções de criação de marca com endereço, xrefs e strings de UI; confiança declarada
LIMITE: 40 passos, 90 min, 8 funções
CONTEXTO: projeto ArtProPlus já importado; não reimportar; triagem em DocumentMarks_TRIAGE.md
```

## Quando o gate não fecha

- Evidência impossível: registre em `manifest.json` → `bloqueios` com motivo e screenshot/log.
- Conflito entre agentes: `docs/agents/CONFLITOS.md` antes de qualquer código.
- Módulo gigante: divida por funcionalidade (`F-025a`, `F-025b`), nunca por “terminar o módulo”.

## Registro de ciclo

Atualize `INVENTORY.md`, `ROADMAP.md` e `FEATURE_MATRIX.csv` no mesmo ciclo. Estado desejado não substitui estado real.
