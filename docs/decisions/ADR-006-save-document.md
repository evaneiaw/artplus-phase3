# ADR-006 — Save Document (F-006)

## Contexto

Spec `F-006` (confiança **baixa**); evidência estática em `Document.dll` (v240) e
`NDLModel.dll` (v240) — 21 funções decompiladas; captura comportamental bloqueada
(`CAP-BLOCKED-006`, menu `File` inativo na GUI v240). O formato binário proprietário
referido como `.artplus` **não** foi derivado da análise estática; a persistência já
existente no ArtPlus é `DocumentSerializer` (JSON `.artplus`, `src/document/document_serializer.h`).

Funções-chave (Ghidra):

- `GHIDRA-Document-0x180099710` — `ArtDoc::Document::Save(path, WriteOptions)` (ponto único de escrita).
- `GHIDRA-Document-0x180016730` — `WriteOptions` (ctor/copy; 12 bools em `+0x60..+0x71`, path em `+0x78`, enum `Flavour`).
- `GHIDRA-Document-0x1802ad561` — `GODocument::Save(GODocument*, GADocSaveParams*)` (gravação proprietária).
- `GHIDRA-Document-0x1802ad5eb` — `GODocument::SignOnSave`.
- `GHIDRA-NDLModel-0x180021990` — `NDLModel::SaveDocument` (wrapper → handler real, retorna `Future<vector<ActionMessage>>`).
- `GHIDRA-NDLModel-0x180042a40` / `0x180042b80` — `SaveOptions` ctor + `IsIncrementalSave`.
- `GHIDRA-NDLModel-0x18004b420` / `0x1800508d0` — `SaveMarkSet` / `SavePreflightReport` (salvamentos especializados).

O ArtPlus já possui um fluxo de salvamento mínimo em `ArtPlusMainWindow::saveArtPlus()`
(`src/ui/main_window.cpp`) que chama `DocumentSerializer::save` diretamente. A spec F-006
exige comportamento adicional: detecção `Save` vs `Save As`, salvamento incremental com
flag `NoOptimizedSave`, backup (copy/rename), criptografia (senhas owner/user em
`GADocSaveParams`) e finalização de estado (`SetDirty(false)`, `SetFilePath`,
`SwitchToWorkingFile`, `ChangeCollector::NotifyChangesCollected`).

## Decisão

1. Introduzir **`DocumentSaveManager`** em `src/document/document_save_manager.h/.cpp`, único
   orquestrador do fluxo de salvamento do ArtPlus. Ele **reusa** `DocumentSerializer`
   (não reimplementa serialização) e centraliza a lógica de decisão (Save vs Save As,
   incremental, backup, criptografia, finalização), hoje espalhada/ausente.
2. Definir os tipos de domínio `SaveFlavour`, `WriteOptions`, `SaveEncryptionParams`,
   `SavePageRange`, `SaveResult` e `SaveException` no mesmo módulo `src/document/`.
   `WriteOptions` espelha `ArtDoc::WriteOptions` (12 bools) mas com nomes explícitos e
   unidades; `SaveOptions` (NDLModel) é adaptado via construtor de `WriteOptions`.
3. Mapear `NDLModel::SaveDocument` (entrada da aplicação, assíncrona) para
   `DocumentSaveManager::saveDocument(...)`. Na v1 a execução é **síncrona** no thread
   chamador; a variante assíncrona (`saveDocumentAsync` → `QFuture<SaveResult>`) fica como
   `TODO(F-006)` para documentos grandes, com a restrição de que o `Document` não é
   thread-safe e toda mutação final ocorre no thread da UI.
4. Migrar `ArtPlusMainWindow::saveArtPlus()` para construir um `DocumentSaveManager` sobre
   `m_document` e chamar `save()`; manter o diálogo `QFileDialog` na UI (regra: regra de
   negócio fora de `src/ui/`, mas o diálogo de seleção de arquivo é UI).
5. Criptografia, salvamento incremental com base incremental (`copy_file` da working file)
   e os ramos `Flavour == 1/2` (preflight/separações) **não** são implementados de forma
   silenciosa: quando solicitados sem suporte, `save()` retorna `SaveResult{ok=false}` com
   mensagem explícita e código de erro, e registra `TODO(F-006)`. Apenas o caminho
   principal (`Flavour == NormalPdf`, sem criptografia) é funcional em v1, apoiado em
   `DocumentSerializer::save`.
6. Backup: implementado com derivação de caminho provisória (`<dest>.bak`, ver
   `TODO(F-006)` — nome real depende de `FUN_180095230`, não decompilado na spec). Regra
   binária preservada: `IsSaveAs == false && NeedBackupOnSave == true`; incremental →
   `copy_file(dest, backup)`; caso contrário → `rename(dest, backup)`.

## Mapeamento binário → ArtPlus

