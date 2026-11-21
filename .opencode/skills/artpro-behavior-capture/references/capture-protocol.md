# Protocolo de captura

## Identificador

`CAP-AAAAMMDD-nn` — `nn` sequencial do dia, dois dígitos.

## Pasta

```text
tools/reference_capture/F-###/
├── input/     cópia da prova (nunca o original)
├── output/    arquivo(s) gerados pelo ArtPro+
├── shots/     shot-01.png, shot-02.png ...
└── notes.md   roteiro em execução (opcional)
```

Evidência final: `rag/evidence/F-###/behavior/` com os mesmos artefatos + hash no `manifest.json`.

## Template de `docs/behavior/<Ferramenta>.md`

```markdown
## Trim Marks
- Versão: 24.03.10017
- Caminho na UI: Marks > Create > Trim Marks
- Entrada: A4, 1 camada, seleção vazia
- Passos:
  1. Marks > Create > Trim Marks
  2. Offset = 3 mm
  3. OK
- Campos do diálogo:
  | Campo | Tipo | Default | Unidade | Faixa |
  |---|---|---|---|---|
  | Offset | numérico | 3 | mm | 0–20 |
- Saída observada: 4 marcas nos cantos, fora do trim
- Erros e limites: "Offset must be positive" com valor negativo
- Capturas: shots/shot-01.png ... shot-05.png
- Evidência: CAP-20261118-03
```

## Checklist de completude

- [ ] Estado inicial registrado (versão, arquivo, perfil, unidades, zoom).
- [ ] Todos os passos, inclusive diálogos intermediários.
- [ ] Texto literal de menu, botão, tooltip e erro.
- [ ] Campos com default, unidade e faixa.
- [ ] Pelo menos uma variação de borda (0, negativo, vazio, unidade alternativa).
- [ ] Saída capturada (arquivo, PDF, valores).
- [ ] Screenshot antes e depois.
- [ ] Hash do input e do output no manifest.
- [ ] Bloqueios registrados com motivo e imagem.

## O que invalida a captura

- Abrir o arquivo original e salvar por cima.
- Declarar comportamento sem screenshot/arquivo de saída.
- Duas sessões de GUI simultâneas.
- Captura com escala/resolução diferente no meio do fluxo.
- “Achei que era assim” sem passo executado.
