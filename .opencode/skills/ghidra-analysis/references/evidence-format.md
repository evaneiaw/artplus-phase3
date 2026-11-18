# Formato de evidência Ghidra

## Identificador

`GHIDRA-<Modulo>-<endereco>`

Exemplo: `GHIDRA-DocumentMarks-0x18001A2C0`.

- `<Modulo>` = nome do arquivo sem extensão (`DocumentMarks`).
- `<endereco>` = VA com `0x`, minúsculo, sem zeros extras.
- O mesmo endereço não pode ter dois IDs.

## Arquivo

`rag/evidence/F-###/ghidra/<modulo>__<endereco>.md`

```markdown
---
modulo: DocumentMarks.dll
versao: 25.0
funcao: 0x18001A2C0
nome: sub_18001A2C0
confianca: media
xrefs: [0x18001B100, 0x1800204A0]
strings: ["Trim Marks", "Offset"]
---

# O que a função faz

<descrição em 3–6 linhas, separando observado de inferido>

## Evidência

- Decompilado: `rag/evidence/F-025/ghidra/DocumentMarks__0x18001A2C0.json`
- Xrefs de entrada: ...
- Chamadas: ...
- Strings vizinhas: ...

## Inferência

> Inferência: ... (fonte: <evidência>)

## Confiança

`media` — motivo objetivo.
```

## Regras

- `confianca` alta só com xrefs coerentes, strings de UI e comportamento confirmado.
- Nome `sub_*` é o nome atual do Ghidra; não invente nome de classe.
- Endereço de imagem x64 começa em `0x180000000` na maioria dos módulos; confirme no import.
- JSON bruto do REA/ExportFunctions fica ao lado, não colado no Markdown.
- Atualize `rag/evidence/F-###/manifest.json` → `estatico` com o ID.
- Não edite `INVENTORY.md`; isso é do orquestrador.

## Exemplo de manifest

```json
{
  "id": "F-025",
  "versao_ref": "25.0",
  "binario": "C:\\Esko\\bg_prog_artpro+_v250\\ArtPro+.app\\Contents\\Windows\\DocumentMarks.dll",
  "sha256": "...",
  "estatico": ["GHIDRA-DocumentMarks-0x18001A2C0"],
  "comportamental": ["CAP-20261118-03"],
  "bloqueios": []
}
```
