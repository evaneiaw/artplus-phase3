# DEPENDENCY_MAP.md

## Mapeamento de Dependências (DLLs Carregadas pelo ArtPro+)

O executável `ArtPro+.exe` depende de um conjunto robusto de DLLs locais (componentes Esko) e bibliotecas de sistema / terceiros identificadas através da análise estática e varredura de imports:

### 1. Componentes Esko e Pré-Impressão
- `Document.dll`, `DocumentFlexoTools.dll`, `DocumentMarks.dll`
- `PDFConversionRules.dll`
- `LayoutEngine.dll`
- `Cornering.dll`, `CurveApprox.dll`, `GeoMapper.dll`, `Snapping.dll`
- `StudioDesignerCoreController.dll`, `StudioDesignerCoreRenderEngine.dll`, `StudioCommon.dll`, `Pack3D.dll`, `Lib3D.dll`
- `PredistortEngine.dll`, `StructuralDesignImportProfile.dll`
- `JEFModel.dll`, `JEFView.dll`, `JEFQtGlue.dll`, `JEFUtils.dll`
- `NDLModel.dll`, `NDLModelActionList.dll`, `NDLModelLocal.dll`, `NDLTypes.dll`
- `GMGOpenColorModel.dll`, `HELLJobTicket.dll`, `Localization.dll`, `Logging.dll`, `Preferences.dll`

### 2. Framework Qt 6
- `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `Qt6Network.dll`, `Qt6PrintSupport.dll`
- `Qt6WebEngineCore.dll`, `Qt6WebEngineWidgets.dll`, `Qt6WebChannel.dll`
- `ExtraQt.dll`, `JEFQtGlue.dll`, `AlephClient_Qt.dll`

### 3. Bibliotecas de Terceiros e Sistema
- `boost_filesystem.dll`, `boost_iostreams.dll`, `boost_nowide.dll`, `boost_thread.dll`, `ExtraBoostFilesystem.dll`
- `libxml.dll`, `zlib1.dll`, `lcms.dll` (Color Management)
- `aws-sdk-cpp-core.dll`
- `d3d9.dll`, `opengl32.dll`, `yagl.dll`, `dali.dll`
- `ADVAPI32.dll`, `KERNEL32.dll`, `USER32.dll`, `SHELL32.dll`, `OLE32.dll`, `OLEAUT32.dll`, `WS2_32.dll`
- **Licenciamento**: `.exe_libFNP.dll`, `SSO.dll`, `Secur32.dll` (Excluídos do escopo de implementação do ArtPlus por restrição de licença e propriedade intelectual).
