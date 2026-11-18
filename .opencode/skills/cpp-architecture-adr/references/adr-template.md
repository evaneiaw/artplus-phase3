# Template de ADR

```markdown
# ADR-012 — Marca de corte

## Contexto

Spec F-025; função `0x18001A2C0` em `DocumentMarks.dll`; evidência
`GHIDRA-DocumentMarks-0x18001A2C0` e `CAP-20261118-03`.

## Decisão

Nova classe `TrimMark` em `src/prepress/marks.h`, integrada a `MarksEngine`.

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

1. Reutilizar `MarksEngine::addCropMark` — não cobre `allSides=false`.
2. Implementar na UI — quebraria a regra de domínio fora de `src/ui/`.

## Consequências

- Arquivos: `src/prepress/marks.h/.cpp`, `CMakeLists.txt`.
- Risco: conflito com marca existente na mesma camada.
- Migração: nenhuma; funcionalidade nova.

## Checklist

- [ ] Cita `F-###` e evidência.
- [ ] Reusa o que existe ou justifica classe nova.
- [ ] Unidades no tipo.
- [ ] Sem dependência de Qt Widgets no domínio.
- [ ] Dependência nova? ADR de licença anexa.
- [ ] Escopo de arquivos declarado para `implementacao`.

## Estado

proposto | aprovado | implementado
```
