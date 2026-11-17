# Undo/Redo & Actions - Document.dll

## Overview
Gerenciamento de ações do usuário, grupos de ações, histórico de undo/redo e rastreamento de mudanças.

## Namespaces/Classes Principais

### ArtDoc::ActionList
- **Ctor**: `ActionList(2 params)` - `18003ed10`
- **Dtor**: `~ActionList` - `18003f890`

#### Métodos Principais
| Método | Address | Retorno/Params |
|--------|---------|----------------|
| `AddAction(void, 2)` | `180040270` | Adiciona uma ação |
| `AddActionGroup(void, 2)` | `180040370` | Adiciona um grupo de ações |
| `BeginActionGroup(void, 3)` | `1800403c0` | Inicia um grupo de ações |
| `EndActionGroup(void, 1)` | `1800408a0` | Finaliza um grupo de ações |
| `DoAction(Action*, 2)` | `180040820` | Executa uma ação |
| `Undo(void, 2)` | `180040fc0` | Desfaz a última ação |
| `Redo(void, 2)` | `180040c40` | Refaz a última ação desfeita |
| `BeginHistory(ActionIterator, 2)` | `1800404c0` | Retorna iterador para o início do histórico |
| `Clear(void, 1)` | `1800405d0` | Limpa o histórico |
| `CommitPendingAction(void, 1)` | `1800406f0` | Confirma ações pendentes |
| `GetOutermostActionGroup()` | `180040a00` | Retorna o grupo de ações mais externo |
| `GetPendingAction()` | `180040a20` | Retorna ações pendentes |
| `PerformDependentActions()` | `180040ad0` | Executa ações dependentes |
| `SetActiveActionGroup()` | `180040d70` | Define o grupo de ações ativo |
| `UpdateDirtyState()` | `180041280` | Atualiza o estado de 'dirty' |

### ArtDoc::Action & ArtDoc::ActionGroup
- **Action Ctor**: `Action(3 params)` - `180039b20`
- **Action Dtor**: `~Action` - `18003a1f0`
- **ActionGroup Ctor**: `ActionGroup(3 params)` - `18003bb50`
- **ActionGroup Dtor**: `~ActionGroup` - `18003c250`

#### Métodos da Ação
| Método | Address | Retorno/Params |
|--------|---------|----------------|
| `DoAction(Action*, 1)` | `18003a550` | Executa ação (auto-referência) |
| `RedoAction(void, 1)` | `18003a800` | Refaz ação (auto-referência) |
| `UndoAction(void, 1)` | `18003a840` | Desfaz ação (auto-referência) |
| `GetDescription()` | `18003a630` | Descrição da ação |
| `IsActionDone()` | `18003a7e0` | Verifica se a ação foi concluída |

### ArtDoc::ChangeTracker & ArtDoc::ErrorRecorder
- **ChangeTracker Ctor**: `ChangeTracker(2 params)` - `18004bf50`
- **ChangeTracker Dtor**: `~ChangeTracker` - `18004dc40`
- **ErrorRecorder Ctor**: `ErrorRecorder(2 params)` - `18003b0a0`
- **ErrorRecorder Dtor**: `~ErrorRecorder` - `18003b380`

#### Métodos do ChangeTracker
| Método | Address | Retorno/Params |
|--------|---------|----------------|
| `AddError(void, 2)` | `18004ecf0` | Adiciona um erro |
| `AddErrorForObject(void, 2)` | `18004edc0` | Adiciona erro para um objeto |
| `GetErrorsForObject()` | `18004f040` | Retorna erros de um objeto |
| `NotifyChanges(void, 1)` | `18004f480` | Notifica mudanças |
| `RemoveError(void, 2)` | `18004f5d0` | Remove um erro |
| `UpdateErrors(void, 3)` | `18004f760` | Atualiza erros |

#### Métodos do ErrorRecorder
| Método | Address | Retorno/Params |
|--------|---------|----------------|
| `AddError(void, 2)` | `18003b3a0` | Adiciona um erro |
| `Playback(void, 1)` | `18003b420` | Reproduz erros |
| `RemoveError(void, 2)` | `18003b490` | Remove um erro |
| `Rewind(void, 1)` | `18003b510` | Reinicia a gravação de erros |

### Erros e Mensagens (Strings)
```
ACTION CANCELLED
ACTION UNKNOWN ERROR
Action::Error (use GetDescription() for more details on the error)
Doc.Core.Errors
```

### Evidence: Cross-references
- `StatusReporting` - interface para reportar status e erros.
- `ErrorLogger` - logging de erros.

---
*Generated from: functions.csv, strings.csv, symbols.csv*