# Checklist de auditoria

## 1. Spec

- [ ] `docs/features/F-###.md` existe e tem frontmatter.
- [ ] 8 seções presentes.
- [ ] Regras numeradas e testáveis.
- [ ] Unidades e tolerâncias.
- [ ] `status` coerente com a realidade.

## 2. Evidência

- [ ] `rag/evidence/F-###/manifest.json` válido.
- [ ] `sha256` bate com o binário.
- [ ] `versao_ref` e `binario` corretos.
- [ ] Todo ID citado tem arquivo.
- [ ] Bloqueios declarados não escondem lacuna central.

## 3. Código

- [ ] Cada regra da spec tem implementação localizável.
- [ ] Sem stub silencioso.
- [ ] Sem `#if 0`, código morto ou `catch(...)`.
- [ ] Unidades no nome do campo.
- [ ] Sem dependência proibida (Widgets no domínio).

## 4. Build

- [ ] Build limpo do zero.
- [ ] Sem aviso de toolchain.
- [ ] `CMAKE_CXX_COMPILER` e `CMAKE_PREFIX_PATH` corretos.

## 5. Teste

- [ ] `ctest` passa; saída anexada.
- [ ] Teste cita `F-###`.
- [ ] Teste falha se a regra for removida (verificação por inspeção).
- [ ] Golden tem origem na evidência.

## 6. Comportamento

- [ ] Comparação com `rag/evidence/F-###/behavior/`.
- [ ] Geometria/valores dentro da tolerância.
- [ ] Divergência explicada ou reprovada.

## 7. Rastreabilidade

```
F-### → GHIDRA-* → arquivo:linha → teste → ctest
```

- [ ] Todos os elos existem.
- [ ] Nenhum elo inventado.

## 8. Veredito

- [ ] `APROVADO` só com comando + saída.
- [ ] `REPROVADO` lista o que falta, específico.
- [ ] `APROVADO COM RESSALVAS` lista cada ressalva.

## Red flags

- Relatório sem caminho de arquivo.
- “Testado” sem saída de `ctest`.
- Evidência com hash de outro binário.
- Spec que descreve o código, não o original.
- Teste que só verifica “não crashou”.
