## Selection Mark
- Versão: 25.0
- Caminho na UI: Barra de ferramentas (ícone não nomeado, próximo ao botão Select)
- Entrada: Objeto selecionado no documento
- Passos:
  1. Pressionar Ctrl+Shift+S para ativar o Selection Mark
  2. Verificar que o objeto sob o cursor ganha handles de seleção
  3. Confirmar que a barra de status mostra "Selection Mark ativo"
  4. Arrastrar o objeto para testar manipulação
  5. Redimensionar usando os handles (canto e laterais)
  6. Pressionar Delete para remover o objeto
  7. Pressionar Ctrl+Z para desfazer e Ctrl+Y para refazer (criar ações únicas)
- Campos do diálogo: Não há diálogo modal - comportamento não-modal
- Saída observada:
  - Handles de seleção aparecem imediatamente no objeto
  - Barra de status exibe "Selection Mark ativo" (texto exato)
  - O objeto pode ser manipulado normalmente com handles visíveis
  - Pressionar Delete remove o objeto e registra uma ação no histórico
  - Ctrl+Z e Ctrl+Y criam ações distintas no histórico (apenas uma ação por operação)
- Erros e limites:
  - Não há mensagem de erro específica para ativação
  - O comportamento é não-modal - não bloqueia a interface
  - O Selection Mark não tem ícone nomeado na árvore de UI (identificado apenas por posição)
  - A funcionalidade depende de ter um objeto selecionado no documento
- Capturas:
  - tools/reference_capture/F-025/shot-01.png (tela inicial)
  - tools/reference_capture/F-025/shot-02.png (Selection Mark ativado com handles visíveis)
  - tools/reference_capture/F-025/shot-03.png (objeto sendo arrastado com handles)
  - tools/reference_capture/F-025/shot-04.png (barra de status mostrando "Selection Mark ativo")
  - tools/reference_capture/F-025/shot-05.png (objeto removido após Delete)
  - tools/reference_capture/F-025/shot-06.png (histórico de ações no menu Edit)