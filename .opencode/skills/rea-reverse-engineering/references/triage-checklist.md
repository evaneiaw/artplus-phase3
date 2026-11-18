# Checklist de triagem

## Identidade

- [ ] Caminho absoluto e versão (`v250`/`v240`).
- [ ] SHA-256 (`Get-FileHash -Algorithm SHA256`).
- [ ] Arquitetura (x86/x64), subsistema (GUI/console), timestamp.
- [ ] Compilador/linker (DIE), packer/protector, entropia por seção.
- [ ] Assinatura digital presente? (registrar, não contornar)

## Superfície

- [ ] Exports (nome, ordinal, função provável).
- [ ] Imports agrupados por DLL: rede, cripto, registro, processo, UI.
- [ ] Strings de UI (menus, tooltips, erros) com encoding correto.
- [ ] RTTI/nomes de classe e vtables.
- [ ] Recursos: ícones, manifest, versão, diálogos.

## Dependências

- [ ] Quem importa este módulo dentro do pacote.
- [ ] Quais DLLs de terceiros ele usa (Qt, OpenCV, ICU, Xerces, OpenImageIO).
- [ ] Módulo é Esko próprio ou wrapper de terceiro.
- [ ] `DEPENDENCY_MAP.md` atualizado.

## Alvos para Ghidra

- [ ] Funções exportadas e handlers de UI.
- [ ] Endereços já conhecidos (com ID `GHIDRA-*`).
- [ ] O que **não** precisa ser analisado (biblioteca de terceiro).

## Diff v240 × v250

- [ ] Imports/exports adicionados/removidos.
- [ ] Strings de UI alteradas.
- [ ] Tamanho e hash por módulo.
- [ ] Mudança de comportamento só com captura, não por diff de string.

## Saída

- `docs/reverse-engineering/<Modulo>_TRIAGE.md`
- `rag/evidence/F-###/rea/<modulo>.md`
- JSON bruto do DIE em `rag/sources/`
