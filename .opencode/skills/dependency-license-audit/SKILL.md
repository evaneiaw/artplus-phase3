---
name: Dependency License Audit
description: Inventaria dependências e licenças do ArtPlus — Qt, OpenCV, ICU, Xerces, OpenImageIO, zlib/freetype — e verifica compatibilidade antes de adicionar biblioteca. Use ao propor dependência, revisar CMake find_package, preparar THIRD_PARTY_NOTICES ou responder risco de licença.
metadata:
  version: "2"
---

# Dependências e licenças

## Inventário

Fonte: `CMakeLists.txt` (`find_package`, `target_link_libraries`), DLLs em `C:\Esko\...\Contents\Windows` e `docs/dependencies/`.

| Dependência | Uso no ArtPlus | Licença típica |
|---|---|---|
| Qt 6.8.3 | UI, Core, Gui, Widgets, Qml | LGPLv3 ou comercial |
| Qt ZlibPrivate | compressão Flate no PDF | zlib |
| OpenCV | análise de imagem (referência; não linkado no produto) | Apache 2.0 |
| ICU | Unicode (referência do original) | Unicode License |
| Xerces-C | XML (referência do original) | Apache 2.0 |
| OpenImageIO | imagem (referência do original) | Apache 2.0 / BSD |
| FreeType | fontes | FTL / GPLv2 |

Confirme versão e licença no arquivo real do pacote antes de afirmar. Esta tabela é ponto de partida, não laudo.

## Procedimento

1. Liste dependências diretas do build e as transitivas relevantes.
2. Para cada uma: nome, versão, licença, origem, uso, se é linkada ou só ferramenta.
3. Verifique obrigações: aviso de copyright, oferta de fonte (LGPL), cópia de licença, patentes.
4. Registre em `docs/dependencies/THIRD_PARTY_NOTICES.md` e `RISKS_AND_DEPENDENCIES.md`.
5. Dependência nova: exige ADR **e** esta auditoria antes do merge.

## Regras

- Não adicione `find_package` sem ADR que autorize.
- Não copie código de dependência para dentro de `src/` — muda a licença do arquivo.
- LGPL dinâmica é diferente de estática; declare o modo de link.
- Binário de `C:\Esko` é referência de análise, não dependência do ArtPlus.
- Não dê parecer jurídico: registre fato, obrigação e risco; decisão final é do proprietário.

## Saída

```
DEPENDÊNCIA: <nome> <versão>
LICENÇA: <nome> (<arquivo>)
USO: <linkada|ferramenta|referência>
OBRIGAÇÃO: <aviso|fonte|none>
RISCO: <baixo|médio|alto> — <motivo>
```
