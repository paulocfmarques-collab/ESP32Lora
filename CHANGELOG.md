# Changelog

Registro das alterações dos aplicativos ESP32 LoRa Transmissor e Receptor.

## 2026-10-31

### Organizacao do codigo compartilhado

- Centralizados na pasta ESP32Lora os modulos comuns de radio, protocolo,
  transporte, endereco/NVS, bateria, botao/OLED, tarefas, monitor e contadores.
- ConfiguracaoComum.h concentra os pinos Heltec, parametros de radio, limites
  RPC e temporizacoes comuns; Configuracao.h de cada sketch preserva suas
  configuracoes especificas.
- Interface e funcoes comuns de diagnostico movidas para a pasta principal,
  mantendo nos sketches a formatacao existente das respostas e seus servicos.
- Cada sketch inclui as implementacoes compartilhadas por ComponentesComuns.cpp,
  sem duplicar os corpos dos modulos. A pasta pai deve acompanhar os sketches.
- Endereco padrao passado pela aplicacao, preservando TX01 e RXXX, sem alterar NVS.
- Documentacao atualizada; compilacao permanece a cargo do usuario.

### Layout das páginas OLED

- Padronizados os cabeçalhos com ícones de rádio, estatísticas, Wi-Fi e OTA,
  separador e indicador de página em destaque nos dois firmwares.
- Identificação do gateway/receptor na página de estado e contadores alinhados
  à direita, com cinco linhas de estatísticas preservadas.
- Aplicada fonte compacta e rolagem horizontal para títulos, SSIDs e mensagens
  longas, evitando ocultar permanentemente o final do texto.
- Preservados os dados de rádio, navegação, proteção com robôs e prioridade de OTA.
- Compilação e avaliação visual ficam com o usuário.

### Proteção de tela nos dois firmwares

- Após cinco minutos sem atividade, o OLED mostra carinhas de robô animadas
  desenhadas com U8g2, piscando e mudando de expressão e posição.
- Botão, comando UDP no gateway ou comando LoRa endereçado ao receptor
  encerram a proteção e reiniciam o prazo, mantendo a página selecionada.
- Comandos serial do receptor também acordam a tela; o primeiro toque durante
  a proteção não avança a página.
- Atualizações de status e estatísticas não reiniciam o prazo. OTA tem prioridade.
- Rádio, rede e consultas continuam ativos; somente a tarefa OLED desenha.
- Compilação e teste nas placas ficam com o usuário.

### Comandos em minúsculas

- Padronizados os comandos de consulta UDP/LoRa, aliases, ajuda serial e
  exemplos em minúsculas nos dois firmwares e no cliente Python.
- Mantida a aceitação de entradas em maiúsculas ou caixa mista, com
  normalização antes da execução.
- Preservados os endereços em maiúsculas (RX01/TX01), marcadores REQ/RES/RAK
  e os campos das respostas, sem alterar o formato do protocolo.
- Atualizados README.md e COMANDOS.md com o catálogo e os exemplos.

### Correção de compilação

- Conversão explícita das casas decimais para `unsigned int` ao formatar valores
  em `DiagnosticoDispositivo`, eliminando ambiguidade dos construtores de
  `String` no Arduino ESP32 3.3.12, nos dois projetos.

### Gateway UDP / LoRa com consultas aos receptores

- Removido o envio periódico de `Ola Mundo` e o broadcast de demonstração.
  Gateway aguarda consultas UDP e receptores ficam ouvindo comandos endereçados.
- Sintaxe `comando:RX01`, incluindo `status:RX01`, encaminha a consulta ao receptor
  e devolve a resposta ao IP e porta do cliente original. Sem destino, consulta local.
- Executor `DiagnosticoDispositivo` e monitores comuns reutilizados nas duas
  funções: CPU, RAM, Flash, rede, rádio, bateria, contadores, build e status de serviços.
- Protocolo `REQ`/`RES`/`RAK` com correlação por endereços e ID, fragmentos de
  160 bytes, remontagem até 1400 bytes, confirmações e repetições limitadas.
- Cache da última resposta no receptor evita repetir a medição na retransmissão
  da mesma solicitação. Consultas por broadcast não são aceitas.
- `CanalGateway` separa a tarefa UDP da tarefa de rádio, com filas limitadas
  e preservação do remetente em consultas de clientes concorrentes.
- Prazo de 60 segundos por solicitação inclui espera na fila. Rádio indisponível,
  fila cheia, ausência de resposta ou resposta incompleta geram erro ao cliente.
