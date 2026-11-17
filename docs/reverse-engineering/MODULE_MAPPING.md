# MODULE_MAPPING.md

## Mapeamento Inicial de Módulos (Baseado em strings e dependências)

| Módulo/DLL | Função Provável no ArtPro+ | Importância para o ArtPlus |
| :--- | :--- | :--- |
| `Document.dll` | Modelo de Documento e persistência | Crítica (base do motor de documento) |
| `PDFConversionRules.dll`| Parser/Writer PDF+ | Crítica (base da engine PDF) |
| `LayoutEngine.dll` | Step & Repeat, montagem | Crítica (base da produção) |
| `Cornering.dll` | Geometria, cantos, arredondamento | Alta (base da geometria) |
| `StudioDesignerCoreRenderEngine.dll` | Renderização Studio/3D | Média (recursos avançados) |
| `DocumentMarks.dll` | Gestão de Marcas (Marks) | Alta (base da produção) |
| `Pack3D.dll` | Estruturas e 3D | Baixa (recursos avançados) |
| `Qt6*.dll` | Interface e UI | Crítica (UI e eventos) |
| `FNP.dll` / `SSO.dll` | Licenciamento/Segurança | **IGNORAR** (proibido reuso) |