| Função (Ghidra) | Classe/método ArtPlus | Observação |
|---|---|---|
| `GHIDRA-Document-0x180099710` | `DocumentSaveManager::save()` | orquestra Save/Save As, incremental, backup, criptografia, finalização |
| `GHIDRA-Document-0x180016730` | `WriteOptions` (struct) | 12 bools + path + Flavour; mapeado para campos nomeados |
| `GHIDRA-Document-0x1802ad561` | `DocumentSerializer::save()` (reuse) | gravação proprietária substituída pelo serializer JSON existente |
| `GHIDRA-Document-0x1802ad5eb` | `DocumentSaveManager::applyEncryption()` (`TODO(F-006)`) | SignOnSave; sem suporte v1 (erro explícito) |
| `GHIDRA-Document-0x180099690` | `DocumentSaveManager::revertToSaved()` (`TODO(F-006)`) | RevertToSaved; não implementado v1 |
| `GHIDRA-Document-0x1800a2110` | `DocumentSaveManager` (PieceInfoWriter) | metadados de peça; `TODO(F-006)` |
| `GHIDRA-Document-0x1800a20e0` | `DocumentSaveManager` (EskoPieceInfoWriter) | idem |
| `GHIDRA-Document-0x18013f240` | `DocumentSaveManager` (WriteContext) | contexto de escrita; `TODO(F-006)` |
| `GHIDRA-Document-0x180195c10` | `DocumentSaveManager` (RegisterPieceInfoWriter) | registro; `TODO(F-006)` |
| `GHIDRA-Document-0x18023b8e0` | `DocumentSaveManager` (WriteImage) | escrita de imagem; reusa `DocumentSerializer` |
| `FUN_180095230` (helper) | `DocumentSaveManager::deriveBackupPath()` | nome de backup; `TODO(F-006)` (semântica não decompilada na spec) |
| `GHIDRA-NDLModel-0x180021990` | `DocumentSaveManager::saveDocument()` | entrada de aplicação (wrapper → handler real) |
| `GHIDRA-NDLModel-0x180042a40` | `WriteOptions::fromSaveOptions()` (adapter) | ctor SaveOptions a partir de string_view |
| `GHIDRA-NDLModel-0x180042b80` | `WriteOptions::isIncrementalSave` | flag |
| `GHIDRA-NDLModel-0x18004b420` | `DocumentSaveManager::saveMarkSet()` (`TODO(F-006)`) | salvamento de marcações (já coberto por metadata em `DocumentSerializer`) |
| `GHIDRA-NDLModel-0x1800508d0` | `DocumentSaveManager::savePreflightReport()` (`TODO(F-006)`) | relatório de preflight |

## Contrato de interface

```cpp
// src/document/document_save_manager.h
#pragma once

#include "document_model.h"
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <vector>
#include <stdexcept>

namespace ArtPlus {

enum class SaveFlavour {
    NormalPdf = 0,   // PDF normal
    Preflight = 1,   // preflight (Flavour == 1)
    Separations = 2  // separações (Flavour == 2)
};

struct SaveEncryptionParams {
    int encryptionLevel = 0;          // 0 = sem criptografia
    QString ownerPassword;            // senha owner (texto plano em memória; convertida na borda)
    QString userPassword;             // senha user
};

struct SavePageRange {
    int firstPage = 0;   // 0-based, inclusivo
    int lastPage = 0;    // 0-based, inclusivo
    bool allPages = true;
};

struct WriteOptions {
    // Flags espelhando ArtDoc::WriteOptions (+0x60..+0x71)
    bool isSaveAs = false;
    bool isSaveAsCopy = false;
    bool isSaveUnmodified = false;
    bool needBackupOnSave = false;
    bool isIncrementalSave = false;
    bool noOptimizedSave = false;     // true => desabilita otimização

    SaveFlavour flavour = SaveFlavour::NormalPdf;

    std::vector<SavePageRange> pageRanges; // vazio => todas as páginas

    SaveEncryptionParams encryption;

    // Adapter a partir de SaveOptions (NDLModel) — TODO(F-006): refinar leitura
    // do basic_string_view de origem (GHIDRA-NDLModel-0x180042a40).
    static WriteOptions fromSaveOptions(const QString& /*source*/);
};

class SaveException : public std::runtime_error {
public:
    explicit SaveException(int code, const std::string& what)
        : std::runtime_error(what), code_(code) {}
    int code() const noexcept { return code_; }
private:
    int code_;
};

struct SaveResult {
    bool ok = false;
    QString filePath;          // caminho efetivamente gravado
    QString backupPath;        // caminho do backup (vazio se não houve)
    int errorCode = 0;         // código SaveException, se aplicável
    QString errorMessage;      // mensagem para UI (português técnico)
};

class DocumentSaveManager : public QObject {
    Q_OBJECT
public:
    explicit DocumentSaveManager(Document& doc, QObject* parent = nullptr);

    // Binário: ArtDoc::Document::Save(path, WriteOptions)
    SaveResult save(const QString& destinationPath, const WriteOptions& options);

    // Binário: NDLModel::SaveDocument (entrada de aplicação)
    SaveResult saveDocument(const QString& destinationPath, const WriteOptions& options);

    // Derivação de caminho de backup (binário: FUN_180095230) — provisória
    static QString deriveBackupPath(const QString& destinationPath);

    // RevertToSaved (binário: 0x180099690) — TODO(F-006)
    bool revertToSaved();

signals:
    void progressStatus(const QString& status);   // STATUS_SAVE / STATUS_OPTIMIZING / STATUS_WRITING_TO_DISK
    void finished(const SaveResult& result);

private:
    Document& doc_;
    bool switchToWorkingFile();                  // binário: SwitchToWorkingFile
    bool applyEncryption(SaveEncryptionParams&);  // binário: Security::GetEncryption/GetPassword + SetNew* (TODO)
    void finalizeState(const QString& path, const WriteOptions& opts); // SetDirty/SetFilePath/...
};

} // namespace ArtPlus
```