- Cliente Python reconhece `status_json:RX01` e usa timeout remoto padrão de
  75 segundos. Mantém três segundos para consultas locais, com opção de ajuste.
- Receptores medem sua própria bateria e temperatura; snapshots de sensores
  sincronizados entre o loop de manutenção e a tarefa LoRa.
- Comandos de status informam serviços realmente disponíveis; o receptor não
  passa a ter listener UDP ou servidor OTA só por compartilhar o catálogo.
- Quatro páginas OLED, configuração serial de endereços, Wi-Fi e OTA do gateway
  preservados. README e COMANDOS.md atualizados para o novo fluxo.
- Atualização dos três firmwares necessária; protocolo anterior incompatível.
  Compilação e validação física continuam a cargo do usuário.


### Consultas UDP do transmissor

- Comandos de consulta na porta UDP 4210, com nomes inspirados no M5NanoC6:
  help, info, status, temp, cpu, ram, heap, flash, net_info, ip, mac e rssi.
- Consultas radio, counters, battery/vbat, ota_status e udp_status, além de
  status_json/info_json para integração com scripts e painéis.
- Tarefa UDP independente no core 0, com respostas ao IP/porta do remetente,
  limite de entrada de 255 bytes e resposta de 1400 bytes em um datagrama.
- Monitor sincronizado de fase, pacote, tentativa, contadores e último ACK.
  Rádio e OTA também expõem snapshots; UDP não disputa o SPI do SX1262.
- Contadores de timeout de ACK e erro de transmissão adicionados à telemetria.
- Leitura periódica de bateria para Heltec V3, ADC GPIO1 e chave GPIO37,
  média de oito leituras calibradas em mV, divisor nominal 4,9 e fator ajustável.
  A estabilização do ADC não bloqueia o processamento dos comandos.
- CPU amostrada pelo sensor interno; RAM inclui heap mínimo/maior bloco e PSRAM;
  Flash informa capacidade física, firmware, partição atual e slot OTA.
- Falha ao inicializar o rádio permite retornar ao loop de rede para diagnóstico.
- Catálogo COMANDOS.md e cliente Python tools/udp_query.py adicionados.
- Compilação, testes UDP nas placas e calibração da bateria ficam com o usuário.


### ArduinoOTA do transmissor pela Arduino IDE

- ArduinoOTA adicionado sem senha e sem chamada a `setPassword`, mantendo o
  upload HTTP já existente em `/ota`.
- Nome mDNS padrão `LoRa-TX01` e porta 3232, configuráveis em `Configuracao.h`.
- Serviço iniciado após conexão ao roteador com IP válido e partição OTA
  disponível; reiniciado quando o IP muda ou a conexão é restabelecida.
- Callbacks de início, progresso, conclusão e erro integrados à barra do OLED.
  Reinício após validação é agendado pelo serviço, preservando a tela de 100%.
- Atendimento dos transportes serializado para compartilhar `Update` sem
  gravações simultâneas; falhas abortam a atualização e restauram a tela normal.
- README em inglês inclui instalação inicial, seleção da porta de rede na IDE
  e diagnóstico de descoberta mDNS. Compilação e testes físicos ficam com o usuário.


### Acesso web pelo IP do transmissor na rede

- Página 3 do transmissor passa a exibir SSID da rede conectada, IP real recebido
  por DHCP e estado da conexão, em vez de manter os dados fixos do AP.
- Sem conexão ao roteador, exibe novamente o SSID e IP do acesso local.
- Atualização do OLED acompanha alterações de SSID, IP, conexão e endereço LoRa,
  mantendo a publicação dos dados sincronizada com a tarefa de display.
- Página web inclui link para o IP atual da estação; configuração e OTA usam
  rotas relativas e funcionam pelo IP do transmissor na rede, na porta 80.
- README em inglês atualizado. Compilação e testes físicos ficam com o usuário.


### Correção do Wi-Fi após reset

- Identificada senha do AP com seis caracteres nos dois projetos, inválida para
  WPA2, fazendo a criação do AP falhar antes de iniciar o servidor HTTP.
- Restaurada a senha de configuração `configlora` nos dois projetos.
- Verificação de compilação exige senha vazia (rede aberta) ou 8 a 63 caracteres;
  rede aberta é criada explicitamente com ponteiro de senha nulo.
