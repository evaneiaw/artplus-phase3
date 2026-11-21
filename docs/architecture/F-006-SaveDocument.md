# F-006 — Arquitetura de Save Document

> Decisão formal: `docs/decisions/ADR-006-save-document.md` (proposta).
> Spec: `docs/features/F-006.md` (confiança **baixa**).
> Evidência: `docs/reverse-engineering/F-006_GHIDRA.md` (21 funções, Document.dll + NDLModel.dll),
> `CAP-BLOCKED-006` (`docs/behavior/F-006.md`) — comportamento bloqueado por GUI inativa.

## 1. Visão geral

O salvamento no ArtPlus é orquestrado por **`DocumentSaveManager`**, dono do fluxo em
`src/document/`. A UI (`ArtPlusMainWindow::saveArtPlus`) apenas decide o caminho via
`QFileDialog` e delega ao gerenciador. O gerenciador **reusa** `DocumentSerializer::save`
(serialize JSON `.artplus`), decidindo: Save vs Save As, backup, incremental, criptografia
(explicitamente não suportada em v1), e finalização do estado do `Document`.

O fluxo binário de referência é:

```
NDLModel::SaveDocument (wrapper assíncrono)
  → ArtDoc::Document::Save(path, WriteOptions)   [0x180099710]
    → SwitchToWorkingFile / checagem de estado
    → Backup (copy/rename)                        [FUN_180095230]
    → GADocSaveParams (incremental / NoOptimizedSave / encryption)
    → GODocument::Save                            [0x1802ad561]
    → SetDirty(false) / SetFilePath / NotifyChangesCollected
```

No ArtPlus o único ponto de escrita é `DocumentSerializer::save` — substitui o comportamento
de `GODocument::Save`. A paridade de bytes com o formato proprietário permanece `TODO(F-006)`.

## 2. Diagrama de classes (mermaid)

```mermaid
classDiagram
    class ArtPlusMainWindow {
        -Document m_document
        -UndoStack m_history
        +saveArtPlus() void
        +openArtPlus() void
    }
    class DocumentSaveManager {
        -Document& doc_
        +save(path, WriteOptions) SaveResult
        +saveDocument(path, WriteOptions) SaveResult
        +deriveBackupPath(path) QString
        +revertToSaved() bool
        -switchToWorkingFile() bool
        -applyEncryption(params) bool
        -finalizeState(path, opts) void
        +progressStatus(QString) signal
        +finished(SaveResult) signal
    }
    class Document {
        +filePath() QString
        +setFilePath(path) void
        +modified() bool
        +setModified(m) void
        +metadata() QJsonObject
        +setMetadata(m) void
    }
    class DocumentSerializer {
        +save(doc, path) bool   «static reuse»
        +load(path, doc) bool   «static reuse»
    }
    class WriteOptions {
        +bool isSaveAs
        +bool isSaveAsCopy
        +bool isSaveUnmodified
        +bool needBackupOnSave
        +bool isIncrementalSave
        +bool noOptimizedSave
        +SaveFlavour flavour
        +SavePageRange[] pageRanges
        +SaveEncryptionParams encryption
        +fromSaveOptions(QString)$ WriteOptions
    }
    class SaveResult {
        +bool ok
        +QString filePath
        +QString backupPath
        +int errorCode
        +QString errorMessage
    }
    class SaveException {
        +int code()
    }

    ArtPlusMainWindow --> DocumentSaveManager : cria/delega
    DocumentSaveManager --> Document : referência (muta estado ao final)
    DocumentSaveManager --> DocumentSerializer : reusa para gravar
    DocumentSaveManager ..> WriteOptions : entrada
    DocumentSaveManager ..> SaveResult : saída
    DocumentSaveManager ..> SaveException : erro interno (não atravessa UI)
    SaveException --|> std::runtime_error
```

## 3. Diagrama de sequência — Save

