# ADR-004 — Selection Mark

## Context
Spec F-004; função estática em `PDFObjectActions.dll` (`GOVSelectOverlappingObjects`, `GOVSelectOverlapping`).

## Decisão
Implementação de `SelectionMark` e `SelectionMarkManager` em `src/prepress/` para gerenciar o contexto de seleção dinâmico sobre objetos PDF existentes, sem diálogos modais, aderindo a R1, R2 e R3.

## Mapeamento binário → ArtPlus
| Função (Ghidra/REA) | Classe/método ArtPlus | Observação |
|---|---|---|
| `GOVSelectOverlappingObjects` | `SelectionMarkManager::activateSelection(Object&)` | Ativa o contexto de seleção sobre o objeto ativo. |
| `GOVSelectOverlapping` | `SelectionMarkManager::updateSelection(Object&, SelectionState)` | Atualiza handles e estado visual do objeto selecionado. |
| Funções de movimento/remoção | `SelectionMark::move(Vector)` / `SelectionMark::remove()` | Métodos encapsulados no objeto de marca. |

## Contrato de interface
```cpp
class SelectionMark {
public:
    // Representa o estado de seleção (ativa/inativa)
    enum class State { Inactive, Active };
    
    SelectionMark(Object& target);
    // Retorna true se a seleção foi ativada com sucesso (evitando E1)
    bool activate() const; 
    void updateHandles() const;
    void move(const QPointF& delta) const;
    void remove() const;
    State getState() const { return state_; }

private:
    Object& target_;
    State state_{State::Inactive};
    // ... outros dados de manipulação
};

class SelectionMarkManager {
public:
    // Inicia o contexto, sem modal (R1)
    static bool activateSelection(Object& activeObject); 
    
    // Processa a ação de arrastar ou modificar handles
    static void handleObjectInteraction(Object& obj, InteractionType type);

    // Responsável por registrar a ação única no undo/redo (R2)
    static void registerUndoAction(const SelectionMark& mark);
};
```

## Alternativas descartadas
1.  **Diálogo Modal:** Descartado por violar R1 (seleção direta, sem diálogo modal).
2.  **Marca Persistente:** Descartado, pois a Selection Mark opera sobre o **contexto de seleção** do objeto existente, não como um objeto de conteúdo novo.

## Consequências
*   **Afetados:** `src/prepress/selection_mark.*`, `src/ui/main_window.cpp` (handler de atalho/toolbar).
*   **Risco:** Baixo, pois a v1 já está implementada (`src/prepress/selection_mark.*`) e esta ADR formaliza o contrato.
*   **Migração de dados:** Nenhuma, pois a marca opera sobre o estado do objeto no `Document` (persiste via metadata, conforme seção 90).

## Estado
implementado