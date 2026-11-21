# F-005 — Arquitetura da Abertura de Documento PDF

## Visão geral
Este documento descreve a arquitetura para a funcionalidade `File > Open Document` (Ctrl+O) conforme especificada em `F-005`. O objetivo é instanciar o modelo central `ArtDoc` lendo um arquivo PDF através das camadas `PDFAccessLayer.dll` e `PDFObjectLayer.dll`, aproveitando componentes já existentes e validados por evidência REA.

## Interface principal e menu File/Open
A UI principal exporá o item de menu **File > Open Document** e o atalho **Ctrl+O**.
*   O diálogo de seleção utilizará o framework Qt padrão (`QFileDialog`) com filtro `*.pdf`.
*   A ação do menu está ligada ao orquestrador `OpenDocumentCommand`, que valida a pré-condição de disponibilidade das DLLs (`Document.dll`, `PDFAccessLayer.dll`, `PDFObjectLayer.dll`) antes de iniciar o fluxo.

## Fluxo de leitura e componentes
O processo de abertura segue uma pipeline serial desacoplada, reutilizando as bibliotecas existentes:

1. **Seleção de arquivo** – A UI exibe o diálogo; ao confirmar, o caminho é passado ao comando de abertura.
2. **Camada de acesso ao PDF (`PDFAccessLayer.dll`)** – Lê a estrutura do arquivo, extraindo as caixas de página (`MediaBox`, `CropBox`, `BleedBox`, `TrimBox`, `ArtBox`). As unidades são `mm`, conforme espec F-005. Se falhar a leitura do cabeçalho PDF, o erro é propagado para o próximo estágio.
3. **Camada de objetos (`PDFObjectLayer.dll`)** – Itera sobre os *content streams* e os objetos visuais (`PDFObject`), mapeando-os para as estruturas internas `Path`, `Text`, `Group` definidas em `Document.dll`.
4. **Modelo central (`ArtDoc`)** – Instancia um objeto `ArtDoc`, registrando as propriedades de camada e os estados de mudança (`WhatChangedFlags`). O `ArtDoc` torna-se a fonte de verdade para renderização e edição subsequente.
5. **Renderização** – O visualizador consulta o `ArtDoc` para exibir o documento, usando `ArtBox` como limite de visualização padrão.

*Nota:* Não será implementado um parser PDF do zero. Toda a lógica de leitura binária e mapeamento de estruturas utiliza as interfaces exportadas pelas DLLs já presentes no ecossistema ArtPro+, conforme evidência REA.

## Gerenciamento de erros
O sistema de erros é centralizado no `ArtDoc` e segue o contrato definido em F-005:

| Cenário | Comportamento |
|---|---|
| Arquivo não encontrado ou inacessível | Retorna `std::nullopt` e registra mensagem de erro padrão do sistema. |
| Formato inválido (não PDF) | Falha na leitura do cabeçalho via `PDFAccessLayer.dll`; o erro é capturado e exibido ao usuário. |
| Documento sem caixas de página definidas | `ArtDoc` é criado, mas marcado como `ReadOnly`. O visualizador usa a área de renderização padrão. |
| Camadas ausentes | Documento aberto como único contêiner (`Layer: Base`); funcionalidade completa preservada, porém sem metadados de camada. |

## Dependências e reutilização de componentes
*   **ArtDoc** – Componente existente, responsabilidade de `Document.dll`. Reutilizado integralmente; não há nova classe de modelo.
*   **PDFAccessLayer** – Camada de acesso ao PDF. Reutilizada; método `readPageBoxes()` exposto.
*   **PDFObjectLayer** – Camada de objetos PDF. Reutilizada; método `parseObjectStream()` exposto.
*   **Document.dll** – Fornece tipos de dados (`Path`, `Group`, `Text`) e flags de mudança (`WhatChangedFlags`). Não há duplicação de lógica de parsing.

Essa arquitetura garante que a funcionalidade de abertura de PDF seja implementada com **mínimo de código novo**, aproveitando o trabalho de engenharia reversa já realizado e validando a integridade das DLLs via evidência REA.