- Logs separados para falhas do modo Wi-Fi, configuração do IP e criação do AP.
- Persistência interna do driver Wi-Fi desativada nos dois projetos; sleep
  desativado durante o acesso de configuração. Preferences mantidas.

### OTA HTTP do transmissor com progresso no OLED

- Página `/ota`, acessível pelo AP ou IP da rede local, sem senha ou login,
  vinculada à página de configuração do transmissor.
- Classe `ServicoOTA` recebe o binário de aplicação na partição inativa e verifica
  capacidade, tamanho exato, cabeçalho ESP32-S3 e descritor de aplicação.
- `Update.end()` valida a imagem antes da troca da partição de boot.
- Barra de envio no navegador e sobreposição temporária no OLED com percentual,
  barra de gravação e estado. Desenho continua na tarefa OLED do core 0.
- OLED chega a 100% somente após validação; reinício agendado após a resposta HTTP.
- Abortos, erros e timeout cancelam a gravação; erro aparece no OLED por cinco
  segundos e a navegação anterior é restaurada. Não há rollback automático de
  uma aplicação que já tenha sido validada e inicializada.
- Configuração de rede e scans bloqueados durante a atualização. Namespaces de
  Preferences preservados; nenhuma tabela de partições é alterada por OTA.
- Página informa quando falta uma partição OTA e orienta a instalação por USB.
- Compilação e testes físicos continuam a cargo do usuário.


### Quarta página OLED: parâmetros do rádio

- Transmissor e receptor passam a ter quatro páginas: estado, estatísticas,
  configurações da rede e configurações do rádio.
- Página 3 dedicada a endereço da placa, SSID do ponto de acesso, IP web e modo Wi-Fi.
- Nova página 4 exibe frequência, largura de banda, SF, coding rate, sync word,
  potência e preâmbulo, com navegação cíclica `1 -> 2 -> 3 -> 4 -> 1`.
- Contagem das páginas centralizada em `Configuracao.h` e usada no indicador
  OLED, no avanço pelo botão e nas mensagens de diagnóstico serial.
- Parâmetros LoRa centralizados em `Configuracao.h`, usados tanto pela
  inicialização do SX1262 quanto pela nova página, mantendo os valores atuais.
- README em inglês atualizado, incluindo diagrama Mermaid e badge de quatro páginas.
- Compilação e testes nas placas permanecem a cargo do usuário.


### Configuração serial dos receptores

- Classe `ComandosSerial` permite consultar e configurar o endereço sem Wi-Fi,
  pelo monitor serial a 115200 baud.
- Comandos `ENDERECO?`, `ENDERECO RX01` e `AJUDA`, com suporte a maiúsculas ou
  minúsculas e linhas terminadas por CR, LF ou CRLF.
- Reutiliza validação e persistência de `EnderecoDispositivo`, na mesma chave
  das Preferences usada pela página web. Atualização aplicada sem reiniciar.
- Leitura não bloqueante e limitada por ciclo, com descarte de linhas inválidas
  ou longas para não executar comandos truncados.
- Console atendido antes do servidor web, inclusive quando o Wi-Fi não inicia;
  alterações também aparecem na página 3 do OLED.
- Compilação e testes nas placas mantidos a cargo do usuário.


### Configuração Wi-Fi do transmissor

- Página do transmissor redesenhada com base no formulário de cinco redes e
  no painel escuro do projeto M5NanoC6; disponível em `/` e `/config`.
- Cadastro de até cinco SSIDs, preservação da senha quando o SSID permanece
  igual e o campo de senha está vazio, exclusão por SSID vazio e opção de rede aberta.
- Redes gravadas juntas nas Preferences (`lora-wifi`, chave `redes`), separadas
  do endereço LoRa. Falhas de validação ou gravação mantêm as redes anteriores.
- Classe `GerenciadorWiFi` faz tentativas na ordem dos campos, com timeout de
  13 segundos por rede, sem laços de espera bloqueantes no fluxo LoRa.
- Modo AP + STA mantém a rede local de configuração disponível, inclusive
  quando nenhuma rede cadastrada conecta. Alterações não reiniciam a placa.
- Busca assíncrona de redes próximas e indicação de conexão, IP e RSSI na página.
- Senhas salvas não são retornadas na página nem na API; entradas são escapadas
  para HTML e JSON e alterações exigem o token do formulário.
- Configuração de endereço TX01 preservada na mesma página. Receptores mantêm
  a configuração web anterior. Compilação e testes físicos ficam com o usuário.


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
