# Versão de Referência — ArtPlus (fonte única da verdade)

Este arquivo é a **definição central** da versão de referência do projeto ArtPlus.
Nenhum documento, agente, comando, skill, script ou evidência ativa pode usar outra
versão como referência. Em qualquer conflito, este valor prevalece.

## Valor canônico

| Campo | Valor |
|---|---|
| Produto | Esko ArtPro+ |
| **Versão de referência** | **24.03.10017** |
| Rótulo curto | `v24.03.10017` |
| Diretório de instalação | `C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows` |
| Executável | `C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows\ArtPro+.exe` |

## Mapeamento de pastas instaladas (`C:\Esko`)

| Pasta | Versão real (VERSIONINFO) | Papel no projeto |
|---|---|---|
| `bg_prog_artpro+_v240` | **24.03.10017** | **Referência oficial (alvo do clone)** |
| `bg_prog_artpro+_v250` | 25.03.116 | Versão mais nova — **histórico, fora do fluxo ativo** |
| `bg_prog_artpro+_v250_extracted` | 25.03.116 | Extração — histórico |
| `bg_prog_artpro+_v250.rar` | 25.03.116 | Arquivo — histórico |

Versões confirmadas em `docs/reverse-engineering/VERSION_CONFIRMATION.md`
(2026-11-21, leitura de VERSIONINFO + SHA-256):

| Executável | FileVersion | SHA-256 |
|---|---|---|
| `bg_prog_artpro+_v240\...\ArtPro+.exe` | 24.03.10017 | `9D33166126305F25C50349BB881F7D9ACDC996F3C0C9181D0BE61295D70B0F03` |
| `bg_prog_artpro+_v250\...\ArtPro+.exe` | 25.03.116 | `D27DE85A46BFCD482F837AF58D620773DFAA4B918A751A1BA04BBD0C37EE2E4B` |

## Regras

1. Evidência, spec e implementação **ativas** usam **somente** `24.03.10017`
   (pasta `bg_prog_artpro+_v240`).
2. Registros derivados da v25 (`bg_prog_artpro+_v250`) são **históricos**: preservados
   para rastreabilidade, claramente identificados como históricos e **fora do fluxo ativo**.
   Não servem como especificação ou evidência da versão-alvo.
3. Nenhum hash, nome de binário ou resultado histórico pode ser alterado para simular
   evidência da v24.
4. `scripts/check_version.py` valida a consistência desta definição contra os arquivos ativos
   (e reporta os registros históricos sem reprovar o build).

## Formato nos metadados

- Spec frontmatter: `versao_ref: 24.03.10017`
- Manifest: `"versao_ref": "24.03.10017"`
- Cabeçalho de evidência: `versao: 24.03.10017`

## Nota sobre o toolchain (NÃO confundir)

`jdk-25.0.5.8-hotspot` / `JDK 25` / `JAVA_HOME ... jdk-25.0.5.8-hotspot` referem-se ao
**Java Development Kit** exigido pelo Ghidra 12.2 — **não** são a versão do ArtPro+ e
**não** devem ser alterados. A verificação de consistência ignora esses tokens.
