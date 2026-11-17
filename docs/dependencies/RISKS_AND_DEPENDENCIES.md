# RISKS_AND_DEPENDENCIES.md

## Relatório de Riscos e Dependências

### Riscos Identificados
1. **Complexidade do Formato PDF**: A manipulação de PDF/PDF+ exige precisão total para não corromper documentos de produção. O risco de corrupção de dados é alto.
2. **Dependência de Licenciamento de Terceiros**: Bibliotecas escolhidas podem ter licenças restritivas ou incompatíveis com o objetivo de produto independente.
3. **Complexidade de Engenharia Reversa**: Interpretar comportamentos complexos da Esko apenas por observação pode levar a implementações ligeiramente diferentes, impactando usuários acostumados com o original.
4. **Manutenção de longo prazo**: O custo de manter uma engine própria (PDF, Geometria, Pré-impressão) é significativo.

### Dependências Técnicas (Avaliação Inicial)
- **Qt 6.8.3**: Base da interface e utilitários.
- **PDFium**: Engine principal de leitura e renderização.
- **Clipper2/Boost.Geometry**: Base para operações de pré-impressão e geometria.
- **SQLite**: Armazenamento para banco de conhecimento e estados.

### Mitigação
- Testes automatizados rigorosos (PDF-to-PDF).
- Auditoria de licenças recorrente.
- Foco inicial em "leitura e preservação" antes de "escrita e edição".
- Modularização estrita para isolar dependências.
