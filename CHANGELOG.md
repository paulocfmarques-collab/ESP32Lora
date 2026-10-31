# Changelog

Registro das alterações dos aplicativos ESP32 LoRa Transmissor e Receptor.

## 2026-10-31

### Configuração web e endereçamento

- Página web embarcada nos dois projetos para configurar o endereço da placa.
- Transmissor com padrão `TX01`; receptores com padrão `RXXX`.
- Persistência via `Preferences`: namespace `lora-config`, chave `endereco`.
  Padrão gravado no primeiro uso e endereço salvo restaurado na inicialização.
- Rede AP individual por placa, com sufixo físico: `LoRa-TX-XXXXXX` ou
  `LoRa-RX-XXXXXX`, senha padrão `configlora`, acesso em `http://192.168.4.1`.
- Classes `EnderecoDispositivo` e `ServidorConfiguracao` separam persistência,
  validação, servidor e interface web do controle LoRa.
- Validação de 1 a 16 caracteres alfanuméricos, hífen ou sublinhado, com
  normalização para maiúsculas e reserva de `TODOS` ao broadcast.
- Retorno de erro quando a gravação falha; mudanças aplicadas sem reiniciar.
- Página 3 do OLED exibe endereço, rede Wi-Fi e IP de configuração.
- Protocolo passa a incluir origem e destino: `DATA|origem|destino|id|mensagem`
  e `ACK|origem|destino|id`. Requer atualizar todos os dispositivos.
- ACK identifica o receptor; transmissor confere destino e ID antes de confirmar.
  Continua confirmando pelo primeiro ACK, sem confirmação coletiva.
- Duplicados reconhecidos por origem e ID; recepção limitada ao buffer disponível.
- Compilação e testes físicos deixados com o usuário, conforme solicitado.


### Correções da atualização do OLED

- Após relato de `pagina 3/3` no serial com o OLED ainda na página 2,
  concentrado todo o desenho e envio de quadros na tarefa OLED do core 0.
  A tarefa LoRa publica somente textos e contadores, sem acessar o framebuffer.
- Todas as páginas passam a ser reenviadas periodicamente a cada 250 ms,
  além da atualização imediata nas mudanças de página e de estado.
- Mutex protege a cópia dos dados; envio ao OLED ocorre fora dessa proteção.
- Mensagem de página no serial emitida após o retorno do envio do quadro;
  isso não confirma fisicamente a recepção pelo display.
- Compilação e confirmação no hardware permanecem a cargo do usuário.

### Correções de navegação

- Separadas a leitura do PRG/BOOT e a atualização física do OLED em tarefas
  distintas no core 0. A tarefa do botão tem prioridade maior e não aguarda
  o desenho da página nem o mutex do display.
- Toques confirmados pelo debounce são acumulados em notificações, evitando
  descartá-los enquanto o OLED está ocupado, inclusive na página de estatísticas.
- Monitor serial informa a página selecionada para conferir o ciclo
  `1/3 -> 2/3 -> 3/3 -> 1/3` nos dois aplicativos.
- Alteração motivada pelo relato de que o receptor parava na página 2;
  confirmação do comportamento corrigido no hardware permanece pendente.

### Adicionado

- Navegação por três páginas no OLED dos dois aplicativos: estado atual,
  estatísticas e configuração do rádio.
- Classe `ControlePaginas` para ler o botão PRG/BOOT, com tratamento de
  oscilações do contato (debounce de 40 ms) e um avanço por toque.
- Leitura do botão em tarefa independente das esperas de recepção e dos
  intervalos entre transmissões.
- Uso explícito dos dois cores nos dois aplicativos: tarefa `ControlePaginas`
  no core 0 e tarefa `TarefaComunicacao` no core 1.
- Atualização física do OLED feita pela tarefa de interface no core 0.
- Indicador de página `1/3`, `2/3` e `3/3` no OLED.
- Proteção do acesso ao display entre as tarefas e cópia sincronizada dos
  contadores para a página de estatísticas.
- `README.md` com a organização e as instruções de uso dos aplicativos.

### Alterado

- Transmissor e receptor reorganizados com orientação a objetos e arquivos
  `.h` e `.cpp` separados por responsabilidade.
- `RadioLoRa` encapsula o módulo SX1262 e sua inicialização.
- `TelaTransmissor` e `TelaReceptor` concentram a apresentação no OLED.
- `ProtocoloLoRa` concentra a criação e a interpretação das mensagens.
- `EstatisticasTransmissor` e `EstatisticasReceptor` armazenam os contadores.
- `AplicacaoTransmissor` e `AplicacaoReceptor` coordenam o fluxo de comunicação.
- `Configuracao.h` reúne pinagem, limites, intervalos e configuração do botão.
- Arquivos `.ino` inicializam a aplicação em `setup()`; o `loop()` apenas
  aguarda a execução das tarefas nos dois cores.
- A página selecionada permanece visível durante os eventos de comunicação.

### Compatibilidade

- Mantidos os formatos `DATA|ID|mensagem` e `ACK|ID`, a confirmação de pacotes
  repetidos e as tentativas de retransmissão com o mesmo identificador.
- Mantidos os parâmetros LoRa e os intervalos de comunicação originais.
- Botão PRG/BOOT configurado no GPIO0, conforme o
  [manual da Heltec V3](https://s.heltec.cn/download/WiFi_LoRa_32_V3/HTIT-WB32LA_V3%28Rev1.1%29.pdf).

### Validação

- Refatoração inicial verificada quanto à sintaxe e aos tipos com o compilador
  ESP32-S3 e os headers reais das bibliotecas instaladas.
- Geração completa do firmware e testes físicos dos dois aplicativos pendentes.
- Compilação e testes das alterações de navegação e uso dos dois cores
  deixados a cargo do usuário, conforme solicitado.
