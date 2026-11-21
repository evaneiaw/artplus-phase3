# Política de autonomia — ArtPlus

Vale para o comando `/auto` e para a operação autônoma do `orquestrador`.
Em qualquer conflito, `AGENTS.md` ganha. A **versão de referência** é definida
em `docs/agents/REFERENCE_VERSION.md` (Esko ArtPro+ **v24.03.10017**).

## 1. Gatilho e escopo

```
/auto [escopo] [--extract] [--static] [--max N]
```

- `escopo` = módulo (`DocumentMarks.dll`) ou `F-###`.
- Vazio = escolher pelo §2.
- `--extract` = parar após evidência + spec + dossiê; não implementar.
- `--static` = só triagem/estático + dossiê; sem GUI, sem spec, sem código.
- `--max N` = no máximo N módulos nesta execução. É um **limite solicitado pelo
  usuário**, não uma regra padrão de encerramento.

## 2. Escolha do alvo (sem escopo)

1. **Terminar o que está em andamento**: estado mais avançado primeiro
   (`testado` > `implementado` > `arquitetado` > `spec` > `comportamento` >
   `analisado` > `triado`).
2. Módulos de **UI/documento**.
3. Domínio próprio da Esko.
4. Infra.
5. Pular terceiros (Qt, OpenCV, ICU, Xerces, OpenImageIO, OpenEXR, zlib) e
   módulos já `auditado`.
6. **Pular `bloqueado`**: feature registrada em `docs/agents/BLOQUEIOS.md` (estado
   `bloqueado` no `INVENTORY.md`) **não é redistribuída** até o bloqueio ser resolvido.

Desempate: tamanho decrescente. Registrar a escolha em **uma linha**.

## 3. Execução contínua

A execução continua até:

- a fila elegível estar concluída;
- não existir atividade independente possível;
- o ambiente impedir a continuação;
- um limite **real** da sessão/plataforma for atingido.

Não há limite padrão de 1 módulo / 90 minutos / 6 ciclos. `--max N` limita
módulos quando o usuário pedir. Ao atingir um limite real da sessão, gravar o
checkpoint (§8) e informar **exatamente como retomar**. Não confundir limite de
sessão com conclusão do projeto.

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
- Máximo **3 subagentes** ativos — limite operacional obrigatório, **sem enforcement
  automático** na config V1; nunca dois `comportamento`.

### Modo `--static` (noturno)

1. `rea` faz a triagem usando `diec`, `strings64` e `scripts/inspect_pe.py`.
   **Não** rode `rea analyze`/`decompile` com Ghidra nesse modo. **Não** crie
   `manifest.json` nem IDs `GHIDRA-*`/`CAP-*` sem um `F-###` válido; registre a
   triagem em `docs/reverse-engineering/`.
2. `testes` gera o dossiê com o que já existe.
3. Se não houver export Ghidra e o módulo tiver até 12 MB, `ghidra` analisa e
   exporta em `rag/ghidra/exports/`; depois `testes` atualiza o dossiê.
4. Se o módulo for maior que 12 MB ou já tiver export, o Ghidra é adiado e
   registrado como lacuna.
5. **Não** aciona `comportamento`, `documentacao`, `arquitetura`,
   `implementacao` nem `auditor`.

## 5. Evidência e confiança

- Toda evidência tem **SHA-256 + versão** (`24.03.10017`).
- Spec exige `GHIDRA-*` **e** `CAP-*`.
- Comportamento impossível (HASP, crash, diálogo bloqueado): spec com
  `confianca: baixa` + bloqueio registrado; **não implementar**.
- Evidência conflitante sobre regra central: bloquear e seguir; nunca adivinhar.
- Inferência sempre marcada com fonte.
- Proibido stub silencioso.
- Registros derivados da v25 são **históricos**: fora do fluxo ativo, sem servir
  de especificação/evidência da versão-alvo (ver `REFERENCE_VERSION.md`).

## 6. Falha, retry e bloqueio

- 2 tentativas por etapa, com motivo diferente; nunca repetir comando idêntico
  que já falhou sem mudar as condições.
- Auditoria reprovada volta **uma vez** com a lacuna exata; segunda = bloqueio.
- Bloqueio vai para `docs/agents/BLOQUEIOS.md` com: ID, módulo, etapa, motivo,
  evidência existente e próximo passo. A feature correspondente entra como
  `bloqueado` no `INVENTORY.md` e **não é redistribuída** até o bloqueio ser resolvido.
- Depois seguir para o próximo alvo elegível.

### Recuperação de execução

Ao falhar uma ferramenta: capturar o erro → identificar a causa → classificar
(permissão / ambiente / dependência / código / ferramenta) → tentar alternativa
segura → redistribuir quando apropriado → registrar bloqueio só após tentativas
justificadas. Não esconder falha; não declarar sucesso sem validação.

## 7. Autonomia de decisão

**Não** perguntar ao usuário: qual módulo analisar, classe implementar,
biblioteca usar, método escolher, arquivo modificar, teste rodar, abordagem
adotar, erro corrigir, especialista acionar, ou próxima tarefa.

Em dúvida técnica: consultar evidência → arquitetura existente → documentação →
comparar alternativas → escolher a mais consistente → registrar a justificativa
→ prosseguir. Sem evidência suficiente, **não inventar**: registrar a lacuna e
avançar nas atividades independentes.

## 8. Checkpoint e retomada

- Checkpoint operacional: `docs/agents/CHECKPOINT.md`.
- Atualizar **após cada etapa concluída** (não só ao fim da sessão).
- Conteúdo: última tarefa concluída, tarefa atual, próximas tarefas,
  especialistas acionados, arquivos modificados, estado do build, estado dos
  testes, auditorias, bloqueios, último commit, próxima ação executável.
- Se a sessão terminar por limite da plataforma, o checkpoint permite retomada
  sem repetir trabalho.

## 9. Escalação (única)

1. Credencial ou autorização pessoal indispensável.
2. Licença legítima ou acesso externo que só o usuário fornece.
3. Operação destrutiva ou irreversível fora do escopo autorizado.
4. Decisão comercial ou de produto não inferível tecnicamente.
5. Bloqueio ambiental que exija intervenção física/administrativa do usuário.

Mesmo nesses casos, continuar todas as tarefas independentes. Não transformar
uma pendência em motivo para interromper o projeto.

## 10. Git

- Repositório `https://github.com/evaneiaw/artplus-phase3`, branch `master`.
- Commit por conjunto coerente de alterações validadas; mensagem descritiva;
  push `origin/master`; confirmar o SHA publicado.
- Não commitar `build/`, `dossiers/`, `tools/ghidra_projects/`.
- Sem `--force`, sem reescrever histórico remoto.
- Conflito: resolver preservando o trabalho existente.

## 11. Saída obrigatória

- `docs/phases/STATUS.md` — estado resumido.
- `docs/agents/CHECKPOINT.md` — checkpoint operacional.
- `dossiers/<Módulo>/DOSSIER.md`, `dossiers/INDEX.md`, `dossiers/knowledge.jsonl`.
- Resumo final de 5 linhas (FEITO, BLOQUEADO, PRÓXIMO, COBERTURA, ONDE LER).

## 12. Definição de pronto

Uma feature só termina com: auditoria aprovada, dossiê atualizado, status
atualizado, commit e push. Sem isso, é bloqueio.
