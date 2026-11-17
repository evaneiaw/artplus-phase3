# DOCUMENT_DLL_FUNCTION_ANALYSIS.md

## 1. Mapeamento de Áreas Funcionais (Document.dll)

Com base na análise estática do binário `Document.dll`, as seguintes áreas funcionais foram identificadas e mapeadas por endereços e símbolos:

### 1.1. Gerenciamento de Documento e Página
| Funcionalidade | Endereço (VA) | Símbolo / Descrição | Confiança |
| :--- | :--- | :--- | :--- |
| Criar Documento | `180015b70` | `Factory` (Document Creator) | Alta |
| Gerenciar Páginas | `1800156d0` | `PageRanges` | Alta |
| Inserir Página | `EXTERNAL:00000389`| `InsertPage` (Import via PDFLib) | Crítica |
| Salvar Documento | `EXTERNAL:0000030c`| `SaveContent` | Alta |
| Notificação de Mudança | `EXTERNAL:0000010c`| `SendDocumentChangedNotification` | Alta |

### 1.2. Hierarquia de Objetos e Geometria
| Funcionalidade | Endereço (VA) | Símbolo / Descrição | Confiança |
| :--- | :--- | :--- | :--- |
| Objeto de Caminho (Path) | `EXTERNAL:00000226`| `GOPath` (Graphics Object Path) | Crítica |
| Objeto de Texto (Text) | `EXTERNAL:00000231`| `GOText` | Alta |
| Clipping Mask | `EXTERNAL:00000210`| `GOClipPath` | Alta |
| Transformação | `EXTERNAL:00000266`| `SetMatrix` | Alta |
| Hierarquia (Parent) | `EXTERNAL:000003cf`| `SetParentContent` | Alta |

### 1.3. Cores e Separações
| Funcionalidade | Endereço (VA) | Símbolo / Descrição | Confiança |
| :--- | :--- | :--- | :--- |
| Espaço de Cor CMYK | `1800210d0` | `CmykConverter` | Alta |
| Gestão de Separação | `180012b20` | `IsSeparable` | Alta |
| Perfil ICC | `18002da50` | `ICCProfile` | Alta |
| Conversão de Cor | `1800271c0` | `ColorConverter` | Alta |

### 1.4. Metadados e XMP (Proprietário Esko)
| Funcionalidade | Endereço (VA) | Símbolo / Descrição | Confiança |
| :--- | :--- | :--- | :--- |
| Metadados XMP | `EXTERNAL:00000336`| `GetMetadata` | Alta |
| Versão Nativa PDF+ | `EXTERNAL:00000096`| `SetPDFNativeVersion` | Crítica |
| PieceInfo (Metadados) | `EXTERNAL:000002e9`| `RemovePieceInfo` | Alta |

## 2. Pontos de Entrada e Ciclo de Vida
O sistema utiliza um padrão de **Factory** e **Visitor** extensivo para processar a árvore do documento.
- **Entry Points de Inicialização**: `EXTERNAL:00000078` (`Initialize`) e `EXTERNAL:00000366` (`Initialize` para subsistemas de erro).
- **Cadeia de Comandos**: Implementada via classes `Action` (`180039b20`) e `ActionList` (`18003ed10`), suportando Undo/Redo nativamente no motor de documento.

## 3. Limitações Observadas
- Muitas funções críticas são importadas de `ArtSys.dll` e `Enfocus PDFLib`, indicando que `Document.dll` atua como um wrapper de alto nível que orquestra essas bibliotecas de baixo nível.
- O padrão `GO*` (Graphics Object) sugere uma abstração própria da Esko sobre as entidades do PDF.
