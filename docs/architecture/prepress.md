## Módulo: Prepress (SelectionMarkManager/SelectionMark)

**Responsabilidades:** Gerenciar o ciclo de vida do contexto de seleção dinâmico sobre objetos PDF existentes. Encapsular a lógica de seleção (ativação/desativação), manipulação visual (handles, arrasto) e integração com o sistema de Undo/Redo.
**Dependências:**
*   `Document Core` (para acessar `Object` e `Document` estado).
*   `UI` (para ler/escrever estado de barra de status e manipular handles visuais).
*   `Undo/Redo System` (para registrar a ação única).
**Dependências Proibidas:**
*   Qualquer módulo que exija diálogo modal (e.g., `ColorManagement` se for usada para prompts de cor).
**Fluxo de Dados:** UI (Evento `Ctrl+Shift+S`) $\rightarrow$ `SelectionMarkManager::activateSelection()` $\rightarrow$ `SelectionMark` (modifica estado do `Object`) $\rightarrow$ `Undo/Redo System`.
**Pontos de Extensão:** Adição de novos tipos de interação de seleção (e.g., "snap to grid" implícito no `SelectionMark::move`).
**O que NÃO entra:** Lógica de criação de *novos* objetos (diferente de `CutMark`), manipulação de caixas de corte (apenas **integração** com elas, conforme R4).

**Rastreabilidade:** F-004 (R1, R2, R3) e E1, E2, E3.