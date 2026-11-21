# Glossário — marcas

Termos usados por `docs/features/F-001.md` e `docs/features/F-003.md`. Uma frase por termo;
sinônimos não são criados para o que o ArtPro+ já nomeia.

| Termo | Definição | Spec |
|---|---|---|
| **Cut Mark** | Marca de corte dinâmica que desenha linhas finas nos cantos de uma caixa de página, prolongadas para fora dela; é o nome usado pela UI e pelo binário. | F-001 |
| **Trim Marks** | Nome de projeto para a funcionalidade implementada pelo Cut Mark; não existe como literal no binário. | F-001 |
| **Dynamic Mark** | Objeto de documento cuja geometria é regenerada a partir de parâmetros (caixa de referência, offset, orientação) em vez de ser arte fixa. | F-001 |
| **Mark Set** | Documento/folha de marcas cuja primeira página é copiada para a página alvo ao aplicar o conjunto de marcas. | F-001 |
| **Trim Box** | Caixa da página que define a área final acabada; default de `Attach to:` do Cut Mark. | F-001 |
| **Offset (de marca)** | Deslocamento, em milímetros, aplicado à marca em relação à caixa de referência; aceita valor numérico ou fórmula (`fx`). | F-001 |
| **Attach to** | Campo da UI que escolhe a referência de posicionamento da marca: uma caixa de página ou outra marca do documento. | F-001 |
| **Bearer Bar** | Faixa de marca dinâmica desenhada encostada por fora da caixa de referência, usada para delimitar a área de produção; o termo é literal na UI e no binário. | F-003 |
| **Location (bitmask)** | Inteiro de 4 bits que seleciona os lados em que a Bearer Bar é gerada: bit0 = esquerda (`l`), bit1 = direita (`r`), bit2 = topo (`t`), bit3 = base (`b`). | F-003 |
| **SizeReference** | Enumeração do PDF+ (0 = não definido, 1–8) que nomeia qual grid/coluna/linha do layout define o comprimento da Bearer Bar; 1–4 exigem `Direction` vertical, 5–8 horizontal. | F-003 |
| **Miliponto (mp)** | Unidade interna do documento de marcas: 1/1000 pt; `1 mm = 2834,6456692913 mp`. | F-001, F-003 |
| **Offset de Bearer Bar** | Afastamento, em milímetros, entre a caixa de referência e a faixa; é unidimensional — a componente vertical é proibida pelo formato. | F-003 |
