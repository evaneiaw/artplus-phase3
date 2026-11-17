# AGENTS.md

Este documento define a estrutura e responsabilidades dos agentes especializados do projeto ArtPlus.

## Agentes

*   **architect**: Arquitetura geral, módulos e contratos de interface.
*   **reverse-engineer**: Investigação do comportamento observável do ArtPro+.
*   **pdf-engine**: Engine de PDF, objetos, fontes e exportação.
*   **geometry-engine**: Geometria, caminhos, transformações e operações booleanas.
*   **prepress-engine**: Pré-impressão, separações, trapping, retículas e preflight.
*   **production-engine**: Step & Repeat, marcas, estruturas e automação.
*   **ui-specialist**: Interface gráfica (Qt), ferramentas e painéis.
*   **qa-engineer**: Testes, regressão e comparação visual.
*   **security-license-auditor**: Auditoria de licenças, dependências e integridade.

## Estratégia de Operação

O agente principal (coordenador) distribui tarefas para os especialistas acima. Especialistas não devem alterar contratos compartilhados sem aprovação do arquiteto.
