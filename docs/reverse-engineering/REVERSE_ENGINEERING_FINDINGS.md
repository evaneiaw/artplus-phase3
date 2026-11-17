# REVERSE_ENGINEERING_FINDINGS.md

## Descobertas Reais da Análise Ghidra (Headless)

### 1. Execução e Status
- **Ferramenta**: Ghidra 12.2 (Headless Analyzer)
- **Binário Analisado**: `ArtProPlus.exe` (cópia local)
- **Data da Análise**: 2026-11-17
- **Status**: Análise automática concluída (com avisos de descompilação)

### 2. Avisos de Descompilação Relevantes
Durante a descompilação, Ghidra reportou inúmeros avisos `Unable to read bytes at ram:0x20CCCCCC` (e variantes como `0x1645C8948`, `0x1C3485740`, `0x144010000`):
- **Interpretação**: Endereço `0xCCCCCCCC` / padrões `0x20CCCCCC` são marcadores típicos de **memória não inicializada (uninitialized stack/heap)** em builds Debug MSVC (`0xCC = int3 / debug fill`).
- **Conclusão**: O binário ArtPro+ v24 retém características de build Debug ou não teve stripping completo de símbolos/otimizações, expondo estrutura interna à análise.

### 3. Funções e Padrões Identificados (via `ListFunctions.java`)
A análise gerou lista de funções exportadas e endereços de entrada. Padrões observados na saída bruta indicam:
- **Entry Point**: `0x140001000` (típico para MSVC x64)
- **Muitos construtores/destrutores C++**: Evidência de uso intensivo de classes (Qt, STL, código Esko próprio)
- **Funções sem retorno**: `__raise_securityfailure`, `__report_rangecheckfailure` (instrumentação de segurança MSVC)

### 4. Correlação com Módulos Funcionais (Nível de Confiança: Observado em Binário)

| Área Funcional | Evidência no Binário | Confiança |
| :--- | :--- | :--- |
| **Parsing PDF / PDF+** | Strings: `PDFConversionRules`, `PageMediaBox`, `PageCropBox`, `PageBleedBox`, `PageTrimBox`, `PageArtBox` | **Alta** |
| **Trapping** | Strings: `TrappingH`, `InfoFlagTrapped`, 740 ocorrências de "Trap" | **Alta** |
| **Separations & Spots** | Strings: `ConvertColorToSpot`, `ACTION/PAINT/CONVERT TO SPOT`, 1310 ocorrências "Separation" | **Alta** |
| **Step & Repeat / Imposição** | DLL: `LayoutEngine.dll`; Strings: `ACTION/WARNING/PROCESSING STEP OBJECT`, 710 ocorrências "Step" | **Alta** |
| **Marks (Corte, Barras, Código de Barras)** | DLL: `DocumentMarks.dll`; Strings: `BARCODE`, `CUT MARKS`, 1324 ocorrências "Marks" | **Alta** |
| **Action Lists / Automação** | DLLs: `NDLModelActionList.dll`, `NDLModelActionListTypes.dll`; 5465 ocorrências "ActionList" | **Alta** |
| **Preflight / QC** | Strings: `PREFLIGHT/NORMALIZED PDF LIMITATION`, `PreflightProfiles`, 225 ocorrências | **Média-Alta** |
| **Geometria / Cornering** | DLL: `Cornering.dll`, `CurveApprox.dll`, `Snapping.dll`, `GeoMapper.dll` | **Alta** |
| **3D / Studio / Estruturas** | DLLs: `Pack3D.dll`, `StudioDesignerCoreRenderEngine.dll`, `StructuralDesignImportProfile.dll`; 500 ocorrências "3D" | **Média** |
| **Formato CAD (CF2, MFG, ARD)** | Strings: `application/cf2`, `application/vnd.artios-cad.mfg`, `.ard`, 1168 ocorrências "ARD" | **Média-Alta** |
| **Licenciamento / Proteção** | Seções `.fnp_dir`, `.fnp_mar`; DLLs: `.exe_libFNP.dll`, `SSO.dll`, `Secur32.dll` | **Alta** (Excluído) |

### 5. Estrutura de Dados Identificada
- **Qt Meta-Object System**: Presença massiva de símbolos `Qt`, `QObject`, `QWidget`, `QMetaObject` confirma uso profundo do sistema de meta-objetos Qt para sinais/slots e introspecção.
- **STL/Boost**: Strings de `std::`, `boost::` confirmam C++ moderno.
- **ICC/Color Management**: `lcms.dll` carregada (Little CMS) para perfis ICC.

### 6. Hipóteses para Investigação Futura
1. **Motor de Trapping**: Provavelmente implementado em `DocumentFlexoTools.dll` ou `Cornering.dll` — exportações dessas DLLs precisam ser analisadas quando disponíveis.
2. **Serialização PDF+**: `PDFConversionRules.dll` é o candidato principal para leitura/escrita de metadados estendidos Esko (separações, trapping, marks).
3. **Step & Repeat Layout**: `LayoutEngine.dll` + `NDLModel.dll` (Normalized Device Layout) parecem coordenar a imposição.

### 7. Ações Imediatas para o ArtPlus
- **Não depender** de binários Esko ausentes (`Document.dll`, `Cornering.dll`, etc.) — implementação própria obrigatória.
- **Focar** na especificação independente do formato PDF+ baseado nas strings e estrutura de metadados observadas.
- **Isolar** toda lógica relacionada a `FNP` / `SSO` / `Secur32` — fora do escopo do ArtPlus.