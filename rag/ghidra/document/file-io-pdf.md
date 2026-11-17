# File I/O & PDF Handling - Document.dll

## Overview
Manipulação de arquivos PDF, abertura, salvamento, criptografia, XMP metadata e handlers.

## Funções e Fluxos Principais (Strings)

### Abertura de Documento
```
Document: Open read/write: ...
Document: Open read/write but file is write-protected: ...
Document: Open read-only: ...
Document: Exception caught while opening ...
Document: File ... could not be found
Document: Unknown encryption value ...
ERROR_FILE_NOT_FOUND
ERROR_FILE_ACCESS_DENIED
ERROR_FILE_INVALID
ERROR_WRONG_PASSWORD
ERROR_FILE_UNKNOWN
DocumentOpenException (use GetDescription() for more details on the error)
```

### Salvamento de Documento
```
STATUS_SAVE
STATUS_OPTIMIZING
STATUS_WRITING_TO_DISK
Document: Could not copy the incremental base file to ...
Document: Could not copy the preflighted working file to ...
Document: GASimpleException ...
Document: GException ...
Document: Standard exception ...
Document: Unknown exception caught while saving ...
Document: The original file could not be copied to the working file; so saving ...
Document: Switching to working file ... failed
Document: Deleting working file ...
Document: Destructor for ...
Document: File has no pages
```

### Background Copy (Incremental Save)
```
BackgroundCopy: Start copying from ...
BackgroundCopy: Copied to ...
BackgroundCopy: Destructed
BackgroundCopy: Sleep ...
; error code is ...
; error code for copying is ...
```

### XMP Metadata
Strings relacionadas a XMP:
```
PDFAVersion
PDFAConformance
version
http://ns.esko-graphics.com/pdfnatversion/1.0/
Enfocus PDF
Metadata
Author
Subject
Keywords
Title
Producer
Creator
GTS_PDFXVersion
GTS_PDFVTVersion
ModDate
CreationDate
Document-%%%%-%%%%-%%%%-%%%%.pdf
```

### PDF/X / PDF/VT / PDF/A
```
GTS_PDFXVersion
GTS_PDFVTVersion
PDFAVersion
PDFAConformance
```

### PDF Structure
```
Perms
PieceInfo
DPartRoot
OutputIntents
```

### File Handlers Registrados
```
application/pdf
application/illustrator
EskoPNGr
EskoPNPDFO
EskoPNBrix
```

### Esko Extensions
```
Esko_Distortion
inpd:Group
Esko:Group
Esko_Group
Esko_Object
```

### Evidence: Cross-references (Symbols)
- `XMPlib` - Biblioteca XMP completa com:
  - `SetPDFNativeVersion`, `SetNrPages`, `SetIsTrapped`
  - `MergeNonEsko`, `MergeExtRefs`, `SetLayers`, `SetBarcodes`, `SetFonts`
  - `GetLayers`, `GetExtRefs`, `GetFonts`, `GetXML`, `SetXML`
  - `SetDimDat`, `SetColorSpace`, `GetColorSpace`, `GetDimDat`
  - `GetIsTrapped`, `GetPDFNativeVersion`, `SetPagRanges`, `SetInks`
  - `GetInks`, `GetInkProperties`
- `XMPlBarcode`, `XMPlExtRef`, `XMPlLay`, `XMPlPagRange`, `XMPlInk`, `XMPlInkProperty`, `XMPlColorSpace`, `XMPlDimDat`, `XMPlFont`

### Enfocus PDFLib
```
Enfocus PDFLib initialized (2/7)
Enfocus PDFLib finalized (3/4)
```

---
*Generated from: strings.csv, symbols.csv*