```mermaid
sequenceDiagram
    actor U as Usuário
    participant W as ArtPlusMainWindow (UI)
    participant M as DocumentSaveManager
    participant D as Document
    participant S as DocumentSerializer
    participant F as Sistema de arquivos

    U->>W: Ctrl+S / File > Save .artplus
    W->>W: resolve destino (filePath ou QFileDialog Save As)
    W->>M: save(destino, WriteOptions)
    M->>M: switchToWorkingFile() (v1: no-op, TODO(F-006))
    M->>D: lê filePath(), modified(), metadata()
    alt destino inexistente ou diferente do atual
        M->>M: isSaveAs = true (E5)
    else mesmo caminho
        M->>M: isSaveAs = false
    end
    opt !isSaveAsCopy && !isSaveUnmodified
        opt needBackupOnSave
            M->>F: copy_file/rename(destino, backup)
            Note over M: incremental → copy; senão → rename
        end
    end
    opt encryption.encryptionLevel != 0
        M-->>W: SaveResult{ok=false, erro "criptografia não implementada"}
        Note over M: TODO(F-006) — nunca grava plaintext
    end
    M->>D: saveToMetadata() delegado pela UI antes do save
    M->>S: save(doc, destino)
    S->>F: escreve JSON .artplus
    F-->>S: ok / falha
    alt sucesso
        M->>D: setFilePath(destino), setModified(false)
        M-->>W: SaveResult{ok=true, filePath, backupPath}
        W->>U: status bar "Saved <arquivo>"
    else falha
        M-->>W: SaveResult{ok=false, errorCode, errorMessage}
        W->>U: QMessageBox::critical
    end
```

**Save As:** a UI chama `QFileDialog::getSaveFileName` e passa `isSaveAsCopy` conforme o
caso. `SaveAsCopy == true` mantém `filePath`/`modified` originais (E7) e não faz backup.

## 4. Fluxo de dados

| Etapa | Entrada | Transformação | Saída |
|---|---|---|---|
| UI | `m_document.filePath()`, ação do usuário | resolve destino; `QFileDialog` se vazio/Save As | `QString destinationPath` |
| Adapter NDLModel | `SaveOptions` (string_view) | `WriteOptions::fromSaveOptions()` | `WriteOptions` com flags nomeadas |
| Decisão de caminho | `doc.filePath()`, `destinationPath` | comparação (`boost::compare_v3` no binário; `QFileInfo` no ArtPlus) | `isSaveAs` |
| Backup | `destinationPath`, `needBackupOnSave`, `isIncrementalSave` | `deriveBackupPath()` + copy/rename | `backupPath` |
| Criptografia | `SaveEncryptionParams` | **não implementado** v1 | erro explícito (nunca plaintext) |
| Serialização | `Document` (modelo em memória) | `DocumentSerializer::toJson` → `QJsonDocument` | arquivo `.artplus` |
| Finalização | resultado da escrita | `setFilePath`, `setModified(false)` (salvo E6/E7) | estado do documento consistente |

