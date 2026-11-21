# ADR-005 — Open Document (PDF)

## Contexto
Spec F-005; funcionalidade File > Open Document, que utiliza `PDFAccessLayer.dll` e `PDFObjectLayer.dll` para instanciar o modelo `ArtDoc` (Document.dll).

## Decisão
A responsabilidade de abrir e parsear documentos PDF reside no módulo `DocumentCore`, centralizado na classe `ArtDoc`. As camadas de acesso externo (`PDFAccessLayer`, `PDFObjectLayer`) são tratadas como dependências de infraestrutura de I/O, e não como parte do modelo de domínio.

## Mapeamento binário → ArtPlus
| Função (Ghidra/Inferido) | Classe/método ArtPlus | Observação |
|---|---|---|
| `[ADDR_PDFAccess_Load]` | `ArtDoc::load(const std::string& path)` | Utiliza `PDFAccessLayer` para extrair caixas de página (mm). |
| `[ADDR_PDFObject_Iterate]` | `ArtDoc::parsePage(Page& p)` | Itera objetos via `PDFObjectLayer` para construir `ContentObject`s. |
| `[ADDR_Content_Parse]` | `ContentObject::parse(PDFStreamData)` | Mapeia comandos PDF (`/Path`, `/Text`) para `Path`, `Text`, `Group`. |
| `[ADDR_State_Update]` | `ArtDoc::applyChanges(const ChangeCollector&)` | Registra `WhatChangedFlags` usando lógica de `Document.dll`. |

## Contrato de interface
```cpp
// DocumentCore/ArtDoc.h
class ArtDoc {
public:
    // Estrutura de Parâmetros de Abertura
    struct OpenParams { std::string filePath; double version = 25.0; };

    // Ponto de entrada principal
    bool load(const OpenParams& params);

    // Acesso aos componentes de domínio
    const LayerProperties& getLayerProperties() const;
    const std::vector<ContentObject>& getContentObjects() const;
    // ... métodos para acessar Group, Text, Path, etc.

private:
    // Métodos de parsing (internos, dependentes de PDFAccessLayer/PDFObjectLayer)
    void parsePage(Page& p) const;
};

// DocumentCore/ContentObject.h
class ContentObject {
public:
    enum ObjectType { TEXT, PATH, GROUP, IMAGE };
    ObjectType getType() const;
    // Métodos específicos de transformação/propriedades
    void setTransform(const QTransform& t);
};
```

## Dependências
- **Requer:** `PDFAccessLayer` (para caixas de página, metadados PDF).
- **Requer:** `PDFObjectLayer` (para iteração de objetos no stream de conteúdo).
- **Não Requer:** `DocumentMarks` (esta funcionalidade é posterior à abertura).

## Estrutura de módulos C++/Qt 6.8.3
- **Módulo:** `DocumentCore` (Responsável pelo modelo `ArtDoc`, `ContentObject`, `LayerProperties`, etc.).
- **Dependências:** `PDFAccessLayer` (Infra I/O), `PDFObjectLayer` (Infra I/O).
- **Fluxo de Dados:** UI (via `File > Open`) $\rightarrow$ `ArtDoc::load` $\rightarrow$ `PDFAccessLayer` $\rightarrow$ Stream PDF $\rightarrow$ `PDFObjectLayer` $\rightarrow$ Objeto/Estrutura Interna (`Path`, `Text`, `Group`) $\rightarrow$ `ArtDoc` (Estado).
- **Pontos de Extensão:** Implementação de *parsers* de comandos PDF (`/Path`, `/Text`) deve ser extensível para novos formatos PDF, respeitando a unidade `mm` conforme exigido por `PDFAccessLayer`.
- **O que NÃO entra:** Lógica de *rendering* visual (pertence ao módulo de Visualização, não DocumentCore).

## Estado
proposto