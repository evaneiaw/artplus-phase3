# ADR-013 — Abertura de Documento PDF

## Contexto
Spec F-005; funcionalidade de abertura de documentos PDF no ArtPlus, envolvendo `Document.dll`, `PDFAccessLayer.dll` e `PDFObjectLayer.dll`.

## Decisão
A abertura de PDF será orquestrada pela camada de UI, que invocará `PDFAccessLayer` para extrair a estrutura de caixas de página (MediaBox, ArtBox, etc.), e em seguida, `PDFObjectLayer` para mapear os objetos visuais internos para as estruturas de dados do `ArtDoc`. Nenhum serviço de parsing de PDF será implementado do zero; a funcionalidade será construída sobre as interfaces binárias existentes.

## Mapeamento binário → ArtPlus
| Função (Ghidra) | Classe/método ArtPlus | Observação |
|---|---|---|
| (Funções de leitura de Box) | `PDFAccessLayer::readPageBoxes()` | Extrai MediaBox, CropBox, BleedBox, TrimBox, ArtBox em mm. |
| (Funções de leitura de Objeto) | `PDFObjectLayer::parseObjectStream()` | Itera sobre `PDFObject` e mapeia para `Path`/`Text`/`Group` de `Document.dll`. |
| (Função principal de abertura) | `ArtDoc::loadFromPdf(const std::string& path)` | Orquestra a leitura, instancia `ArtDoc` e registra `WhatChangedFlags`. |

## Contrato de interface
A interface de abertura será exposta como um método de serviço, acessível pela camada de UI:
`std::optional<ArtDoc> ArtDoc::loadFromPdf(const std::filesystem::path& filePath, const ErrorCodes& contextErrors);`

## Alternativas descartadas
1. Implementar parser PDF completo: Descartada devido à evidência REA forte das DLLs existentes e princípio de reutilização.
2. Usar bibliotecas externas (ex: Poppler): Descartada por política de *clone funcional* e dependência no ecossistema Esko validado.

## Consequências
*   A camada de UI deve integrar o diálogo de seleção de arquivo padrão e o menu `File > Open Document`.
*   Tratamento de erros deve ser centralizado no `ArtDoc` (e não apenas na UI), mapeando falhas de parsing para `ErrorCodes` definidos em `Document.dll`.

## Estado
proposto