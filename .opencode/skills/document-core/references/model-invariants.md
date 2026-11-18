# Invariantes do modelo de documento

## Ownership

```
Document
├── pages: map<PageId, Page> + pageOrder
├── layers: map<LayerId, Layer> + layerOrder
├── objects: map<ObjectId, DocumentObject>
└── selection: vector<ObjectId>
```

- `Document` é dono de tudo via `shared_ptr`.
- `Layer::objects` guarda IDs na ordem de empilhamento.
- `DocumentObject::children` guarda IDs, não ponteiros.
- `parentId` e `children` precisam ser consistentes nos dois sentidos.

## Operações atômicas

| Operação | Efeito |
|---|---|
| `addObject` | insere no mapa, define `layerId`, adiciona à camada |
| `removeObject` | remove da camada, do pai e das máscaras que o referenciam |
| `addLayer` / `removeLayer` | atualiza ordem; remover exige realocar ou remover objetos |
| `addPage` / `removePage` | atualiza ordem; remover exige tratar camadas/objetos da página |
| `selectObject` | só altera seleção; não muda `modified` |

## Integridade

- IDs não são reciclados.
- Nenhum objeto aponta para ID inexistente após a operação.
- `softMaskId` e `clipMaskIds` referenciam objetos do mesmo documento.
- `clone()` é profundo para conteúdo e não compartilha `QImage` mutável.
- `boundingBox()` considera transformação e filhos quando aplicável.

## Undo

- Comando guarda IDs e snapshots suficientes para refazer/desfazer.
- `undo` seguido de `redo` retorna ao estado anterior byte a byte (exceto ordem de mapa, que não é observável).
- Um gesto = um comando.

## Serialização

- `format_version` explícito.
- Campo novo com default.
- Não serializar estado derivado (bounding box, separações, preview).
- Roundtrip testado com tolerância.

## Checklist de alteração

- [ ] Novo campo tem unidade no nome.
- [ ] Default retrocompatível.
- [ ] Clone atualizado.
- [ ] Serialização atualizada.
- [ ] Undo cobre a operação.
- [ ] Teste de borda (órfão, vazio, duplicado).
- [ ] ADR se mudou contrato público.
