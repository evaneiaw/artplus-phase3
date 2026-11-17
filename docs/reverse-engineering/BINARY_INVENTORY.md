# BINARY_INVENTORY.md

## 1. Executável de Referência
- **Nome**: `ArtPro+.exe`
- **Caminho Original**: `C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows\ArtPro+.exe`
- **Tamanho**: 31,276,976 bytes (~29.83 MB)
- **Arquitetura**: PE32+ (64-bit AMD64)
- **Timestamp do COFF**: 1710264208 (Março de 2024)

## 2. Seções PE Identificadas
| Nome da Seção | Endereço Virtual (VA) | Tamanho Virtual | Ponteiro Raw | Tamanho Raw |
| :--- | :--- | :--- | :--- | :--- |
| `.text` | `0x1000` | `0x12D238B` | `0x400` | `0x12D2400` |
| `.textidx` | `0x12D4000` | `0xA7903` | `0x12D2800` | `0xA7A00` |
| `.rdata` | `0x137C000` | `0x6EE7A8` | `0x137A200` | `0x6EE800` |
| `.data` | `0x1A6B000` | `0x4E9AF8` | `0x1A68A00` | `0x234000` |
| `.pdata` | `0x1F55000` | `0xA2FD8` | `0x1C9CA00` | `0xA3000` |
| `.fnp_dir` | `0x1FF8000` | `0x78` | `0x1D3FA00` | `0x200` |
| `.fnp_mar` | `0x1FF9000` | `0x1` | `0x1D3FC00` | `0x200` |
| `.rsrc` | `0x1FFA000` | `0x3DD98` | `0x1D3FE00` | `0x3DE00` |
| `.reloc` | `0x2038000` | `0x53730` | `0x1D7DC00` | `0x53800` |

## 3. Análise Ghidra Headless
O binário foi importado e analisado com sucesso utilizando o framework Ghidra (Headless Analyzer). A análise automática de funções, referências cruzadas e descompilação de símbolos confirmou a estrutura modular baseada em Qt 6 e DLLs satélites de pré-impressão.
