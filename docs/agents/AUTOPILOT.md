# Política de autonomia — ArtPlus

Vale para o comando `/auto`. Em qualquer conflito, `AGENTS.md` ganha.
Objetivo: extrair, especificar, implementar, testar e auditar sem pedir decisões.

## 1. Gatilho e escopo

```
/auto [escopo] [--extract] [--max N]
```

- `escopo` = módulo (`DocumentMarks.dll`) ou `F-###`.
- Vazio = escolher pelo §2.
- `--extract` = parar após evidência + spec + dossiê; não implementar.
- `--static` = só evidência estática + dossiê; sem GUI, sem spec, sem código.
- `--max N` = no máximo N módulos nesta execução (padrão 1).

## 2. Escolha do alvo (sem escopo)

1. **Terminar o que está em andamento**: estado mais avançado primeiro
   (`testado` > `implementado` > `arquitetado` > `spec` > `comportamento` >
   `analisado` > `triado`).
2. Módulos de **UI/documento**.
3. Domínio próprio da Esko.
4. Infra.
5. Pular terceiros (Qt, OpenCV, ICU, Xerces, OpenImageIO, OpenEXR, zlib) e
   módulos já `auditado`.

Desempate: tamanho decrescente. Registrar a escolha em **uma linha**.

## 3. Granularidade

- Um ciclo = uma funcionalidade de UI, com `F-###` sequencial.
- Um módulo pode ter várias features; só é `auditado` quando todas estiverem.
- Módulo gigante é dividido por menu/ferramenta, nunca por "terminar o DLL".

## 4. Pipeline e gates

| Etapa | Dono | Gate de saída |
|---|---|---|
| Triagem | `rea` | identidade + hash + superfície + dependências |
| Estático | `ghidra` | funções com endereço, xrefs e confiança |
| Comportamento | `comportamento` | `CAP-*` com passos, campos e saída |
| Spec | `documentacao` | `F-###` com 8 seções e evidência |
| Arquitetura | `arquitetura` | ADR com mapa função→classe e contrato |
| Código | `implementacao` | `src/**` compilando |
| Teste | `testes` | teste citando `F-###` passando |
| Auditoria | `auditor` | laudo com comando + saída |

- Estático e comportamento podem rodar em paralelo.
- Spec só com **1 evidência estática + 1 comportamental**.
- Máximo **3 subagentes** ativos; nunca dois `comportamento`.

### Modo `--static` (noturno)

1. `rea` faz a triagem do módulo usando `diec`, `strings64` e `scripts/inspect_pe.py`.
   **Não** rode `rea analyze`/`decompile` com Ghidra nesse modo; use os exports existentes.
   **Não crie `manifest.json` nem IDs `GHIDRA-*`/`CAP-*` sem um `F-###` válido**;
   registre a triagem em `docs/reverse-engineering/`.
2. `testes` gera o dossiê com o que já existe.
3. Se não houver export Ghidra e o módulo tiver até 12 MB, `ghidra` analisa e
   exporta em `rag/ghidra/exports/`; depois `testes` atualiza o dossiê.
   O `ghidra` não cria `manifest.json` sem `F-###`.
4. Se o módulo for maior que 12 MB ou já tiver export, o Ghidra é adiado e
   registrado como lacuna.
5. **Não** aciona `comportamento`, `documentacao`, `arquitetura`,
   `implementacao` nem `auditor`. O objetivo é conhecimento e dossiê.

## 5. Evidência e confiança

- Toda evidência tem **SHA-256 + versão**.
- Spec exige `GHIDRA-*` **e** `CAP-*`.
- Comportamento impossível (HASP, crash, diálogo bloqueado): spec com
  `confianca: baixa` + bloqueio registrado; **não implementar**.
- Evidência conflitante sobre regra central: bloquear e seguir; nunca adivinhar.
- Inferência sempre marcada com fonte.
- Proibido stub silencioso.

## 6. Falha, retry e bloqueio

- 2 tentativas por etapa, com motivo diferente.
- Auditoria reprovada volta **uma vez** com a lacuna exata; segunda = bloqueio.
- Bloqueio vai para `docs/agents/BLOQUEIOS.md` com: ID, módulo, etapa, motivo,
  evidência existente e próximo passo. Depois seguir para o próximo alvo.
- Nunca repetir comando idêntico que já falhou.

## 7. Orçamento

- 90 minutos ou 6 ciclos por execução; `--max` limita módulos.
- Ao atingir, parar e reportar. Não pedir autorização para continuar.
- A próxima execução retoma do estado registrado.

### Execução noturna

- Um módulo por sessão nova; limite de 50 minutos por módulo.
- Modo `--static`: sem GUI, sem spec, sem código.
- Parar ao atingir as horas configuradas ou a fila.
- Relatório em `docs/phases/OVERNIGHT-AAAAMMDD.md`.

## 8. Escalação (única)

1. Licença/HASP que impede o comportamento.
2. Evidência inconciliável sobre regra central.
3. Ação destrutiva ou irreversível.
4. Orçamento estourado (apenas reportar).
5. Credencial ou rede externa necessária.

Fora desses casos, decidir e registrar.

## 9. Git

- Criar branch `auto/AAAAMMDD` no início.
- Commit por feature auditada, só com os caminhos do ciclo.
- Não commitar `build/`, `dossiers/`, `tools/ghidra_projects/`.
- Sem push e sem `--force`.
- Se houver mudanças de outra sessão na árvore, não stagear.

## 10. Saída obrigatória

- `docs/phases/STATUS.md` — módulo atual, estado, cobertura, bloqueios, próximo.
- `dossiers/<Módulo>/DOSSIER.md` — documento único do módulo.
- `dossiers/INDEX.md` — painel de todos os dossiês.
- `dossiers/knowledge.jsonl` — conhecimento pesquisável.
- Resumo final de 5 linhas.

## 11. Definição de pronto

Uma feature só termina com: auditoria aprovada, dossiê atualizado, status
atualizado, commit na branch e `ci_local.ps1` verde. Sem isso, é bloqueio.