## Alternativas descartadas

1. **Manter salvamento direto em `DocumentSerializer::save` na UI** — descartada: não cobre
   Save/Save As, backup, incremental, criptografia nem finalização exigidos por F-006; espalha
   regra de negócio em `src/ui/`.
2. **Implementar parser/escritor proprietário `.artplus` agora** — descartada: formato não
   derivado (confiança baixa, `CAP-BLOCKED-006`); risco de inventar formato incompatível.
   Reusa-se o serializer JSON existente e marca `TODO(F-006)` para paridade binária.
3. **Chamada assíncrona obrigatória (`QFuture`)** — descartada na v1: `Document` não é
   thread-safe e o custo de marshal supera o benefício para documentos típicos; mantém-se
   síncrono com `saveDocumentAsync` como `TODO(F-006)`.
4. **Criptografia silenciosa (escrever plaintext quando senha fornecida)** — descartada:
   violaria a regra "nada de stub silencioso"; v1 retorna erro explícito.

## Consequências

- Arquivos afetados (proposta, não editados nesta fase): `src/document/document_save_manager.h/.cpp`,
  `src/document/document_model.h` (acréscimo de campos `filePath`/`modified` já existentes — OK),
  `src/ui/main_window.h/.cpp` (migração de `saveArtPlus()`), `CMakeLists.txt` (incluir novo
  `.cpp` em `DOCUMENT_SOURCES`).
- Risco: confiança baixa da spec; partes não verificadas retornam erro explícito em vez de
  fingir sucesso. Migração de dados: nenhuma (formato JSON existente preservado).
- Dependências: `QtCore` (QObject, QFuture-ready), `QtGui` (QImage para WriteImage), `boost`
  não é necessário no ArtPlus (usa `QFileInfo`/`QFile`); nenhuma dependência nova → sem ADR
  de licença adicional.

## Riscos e mitigações

| Risco | Mitigação |
|---|---|
| Formato `.artplus` binário não derivado | Reusa serializer JSON; `TODO(F-006)` documentado; erro explícito se paridade binária for exigida |
| Nome de backup desconhecido (`FUN_180095230`) | Derivação provisória `.bak` + `TODO(F-006)` |
| Criptografia não suportada | Retorna `SaveResult{ok=false}` com mensagem; não escreve plaintext |
| Incremental base copy não verificado | Flag registrada; base copy implementado como cópia do destino existente quando `isIncrementalSave` (com log) |
| `Flavour == 1/2` não verificados | Ramos retornam erro explícito `SaveException(8)` (paridade com E3) |
| CAP-BLOCKED-006 (GUI inativa) | Arquitetura não depende de captura para o caminho principal; captura futura valida |

## TODO(F-006) — partes deliberadamente não implementadas em v1

1. Formato binário `.artplus` (layout, chunking, compressão, assinatura) — `GODocument::Save`.
2. `NoOptimizedSave` — semântica exata do valor de flag ↔ modo de gravação.
3. Nome de arquivo de backup — conteúdo de `FUN_180095230`.
4. `Flavour` completo para `1` (preflight) e `2` (separações).
5. Mensagens de exceção de UI (disco cheio, permissão, caminho inválido).
6. Revalidação contra v24.03.10017 (versão-alvo) nas flags de backup.
7. `RevertToSaved`, `WriteContext`, `PieceInfoWriter`, `RegisterPieceInfoWriter`.
8. Salvamento assíncrono (`QFuture<SaveResult>`).
9. Criptografia owner/user (Security).

## Pontos ambíguos da spec (não decididos em definitivo)

Parágrafos exatos de `docs/features/F-006.md` que permanecem abertos e sustentam os
`TODO(F-006)` acima — não foram decididos por conta própria:

- §8.1 — formato binário `.artplus`: layout, chunking, compressão e assinatura.
- §8.2 — `NoOptimizedSave`: semântica exata (valor de flag ↔ modo de gravação).
- §8.3 — nome de arquivo de backup: conteúdo de `FUN_180095230`.
- §8.4 — `Flavour`: comportamento de `1` (preflight) e `2` (separações).
- §8.5 — mensagens de exceção de UI (disco cheio, permissão negada, caminho inválido).
- §8.6 — revalidação contra v24.03.10017 (versão-alvo) nas flags de backup.

A única escolha provisória (derivação `.bak`) está explicitamente marcada como `TODO(F-006)`
e será substituída quando `CAP-BLOCKED-006` for resolvido.

## Estado

proposto