# ESP32 LoRa com ACK

Aplicativos para duas placas Heltec WiFi LoRa 32 V3: transmissor e receptor.
Abra o arquivo `.ino` da pasta correspondente no Arduino IDE, selecione a
Heltec WiFi LoRa 32 V3 e instale as bibliotecas RadioLib e U8g2.
Os arquivos `.cpp` da mesma pasta sao compilados automaticamente.

## Organizacao por responsabilidade

- `AplicacaoTransmissor` / `AplicacaoReceptor`: coordenam inicializacao, fluxo
  de envio/recepcao, ACKs, retransmissoes e deteccao de duplicados.
- `RadioLoRa`: possui o modulo SX1262 e encapsula o acesso ao radio.
- `TelaTransmissor` / `TelaReceptor`: inicializam o OLED e desenham as telas.
- `ProtocoloLoRa`: cria e interpreta `DATA|ID|mensagem` e `ACK|ID`.
- `EstatisticasTransmissor` / `EstatisticasReceptor`: guardam os contadores.
- `Configuracao.h`: pinagem, limites e intervalos de cada aplicativo.
- `.ino`: inicializa a aplicacao; o `loop()` atende o servidor de configuracao.

A tela recebe os contadores por referencia constante; apenas a aplicacao os
atualiza. O radio e o display pertencem aos respectivos objetos, sem instancias
globais de hardware separadas. O protocolo nao depende de radio nem de OLED.

Cada pasta e um sketch independente e inclui seus proprios arquivos de radio e
protocolo. Ao alterar esses arquivos ou os parametros LoRa em `RadioLoRa.cpp`,
aplique a mesma alteracao nas duas pastas para manter a compatibilidade.

Foram mantidos os parametros LoRa (915 MHz, 125 kHz, SF9, CR 4/5, sync 0x12,
20 dBm e preambulo 8), os formatos das mensagens e os tempos do codigo original.
A validacao fisica exige gravar as duas placas e observar a troca de ACKs,
retransmissoes e contadores no OLED e no monitor serial.

Validacao realizada: todos os arquivos .cpp e .ino dos dois aplicativos passaram
na verificacao de sintaxe e tipos do compilador ESP32-S3 instalado, usando os
headers reais de Arduino, RadioLib e U8g2. A compilacao completa foi interrompida
durante o processamento das bibliotecas; a geracao do firmware e o teste nas
placas permanecem pendentes.

## Navegação no OLED

Nos dois aplicativos, um toque no botão PRG/BOOT avança ciclicamente:

1. Estado atual da comunicação.
2. Estatísticas do aplicativo.
3. Configuração do rádio.

O canto superior direito mostra a página atual. Os eventos de transmissão e
recepção atualizam o estado sem mudar a página selecionada. Manter o botão
pressionado provoca apenas um avanço; solte e pressione novamente para avançar.
A classe `ControlePaginas` lê o GPIO0 com debounce de 40 ms em uma tarefa própria,
inclusive durante as esperas do rádio. O acesso ao OLED é protegido entre tarefas.
O botão RST continua sendo o botão de reinicialização da placa.

O histórico de alterações está em [CHANGELOG.md](CHANGELOG.md).

## Uso dos dois cores

Os dois projetos usam tarefas fixadas explicitamente nos cores da ESP32-S3:

| Core | Tarefa | Responsabilidade |
| --- | --- | --- |
| 0 | `ControlePaginas` | Tarefas separadas para leitura do PRG/BOOT e atualização do OLED; toques acumulados por notificações. |
| 1 | `TarefaComunicacao` | Envio e recepção LoRa, ACKs, retransmissões e contadores. |

A inicialização acontece em `setup()`. Ao final dela, a tarefa de comunicação
passa a executar os ciclos da aplicação. O `loop()` do Arduino atende o servidor web,
sem duplicar a comunicação. Os cores são definidos em `Configuracao.h`.

As telas protegem os textos e as cópias de contadores com um mutex.
Somente a tarefa OLED acessa o framebuffer e desenha as páginas. A tarefa de comunicação publica o
estado e uma cópia dos contadores; a tarefa de interface envia a página
selecionada ao OLED. A troca de página continua disponível durante as esperas do
rádio e os intervalos de transmissão.

A compilação e os testes das alterações de navegação e uso dos dois cores ficam
com o usuário, conforme solicitado.

A leitura do botão tem prioridade maior que a tarefa do OLED e ocorre a cada
10 ms, com debounce de 40 ms. Cada toque estável é guardado até a interface
processá-lo. O monitor serial registra a seleção como `OLED Receptor - pagina
3/3` (ou `Transmissor`), permitindo conferir se a terceira página foi selecionada.

Todas as páginas são redesenhadas a cada 250 ms. A tarefa LoRa publica dados
de estado sem desenhar no display. O envio I2C não mantém o mutex dos dados.

## Endereço configurável pela página web

Cada placa cria uma rede Wi-Fi própria para configuração. O transmissor usa
`LoRa-TX-XXXXXX` e cada receptor usa `LoRa-RX-XXXXXX`; o sufixo vem da identificação
física da placa, permitindo distinguir receptores ainda com o endereço padrão.

1. Consulte a rede na página 3 do OLED ou no monitor serial.
2. Conecte o celular ou computador a essa rede. Senha padrão: `configlora`.
3. Abra `http://192.168.4.1` no navegador. A rede é local e não fornece internet.
4. Informe o endereço e pressione **Salvar endereço**.

O transmissor começa como **TX01** e os receptores como **RXXX**. Configure os dois
receptores com endereços diferentes, por exemplo **RX01** e **RX02**, antes de usá-los
juntos. O padrão RXXX não é uma identificação única.

O endereço aceita de 1 a 16 letras, números, hífen ou sublinhado; espaços nas
extremidades são removidos e letras são convertidas para maiúsculas. `TODOS` é
reservado ao broadcast. A validação acontece no servidor da placa.

A classe `EnderecoDispositivo` lê e grava o endereço nas Preferences, namespace
`lora-config`, chave `endereco`. O padrão é gravado no primeiro uso e o valor salvo
é carregado após reiniciar ou desligar. Salvar aplica o endereço sem reiniciar;
um pacote já em transmissão mantém a origem original até encerrar suas tentativas.
Uma falha de gravação é exibida na página e mantém o endereço anterior em RAM.

`ServidorConfiguracao` concentra o Wi-Fi AP, as rotas HTTP e a página, sem arquivos
externos ou conexão com a internet. A senha está em `Configuracao.h`. O servidor
é atendido no `loop()` do Arduino; as tarefas do botão, OLED e LoRa permanecem
separadas nos cores 0 e 1.

### Mensagens com identificação

- Dados: `DATA|TX01|TODOS|123|Ola Mundo`.
- Confirmação: `ACK|RX01|TX01|123`.

O transmissor confere o número do pacote e o destinatário do ACK, e mostra no
serial e no OLED qual receptor respondeu. O receptor reconhece retransmissões
pela combinação de origem e número do pacote. O destino `TODOS` permite broadcast;
um destino específico é aceito somente pelo receptor correspondente.

**Atualize as três placas:** o novo formato não é compatível com os firmwares
anteriores. O transmissor ainda confirma a entrega pelo primeiro ACK válido;
esta alteração não implementa espera pela confirmação de todos os receptores nem
resolve a colisão de ACKs simultâneos.

A página 3 passa a mostrar endereço, rede de configuração e IP de acesso.
Compilação e testes físicos desta alteração ficam com o usuário, conforme solicitado.
