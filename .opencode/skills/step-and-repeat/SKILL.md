---
name: Step and Repeat
description: Implementa Step & Repeat e montagem de produção no ArtPlus — grade, espaçamento, cópias, circular, espelhado, aninhado, preview e undo. Use ao replicar objetos, montar folha ou validar repetição contra o original.
metadata:
  version: "2"
---

# Step & Repeat

Código: `src/prepress/step_repeat.h/.cpp` (`StepRepeatEngine`).
Tipos: `Settings`, `Result`, `Preview`, `CircularSettings`, `MirrorSettings`, `NestedSettings`.
Métodos: `execute`, `preview`, `createCopy`, `executeCircular`, `executeMirror`, `executeNested`.

## Regras

1. Contagem (`copiesX`, `copiesY`) e espaçamento (`spacingX`, `spacingY`) em `mm`; spacing é distância entre âncoras declaradas.
2. Cópias são objetos independentes: editar a original depois **não** altera cópias já criadas (confirme na spec).
3. `preview` não muta o documento.
4. Uma execução = um comando de undo que remove todas as cópias criadas.
5. Circular: centro, raio, ângulo inicial, passo e número de cópias explícitos.
6. Espelhado: eixo e se a cópia é reflexão real ou rotação de 180°.
7. Aninhado: prioriza encaixe sem sobreposição; se não couber, retorna resultado parcial explícito.
8. Não replicar marcas de produção junto com o conteúdo, salvo flag na spec.
9. Limite de segurança para cópias (evite milhões por engano) com erro claro.

## Evidência

Captura do diálogo (campos, default, faixa), preview e resultado no documento; PDF exportado. Sem isso, não invente regra de espaçamento.

## Testes

- 3×2 com spacing 5 mm → 6 cópias nas posições esperadas.
- Spacing 0 → cópias sobrepostas, sem erro.
- Undo → documento original exato.
- Circular 8 cópias → ângulos e centros corretos.
- Aninhado sem espaço → resultado parcial documentado.

## Limites

- Não chame `createCopy` em laço sem controle de falha.
- Não altere `src/document/` para viabilizar a cópia.
