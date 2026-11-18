---
name: Preflight Quality Control
description: Executa preflight e controle de qualidade no ArtPlus — fontes ausentes, imagem de baixa resolução, cor fora de gamut, cor não autorizada, objeto fora da página e problema de transparência. Use ao implementar verificação, emitir relatório ou decidir severidade de achado.
metadata:
  version: "2"
---

# Preflight e controle de qualidade

Modelo atual: `Color::PreflightRuleType` em `src/document/document_model.h` — `MissingFonts`, `LowResolutionImages`, `OutOfGamutColors`, `UnauthorizedColors`, `ObjectsOutsidePage`, `TransparencyIssues`.

## Regras

1. **Regra tem limiar explícito**: resolução mínima em `dpi`, tolerância de cor em DeltaE, margem em `mm`. Sem limiar, não há achado.
2. **Achado tem severidade**: `erro` bloqueia saída; `aviso` permite com registro; `info` só informa. A spec define.
3. **Localização**: todo achado aponta página, objeto e caixa/coordenada.
4. **Não corrigir sozinho**: preflight detecta; correção é ação separada e explícita.
5. **Fonte**: “ausente” = não embutida e não disponível no sistema; não confunda com substituição.
6. **Imagem**: calcular resolução efetiva após transformação, não só o DPI do arquivo.
7. **Cor**: comparar com perfil de destino; “não autorizada” = tinta fora da lista aprovada.
8. **Fora da página**: usar a caixa declarada (trim/bleed), não o mediaBox por default.
9. **Relatório reproduzível**: mesmas entradas + mesmas regras = mesmos achados, em ordem estável.

## Evidência

Captura do preflight do ArtPro+ com perfil/regras, lista de achados e mensagens literais. Sem isso, não invente texto nem default.

## Testes

- PDF com fonte não embutida → achado com página/objeto.
- Imagem 72 dpi escalada para 300% → resolução efetiva correta.
- Objeto 2 mm fora do trim com margem 3 mm → achado.
- Documento limpo → zero achados (não falso positivo).

## Limites

- Não bloqueie export sem regra na spec.
- Não altere o documento durante a verificação.
