# DOCUMENT_WORKFLOWS.md

## 1. Fluxos de Trabalho Identificados (Document.dll)

Com base na análise de símbolos, strings e o grafo de chamadas, os seguintes fluxos de trabalho principais foram mapeados. Estes fluxos representam como o ArtPro+ orquestra operações complexas no documento.

### 1.1. Fluxo de Criação e Abertura de Documento
1.  **Inicialização**: O subsistema de erros e progresso é inicializado (`ErrorReporterScopeGuard`, `ProgressInitialize`).
2.  **Factory**: Uma instância de documento é criada via `Factory` em `180015b70`.
3.  **Importação PDF**: Se estiver abrindo um arquivo, o `PDFLib` é utilizado para carregar o conteúdo (`InsertPage`, `GetXML`, `GetMetadata`).
4.  **Processamento de Metadados**: O namespace XMP da Esko é lido (`http://ns.esko-graphics.com/pdfnatversion/1.0/`) para determinar se o arquivo é um PDF \"Normalizado\" (PDF+).
5.  **Construção da Árvore**: A hierarquia de objetos (`GOObject`) e camadas (`GetLayers`) é construída em memória.

### 1.2. Fluxo de Edição de Objetos Geométricos (Paths)
1.  **Seleção/Localização**: O objeto `GOPath` alvo é identificado.
2.  **Transformação**: Se houver alteração de posição, escala ou rotação, a matriz de transformação do objeto é atualizada (`SetMatrix`).
3.  **Manipulação de Geometria**: O motor de geometria (`TIntersectionEngine`, `CCurve`) processa alterações nos pontos e curvas do caminho.
4.  **Atualização do Bounding Box**: A caixa delimitadora (Bounding Box) do objeto e do grupo pai é recalculada.
5.  **Notificação**: Uma notificação de mudança no documento é disparada (`SendDocumentChangedNotification`).

### 1.3. Fluxo de Gerenciamento de Cores e Separações
1.  **Definição de Cor**: Uma `ArtColor::ColorSpec` é definida para um objeto.
2.  **Conversão de Cor**: Se necessário, o motor de cores (`ColorConverter`) converte a cor entre espaços (ex: RGB para CMYK) usando perfis ICC.
3.  **Identificação de Separação**: O sistema verifica se a cor é separável (`IsSeparable`) e se corresponde a uma tinta spot (Spot Color) existente.
4.  **Gerenciamento de Inks**: Tintas são registradas e gerenciadas via `TInk_GetLabValue` e `TIDB_GetInk`.
5.  **BlendModes e Transparência**: O modo de mesclagem (ex: Multiply, Overlay) e os níveis de opacidade são aplicados durante o rendering ou exportação.

### 1.4. Fluxo de Salvamento e Exportação
1.  **Preparação de Conteúdo**: O estado atual do documento é consolidado (`SaveContent`).
2.  **Atualização de XMP**: Os metadados proprietários são atualizados no documento (`SetPDFNativeVersion`, `SetLayers`).
3.  **Geração de PDF**: O `PDFLib` é instruído a gravar as estruturas de dados de volta no formato PDF, preservando a fidelidade dos objetos e metadados.
4.  **Finalização**: Recursos temporários são liberados e o estado de \"limpo\" (não modificado) é restaurado.

## 2. Padrões Arquiteturais Dominantes
- **Visitor Pattern**: Utilizado extensivamente para percorrer a árvore do documento (métodos `Accept`).
- **Smart Pointers (`GSharedObject`)**: Essenciais para o gerenciamento de tempo de vida de objetos compartilhados em toda a hierarquia.
- **Undo/Redo (Command Pattern)**: Implementado via classes `Action` e `ActionList`, permitindo a reversão de qualquer etapa dos fluxos acima.

## 3. Implicações para a Implementação ArtPlus
Os fluxos de trabalho identificados sugerem que o ArtPlus deve focar em uma API de alto nível que esconda a complexidade da manipulação direta do PDF. A replicação desses fluxos garantirá compatibilidade comportamental com o ArtPro+, permitindo que o ArtPlus atue como um editor de PDF+ robusto e independente.
