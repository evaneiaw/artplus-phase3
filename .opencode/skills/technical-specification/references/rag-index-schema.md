# Índice RAG: schema e validação

## `rag/index/features.jsonl`

Uma linha por spec:

```json
{"id":"F-025","modulo":"DocumentMarks.dll","ui":"Marks > Create > Trim Marks","status":"spec","evidencia":["GHIDRA-DocumentMarks-0x18001A2C0","CAP-20261118-03"],"tags":["marks","trim"]}
```

Campos obrigatórios: `id`, `modulo`, `ui`, `status`, `evidencia`, `tags`.

## `rag/index/functions.jsonl`

```json
{"modulo":"DocumentMarks.dll","endereco":"0x18001A2C0","nome":"sub_18001A2C0","spec":"F-025","confianca":"media"}
```

Campos obrigatórios: `modulo`, `endereco`, `nome`, `spec`, `confianca`.

## `rag/metadata/schema.json`

Define os dois arquivos acima e `schema_version`. Se mudar campo, incremente a versão e migre o índice.

## `rag/metadata/coverage.json`

Contagem por estado, coerente com `INVENTORY.md`:

```json
{"schema_version":1,"atualizado":"2026-11-18","por_estado":{"nao-tratado":280,"triado":5,"analisado":2,"comportamento":1,"spec":1,"arquitetado":0,"implementado":0,"testado":0,"auditado":0}}
```

## Validação

```powershell
python -c "import json,io; [json.loads(l) for l in io.open(r'H:\Desktop\ArtPlus\rag\index\features.jsonl',encoding='utf-8') if l.strip()]; print('features ok')"
python -c "import json,io; [json.loads(l) for l in io.open(r'H:\Desktop\ArtPlus\rag\index\functions.jsonl',encoding='utf-8') if l.strip()]; print('functions ok')"
python -c "import json,jsonschema; s=json.load(open(r'H:\Desktop\ArtPlus\rag\metadata\schema.json',encoding='utf-8')); print(s.get('title'))"
```

## Regras

- UTF-8 sem BOM; uma linha = um objeto; sem vírgula final.
- `id` de spec é único.
- Endereço de função normalizado (`0x` + hex minúsculo).
- `spec` pode ser vazio enquanto a função não foi associada, mas `confianca` é obrigatória.
- Não duplique a mesma função em duas linhas.
