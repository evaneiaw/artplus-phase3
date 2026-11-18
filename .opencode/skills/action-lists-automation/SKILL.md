---
name: Action Lists Automation
description: Implementa Action Lists e execução em lote no ArtPlus — gravação, replay determinístico, serialização, contexto de execução e tratamento de falha. Use ao criar, salvar, carregar ou reproduzir uma lista de ações sobre o documento.
metadata:
  version: "2"
---

# Action Lists e automação

Código: `src/automation/action_list.h/.cpp`.

## Modelo atual

`ActionListEngine` oferece `startRecording`, `stopRecording`, `recordedList`, `play`, `playAction`, `saveList` e `loadList`.
Tipos: `ActionType`, `Action`, `ActionList`, `ExecutionContext`, `Result`/`ExecutionResult`.

## Regras

1. **Replay determinístico**: mesma lista + mesmo documento = mesmo resultado. Nada de estado global escondido.
2. **Contexto explícito**: documento, página e seleção viajam em `ExecutionContext`; não leia estado da UI.
3. **Falha não é silêncio**: `Result` informa ação, índice e motivo. Falha no meio para ou continua conforme política declarada na spec.
4. **Serialização versionada**: lista salva tem `format_version`; campo novo com default retrocompatível.
5. **IDs estáveis durante o replay**: ação referencia objeto por ID; se o objeto não existe, erro explícito.
6. **Undo**: uma execução de lista é uma transação; agrupe comandos para um único undo quando a spec exigir.
7. **Batch**: processar N documentos não pode vazar memória nem reter `Document` após o item.
8. **Sem código arbitrário**: Action List não executa script do usuário; scripting é módulo separado (`scripting.*`).

## Evidência obrigatória

Captura do ArtPro+ mostrando: menu de gravação, lista gerada, diálogo de reprodução, resultado e mensagem de erro. Sem isso, não invente formato de arquivo nem semântica de ação.

## Testes

- Gravar 3 ações → salvar → carregar → reproduzir → comparar documento.
- Ação com objeto removido → erro esperado.
- Lista vazia → no-op explícito.
- Duas execuções seguidas → mesmo resultado (idempotência quando a spec exigir).

## Limites

- Não edite `src/document/` para facilitar a lista; proponha ADR.
- Não invente `ActionType` que o original não tem.