Unidades: caminhos em `QString` (UTF-16 na borda Qt, convertidos o mais tarde possível);
senhas owner/user em `QString`; páginas 0-based; `Flavour` enum. Coordenadas do documento
permanecem em ponto PDF (1/72") no modelo — o save não converte unidades.

## 5. Decisões de ownership e threading

- **Ownership:** `DocumentSaveManager` é um `QObject` com `parent` na janela principal
  (mesmo padrão dos managers de marcas: `m_cutMarkManager{m_document}` etc.), mantendo
  `Document&` por referência; não possui o documento.
- **Threading:** v1 **síncrono no thread da UI**. `Document` e `UndoStack` não são
  thread-safe; serialização acontece no mesmo thread do clique. A variante assíncrona
  (`saveDocumentAsync() → QFuture<SaveResult>`) é `TODO(F-006)` e, quando implementada,
  deve: (a) capturar snapshot imutável do modelo fora da thread da UI; (b) gravar em
  arquivo temporário; (c) marshal de volta à thread da UI para `setFilePath`,
  `setModified(false)`, `m_history.clear()` e emissão de sinais.
- **Exceções:** `SaveException` é usada internamente (equivalente ao `_CxxThrowException`
  binário), mas é capturada antes da borda Qt e convertida em `SaveResult`; nenhuma
  exceção atravessa um slot (regra `qt-cpp-development`).
- **Atomicidade:** backup é feito **antes** da escrita; se `DocumentSerializer::save`
  falhar, o backup permanece e o documento continua com o estado anterior. Não há
  escrita em arquivo temporário na v1 — `TODO(F-006)` (paridade com working file).

## 6. Dependências entre módulos

Permitidas:

| De → Para | Uso |
|---|---|
| `src/ui` → `src/document` | construir `WriteOptions`, chamar `DocumentSaveManager::save`, exibir `SaveResult` |
| `src/document` → `src/document` | `DocumentSaveManager` reusa `DocumentSerializer` e `Document` |
| `src/document` → `QtCore`, `QtGui` | `QObject`, `QFileInfo`, `QFile`, `QImage` (WriteImage) |
| `src/pdf` → `src/document` | apenas leitura (export PDF existente); **não** participa do save `.artplus` |

Proibidas:

- `src/document` → `QtWidgets` (nenhum diálogo/message box no domínio).
- `src/document` → `src/ui`, `src/prepress`, `src/automation`.
- `src/pdf` → escrita de `.artplus` (responsabilidade de `DocumentSerializer`).
- Nova dependência externa (boost, criptografia) sem ADR de licença.

## 7. Pontos de extensão

1. `SaveFlavour` — novos ramos `Preflight`/`Separations` entram em `save()` sem alterar a UI.
2. `WriteOptions::fromSaveOptions()` — adapter para futuros chamadores NDLModel/Action Lists.
3. `applyEncryption()` — ponto único para plugar criptografia real quando houver evidência.
4. `deriveBackupPath()` — substituível quando `FUN_180095230` for decompilado/capturado.
5. `SaveResult` — permite adicionar códigos de erro sem mudar assinatura dos slots.

## 8. O que NÃO entra neste módulo

- Diálogo de arquivo, atalhos e mensagens modais (`src/ui`).
- Formato binário proprietário `.artplus` inventado sem evidência (`TODO(F-006)`).
- Exportação PDF (`src/pdf`, F-005/F-006 separados).
- Regras de preflight, marks ou step & repeat (domínios próprios; seus parâmetros já vão
  via `Document::metadata()` e `DocumentSerializer`).
- Working file temporário / restart recovery (domínio `src/automation/recovery`).

## 9. Testes planejados (a serem escritos pelo agente `testes`)

| ID | Cenário | Verificação | Evidência |
|---|---|---|---|
| T1 | Save no mesmo caminho (`isSaveUnmodified=false`) | `isSaveAs=false`; arquivo reescrito; `modified()==false` | F-006 R1 |
| T2 | Save em caminho novo | `isSaveAs=true`; `setFilePath(dest)`; backup não criado | F-006 R1/E5 |
| T3 | `IsSaveAsCopy=true` | `filePath()` e `modified()` inalterados; arquivo destino existe | F-006 E7 |
| T4 | `IsSaveUnmodified=true` | pula atualização de caminho/dirty | F-006 E6 |
| T5 | `NeedBackupOnSave=true`, incremental=false | `rename` para `deriveBackupPath()` | F-006 R3 |
| T6 | `NeedBackupOnSave=true`, incremental=true | `copy_file` para backup; destino preservado | F-006 R3 |
| T7 | Destino inexistente + `isIncrementalSave=true` | flag forçada a `false`; save normal | F-006 R2/E5 |
| T8 | `NoOptimizedSave` presente | flag registrada em `WriteOptions`; serializer recebe doc íntegro | F-006 R2 (inferência) |
| T9 | Criptografia solicitada | `SaveResult.ok==false`, mensagem explícita; nenhum arquivo gravado | ADR-006 D5 |
| T10 | `Flavour != NormalPdf` | erro explícito (não stub silencioso) | F-006 R2/E3 |
| T11 | Falha de escrita (diretório inexistente) | `SaveResult.ok==false`, `errorCode` preenchido; backup preservado | F-006 E4 |
| T12 | Round-trip `DocumentSerializer::save` → `load` | `IntegrityReport::ok == true` (reuse existente) | F-006 §6 |
| T13 | UI: `File > Save .artplus` e Ctrl+S | slot chama manager; status bar atualizada | F-006 §3 (escopo, CAP-BLOCKED-006) |
| T14 | UI: Save As | `QFileDialog` e caminho atualizado | F-006 §3 (escopo, CAP-BLOCKED-006) |
| T15 | Assíncrono (quando implementado) | `QFuture<SaveResult>`; mutações na thread da UI | ADR-006 D3 (`TODO(F-006)`) |

Método: Qt Test + CTest (`ctest --preset unit-tests`), com `QTemporaryDir` para destino e
backup. Tolerâncias: comparação de `QString` exata; `IntegrityReport` já existente cobre
geometria. Nenhum golden de formato binário até `CAP-BLOCKED-006` ser resolvido.

## 10. TODO(F-006) — visão consolidada

Ver ADR-006, seção "TODO(F-006)". Em resumo: formato binário, `NoOptimizedSave`, nome de
backup, `Flavour` 1/2, mensagens de erro de UI, revalidação contra v24.03.10017 (versão-alvo), RevertToSaved/WriteContext/
PieceInfo, save assíncrono e criptografia. Todos os pontos não implementados retornam erro
explícito ou registram `TODO(F-006)` — nenhum stub silencioso no caminho principal.