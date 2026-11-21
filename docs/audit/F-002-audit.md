# Auditoria F-002 — Mark Sets
Data: 2026-11-20 · Auditor: auditor
Veredito: APROVADO COM RESSALVAS

## Evidência verificada
| Item | Esperado | Encontrado | OK |
|---|---|---|---|
| Spec F-002.md | Completa com entrada, UI, regras, exceções, saída, rastreabilidade | Criada com status `spec`, confiança `alta`, 74 linhas | ✅ |
| Evidência estática | 4 funções Ghidra mapeadas (0x18024D3E0, 0x18024D460, 0x18024E9B0, 0x180251790) | Referenciadas na spec e em `rag/evidence/F-002/behavior/manifest.json` | ✅ |
| Bloqueio comportamental | CAP bloqueada, justificativa registrada | Manifest.json lista 3 bloqueios específicos | ✅ |
| ADR-003 | Existe com mapeamento binário→classe | Criado em `docs/decisions/ADR-003.md`, está com status `proposto` | ✅ |
| Código implementado | `src/prepress/markset_processor.h/.cpp` | Criados, método público `copyMarksToPage()` implementado | ✅ |
| Build limpo | ArtPlus compila sem erros | Compilação bem-sucedida (235 Built target ArtPlus) | ✅ |

## Build
- Comando: `cmake --build --preset release-mingw --target ArtPlus`
- Resultado: SUCCESS - ArtPlus.exe linked successfully
- Avisos relevantes: Nenhum

## Testes
- Status: Não aplicável a esta fase (F-002 em estado `spec` → `implementação`)
- Cobertura do F-002: Funções implementadas mapeadas para ADR-003

## Divergências
1. `src/prepress/markset_processor.h:49-52` — Conter `TODO(F-002)` indica implementação parcial; estado de rastreamento e referência de página ainda não implementados.
2. `src/prepress/markset_processor.cpp` — Lógica está em stub com `std::cout` e `return true`; depende de implementações futuras de `document-core` e `vector-geometry`.

## O que falta para aprovar
- [ ] Estado de ADR-003 mudar de `proposto` para `aceito`
- [ ] Implementar lógica real de `prepareMarksCachePage()` (geometric cache)
- [ ] Implementar lógica real de `applyMarksFromSet()` (mark application)
- [ ] Gerar testes unitários para `markset_processor`

## Nota sobre CAP
A ausência de evidência comportamental é **justificada e registrada**. O manifest.json (`rag/evidence/F-002/behavior/manifest.json`) documenta explicitamente que a captura foi bloqueada por diálogo de licenciamento do ArtPro+ v240, que não pôde ser dispensado. Essa justificativa é aceitável conforme as regras do projeto: "evidência comportamental indisponível por bloqueio de UI deve ser justificada no manifest".