# RELATÓRIO DE ANÁLISE TÉCNICA E REVERSA (GHIDRA)

## 1. Objetivo da Análise
Investigar a estrutura interna, dependências, rotinas de carregamento de plugins, formato de metadados estendidos em PDF+ (PDF/X e tags proprietárias Esko) e pontos de entrada do executável de referência:
`C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows\ArtPro+.exe`

## 2. Metodologia Autorizada
- Análise estática com Ghidra (Headless / GUI) focada exclusivamente em:
  - Imports e exports (tabela PE).
  - Strings e referências a arquivos de configuração, formatos de arquivo (`.pdf`, `.pdf+`, `.ard`, `.cf2`, `.mfg`).
  - Assinaturas de funções de parsing de PDF e metadados de pré-impressão.
- **Restrição**: Nenhuma tentativa de contornar licenças, copiar código proprietário ou descriptografar módulos protegidos. O objetivo é estritamente técnico para estruturar os parsers e formatos compatíveis no ArtPlus.

## 3. Descobertas Preliminares (Estrutura PE e Bibliotecas)
- O executável é um binário PE de 64 bits para Windows (`ArtPro+.exe`, ~31.2 MB).
- Utiliza DLLs Qt 6 / Qt 5 para interface e renderização gráfica.
- Integração com bibliotecas padrão de PDF e processamento de imagem.
- Presença de strings indicando suporte a PDF/X, metadados XMP, separações de tinta customizadas e perfis ICC.

## 4. Implicações para o ArtPlus
O ArtPlus implementará compatibilidade com os formatos abertos suportados pelo ArtPro+ (PDF/PDF+, CF2 para CAD estrutural, XML/XMP para metadados de separações e trapping) sem depender de bibliotecas proprietárias da Esko.
