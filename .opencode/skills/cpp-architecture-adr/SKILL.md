---
name: C++ Architecture and ADR
description: Traduz specs F-### em arquitetura C++17/Qt 6.8.3 — fronteiras de módulo, classes, contratos de interface, ADR e mapeamento função-do-binário → método-do-ArtPlus. Use ao decidir onde uma funcionalidade vive, propor classe, autorizar dependência ou registrar decisão.
metadata:
  version: "1"
---

# Arquitetura C++/Qt e ADR

Você decide **onde** a funcionalidade vive e **como** os módulos conversam. Não implementa.

## Entradas

```
docs/features/F-###.md       spec (exigida, status: spec)
docs/architecture/           desenho existente
docs/decisions/              ADRs anteriores
src/                         estrutura atual (leitura)
CMakeLists.txt               alvos e dependências
AGENTS.md                    donos e contrato
```

## Mapa de módulos atual

| Área | Diretório | Responsabilidade |
|---|---|---|
| Documento | `src/document/` | `Document`, `Page`, `Layer`, objetos, undo, serialização |
| PDF | `src/pdf/` | lexer, parser, documento PDF, import, export, content streams |
| Prepress | `src/prepress/` | trapping, marks, step & repeat, imposição, packaging |
| Cor | `src/color/` | perfis ICC, conversões, spot, simulação |
| Geometria | `src/geometry/` | Bézier, booleanos, offset, simplificação |
| Automação | `src/automation/` | Action Lists, scripting, performance, recovery |
| UI | `src/ui/` | `MainWindow`, painéis, interação |
| App | `src/app/` | entry point |

Reuse o que existe antes de propor classe nova. Classe nova exige justificativa explícita.

## ADR (`docs/decisions/ADR-###-<slug>.md`)

```markdown
# ADR-012 — Marca de corte

## Contexto
Spec F-025; função 0x18001A2C0 em DocumentMarks.dll.

## Decisão
`TrimMark` em `src/prepress/marks.h`, integrada a `MarksEngine`.

## Mapeamento binário → ArtPlus
| Função (Ghidra) | Classe/método | Observação |
|---|---|---|
| 0x18001A2C0 | `TrimMark::build()` | calcula geometria |
| 0x18001A380 | `TrimMark::apply(Document&)` | insere no documento |

## Contrato de interface
```cpp
class TrimMark {
public:
    struct Params { double offsetMm; double lengthMm; double widthMm; bool onAllSides; };
    static TrimMark fromParams(const Params&);
    void apply(Document& doc, size_t pageIndex) const;
};
```

## Alternativas descartadas
...

## Consequências
Arquivos afetados, risco, migração de dados.

## Estado
proposto | aprovado | implementado
```

## Regras de decisão

- Toda decisão cita `F-###` e a evidência que a sustenta.
- Contrato que atravessa módulos exige ADR própria e revisão do orquestrador.
- Dependência nova exige ADR **e** auditoria de licença (`dependency-license-audit`).
- Unidades no tipo: `offsetMm`, `widthPt`, `dpi`; nunca `double offset` sem unidade.
- Coordenada do documento é **ponto PDF (1/72")**; conversão para mm fica na borda de UI/spec.
- Não coloque regra de negócio em `src/ui/`; UI chama o motor.
- Não coloque dependência de Qt Widgets em módulo de domínio (document, pdf, prepress, geometry, color).
- Spec ambígua: devolva apontando o parágrafo — não decida sozinho.

## Saída 2 — visão do módulo (`docs/architecture/<Modulo>.md`)

Responsabilidades, dependências permitidas e proibidas, fluxo de dados, pontos de extensão, o que **não** entra nele.

## Referências

- `references/adr-template.md` — ADR preenchível com checklist de aceite.
- `references/src-module-map.md` — mapa detalhado de arquivos, classes e dependências atuais.
