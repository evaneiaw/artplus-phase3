---
name: Technical Specification
description: Converte evidência de Ghidra/REA/Comportamento em especificação F-### verificável, com frontmatter, regras numeradas, unidades, exceções e rastreabilidade; mantém o índice RAG e o glossário. Use ao escrever spec, revisar cobertura ou responder o que falta para implementar sem abrir o binário.
metadata:
  version: "1"
---

# Especificação técnica F-###

Uma spec boa permite implementar **sem abrir o binário** e sem adivinhar. Se falta evidência, a spec não é escrita.

## Entradas obrigatórias

```
rag/evidence/F-###/ghidra/      achados estáticos
rag/evidence/F-###/rea/         triagem e dependências
rag/evidence/F-###/behavior/    capturas do original
docs/reverse-engineering/       relatórios por módulo
docs/behavior/                  comportamento observado
```

Mínimo para `status: spec`: **uma evidência estática e uma comportamental** do mesmo `F-###`. Se uma for impossível, exija justificativa em `manifest.json` → `bloqueios`.

## Frontmatter obrigatório

```yaml
---
id: F-025
modulo: DocumentMarks.dll
ui: Marks > Create > Trim Marks
versao_ref: 25.0
status: spec            # spec | architecture | implemented | tested | audited
confianca: alta         # alta | media | baixa
evidencia: [GHIDRA-DocumentMarks-0x18001A2C0, CAP-20261118-03]
adr: ADR-012
tests: []
---
```

## Corpo (8 seções, nesta ordem)

1. **Resumo** — o que faz, em 3 linhas.
2. **Entrada** — tipo de documento, seleção, camadas, pré-condições.
3. **UI** — menu, diálogo, campos (`nome | tipo | default | unidade | faixa`), atalhos.
4. **Regras** — algoritmo em passos numerados, com fórmulas e unidades explícitas.
5. **Exceções** — entrada inválida, vazia, fora de faixa, documento sem camada.
6. **Saída** — efeito no documento, o que é gravado no PDF, o que é só visual.
7. **Rastreabilidade** — tabela `evidência | o que prova | caminho`.
8. **Em aberto** — o que não se sabe, com a pergunta objetiva.

## Barra de qualidade

- Toda regra precisa ser **testável**: valor de entrada → valor/estado de saída.
- Números sempre com unidade e tolerância (`offsetMm = 3.0 ± 0.01`).
- Comportamento observado e inferência não se misturam: inferência vai como `> Inferência: ... (fonte: <evidência>)`.
- Texto de UI é literal, entre aspas.
- Uma spec por funcionalidade; funcionalidade grande vira `F-025a`, `F-025b`.
- Nada de "provavelmente", "deve ser", "similar ao".
- Português técnico, sem floreio.

## Índice RAG

- `rag/index/features.jsonl` — uma linha por spec: `{"id","modulo","ui","status","evidencia","tags"}`.
- `rag/index/functions.jsonl` — `{"modulo","endereco","nome","spec","confianca"}`.
- `rag/metadata/schema.json` — schema dos dois arquivos; versione `schema_version` se mudar.
- `rag/metadata/coverage.json` — contagem por estado, coerente com `INVENTORY.md`.

`rag/index/*.jsonl` deve ser JSON válido por linha: sem vírgula final, sem comentário, UTF-8.

## Glossário

Termo novo de prepress (trap, spread, choke, overprint, separação, tinta spot) entra em `docs/glossary/` com definição de uma frase e a spec que o usa. Não invente sinônimo para termo que o ArtPro+ já nomeia.

## Devolução ao orquestrador

Se faltar evidência, devolva assim:

```
BLOQUEIO: F-### / <regra>
FALTA: <o dado exato>
ONDE OBTER: <ghidra|rea|comportamento> / <alvo>
IMPACTO: <o que não pode ser especificado sem isso>
```

## Referências

- `references/spec-template.md` — template pronto para copiar.
- `references/rag-index-schema.md` — schemas, exemplos e validação.
