# GHIDRA_STRATEGY.md

## Estratégia de Engenharia Reversa (Ghidra)

O ArtPlus utilizará o Ghidra para análise estática profunda. Dada a natureza do executável `ArtPro+.exe` (29.8 MB, 64-bit Windows PE, Qt 6), a estratégia foca em:

1. **Análise de Módulos**: Mapear as DLLs externas (`Document.dll`, `PDFConversionRules.dll`, `LayoutEngine.dll`) para identificar as fronteiras entre a lógica de negócio Esko e a implementação Qt/Standard.
2. **Identificação de Formatos**: Analisar as rotinas de I/O para `PDF`, `PDF+`, `CF2`, `MFG` e `ARD` (ArtiosCAD).
3. **Mapeamento de Funcionalidades**: Cruzar strings identificadas (ex: "ACTION/PAINT/CONVERT TO SPOT") com funções de controle de fluxo de pré-impressão.
4. **Segurança e Licenciamento**: Isolar e ignorar (ou excluir das ferramentas de implementação do ArtPlus) as rotinas identificadas como proteção contra cópia ou licenciamento (`FNP.dll`, `SSO.dll`, `Secur32.dll`).
5. **Automação**: Utilizar o Ghidra Headless para gerar relatórios de símbolos e referências cruzadas para acelerar a especificação dos motores geométrico e PDF do ArtPlus.
