#include "AplicacaoTransmissor.h"
#include "Configuracao.h"
#include "ProtocoloLoRa.h"

AplicacaoTransmissor::AplicacaoTransmissor()
    : configuracaoWeb(endereco, "TX"), tela(estatisticas), controlePaginas(atualizarTela, this),
      comunicacao(executarComunicacao, this)
{
}

bool AplicacaoTransmissor::aguardarACK(unsigned long idPacote, uint8_t tentativa,
                                       const String& origemPacote)
{
    uint8_t bufferACK[Configuracao::TAMANHO_BUFFER_ACK];

    memset(bufferACK, 0, sizeof(bufferACK));

    Serial.println();
    Serial.println("Entrando no modo de recepcao...");

    Serial.print("Aguardando ACK do pacote: ");
    Serial.println(idPacote);

    tela.mostrarAguardandoACK(idPacote, tentativa);

    /*
      Coloca o rádio em standby antes de mudar
      do modo de transmissão para recepção.
    */
    int state = radio.standby();

    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.print("Erro ao entrar em standby: ");
        Serial.println(state);

        return false;
    }

    /*
      Aguarda a resposta do receptor.

      O buffer permite receber até 63 bytes,
      deixando um byte reservado para '\0'.
    */
    state =
        radio.receive(bufferACK, Configuracao::TAMANHO_BUFFER_ACK - 1, Configuracao::TIMEOUT_ACK);

    // ===================================================
    // ACK RECEBIDO
    // ===================================================

    if (state == RADIOLIB_ERR_NONE)
    {
        size_t tamanhoACK = radio.getPacketLength();

        if (tamanhoACK >= Configuracao::TAMANHO_BUFFER_ACK)
        {
            tamanhoACK = Configuracao::TAMANHO_BUFFER_ACK - 1;
        }

        // Finaliza o conteúdo como texto
        bufferACK[tamanhoACK] = '\0';

        String respostaRecebida = String((char*)bufferACK);

        respostaRecebida.trim();

        float rssiACK = radio.getRSSI();
        float snrACK = radio.getSNR();
        float erroFrequencia = radio.getFrequencyError();

        Serial.println();
        Serial.println("Resposta LoRa recebida!");

        Serial.print("Conteudo bruto: ");
        Serial.println(respostaRecebida);

        Serial.print("Tamanho: ");
        Serial.print(tamanhoACK);
        Serial.println(" bytes");

        Serial.print("RSSI do ACK: ");
        Serial.print(rssiACK);
        Serial.println(" dBm");

        Serial.print("SNR do ACK: ");
        Serial.print(snrACK);
        Serial.println(" dB");

        Serial.print("Erro de frequencia: ");
        Serial.print(erroFrequencia);
        Serial.println(" Hz");

        unsigned long idConfirmado = 0;

        String receptor, destinatario;
        bool formatoValido =
            ProtocoloLoRa::interpretarACK(respostaRecebida, receptor, destinatario, idConfirmado);

        // A resposta não segue o formato ACK|origem|destino|ID
        if (!formatoValido)
        {
            estatisticas.acksInvalidos++;

            Serial.println();
            Serial.println("Resposta recebida, mas o formato e invalido.");
            Serial.println("Formato esperado: ACK|receptor|transmissor|numero");

            Serial.print("Total de ACKs invalidos: ");
            Serial.println(estatisticas.acksInvalidos);

            return false;
        }

        Serial.print("ID confirmado pelo receptor: ");
        Serial.println(idConfirmado);

        /*
          Somente confirma o pacote se o ID recebido
          for exatamente igual ao ID transmitido.
        */
        if (destinatario != origemPacote)
        {
            estatisticas.acksInvalidos++;
            Serial.println("ACK ignorado: destinado a outro transmissor.");
            return false;
        }
        Serial.print("Receptor que respondeu: ");
        Serial.println(receptor);
        if (idConfirmado == idPacote)
        {
            Serial.println();
            Serial.println("ACK VALIDO!");
            Serial.println("O receptor confirmou o pacote correto.");

            tela.mostrarACKRecebido(idPacote, rssiACK, snrACK, receptor);

            return true;
        }

        // ACK válido, mas referente a outro pacote
        estatisticas.acksInvalidos++;

        Serial.println();
        Serial.println("ACK referente a outro pacote!");

        Serial.print("ID enviado: ");
        Serial.println(idPacote);

        Serial.print("ID recebido: ");
        Serial.println(idConfirmado);

        tela.mostrarACKInvalido(idPacote, idConfirmado);

        return false;
    }

    // ===================================================
    // TIMEOUT
    // ===================================================

    if (state == RADIOLIB_ERR_RX_TIMEOUT)
    {
        Serial.println();
        Serial.println("Timeout: nenhum ACK foi recebido.");

        Serial.print("Pacote sem confirmacao: ");
        Serial.println(idPacote);

        return false;
    }

    // ===================================================
    // ERRO DE CRC
    // ===================================================

    if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
        estatisticas.errosCRC++;

        Serial.println();
        Serial.println("ACK recebido com erro de CRC.");

        Serial.print("Total de erros CRC: ");
        Serial.println(estatisticas.errosCRC);

        return false;
    }

    // ===================================================
    // OUTRO ERRO
    // ===================================================

    Serial.println();
    Serial.print("Erro durante a recepcao do ACK: ");
    Serial.println(state);

    return false;
}

bool AplicacaoTransmissor::enviarComACK(unsigned long idPacote, const String& mensagem)
{
    /*
      Formato transmitido:

      DATA|origem|TODOS|ID|mensagem

      Exemplo:

      DATA|TX01|TODOS|1|Ola Mundo
    */

    const String origemPacote = endereco.obter();
    String pacote = ProtocoloLoRa::criarDados(origemPacote, "TODOS", idPacote, mensagem);

    /*
      Todas as tentativas usam o mesmo ID.

      Isso permite que o receptor reconheça uma
      retransmissão e não processe o pacote duas vezes.
    */
    for (uint8_t tentativa = 1; tentativa <= Configuracao::MAX_TENTATIVAS; tentativa++)
    {
        Serial.println();
        Serial.println("==================================");

        Serial.print("Pacote completo: ");
        Serial.println(pacote);

        Serial.print("ID: ");
        Serial.println(idPacote);

        Serial.print("Tentativa: ");
        Serial.print(tentativa);
        Serial.print("/");
        Serial.println(Configuracao::MAX_TENTATIVAS);

        Serial.print("Tamanho: ");
        Serial.print(pacote.length());
        Serial.println(" bytes");

        tela.mostrarEnviando(idPacote, tentativa);

        // Garante que o rádio esteja pronto
        radio.standby();

        // Transmite o pacote
        int state = radio.transmit(pacote);

        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.println("Pacote transmitido.");
            Serial.println("Aguardando confirmacao do receptor...");

            /*
              O transmissor entra em recepção e aguarda
              o ACK referente ao mesmo ID.
            */
            bool confirmado = aguardarACK(idPacote, tentativa, origemPacote);

            if (confirmado)
            {
                estatisticas.pacotesConfirmados++;

                Serial.println();
                Serial.println("PACOTE ENTREGUE COM SUCESSO!");

                Serial.print("Pacotes confirmados: ");
                Serial.println(estatisticas.pacotesConfirmados);

                return true;
            }
        }
        else
        {
            Serial.print("Erro durante a transmissao: ");
            Serial.println(state);

            tela.mostrarErro("ERRO DE ENVIO", state);
        }

        /*
          Se ainda houver tentativas disponíveis,
          retransmite exatamente o mesmo pacote.
        */
        if (tentativa < Configuracao::MAX_TENTATIVAS)
        {
            estatisticas.totalRetransmissoes++;

            Serial.println();
            Serial.println("O mesmo pacote sera retransmitido.");

            Serial.print("Proxima tentativa: ");
            Serial.println(tentativa + 1);

            tela.mostrarRetransmissao(idPacote, tentativa + 1);

            /*
              Pequeno intervalo aleatório para diminuir
              a possibilidade de colisão no canal.
            */
            delay(random(700, 1500));
        }
    }

    // Todas as tentativas falharam
    estatisticas.pacotesNaoConfirmados++;

    Serial.println();
    Serial.println("FALHA: limite de tentativas atingido.");

    Serial.print("Pacote nao confirmado: ");
    Serial.println(idPacote);

    tela.mostrarFalha(idPacote);

    return false;
}

void AplicacaoTransmissor::iniciar()
{
    Serial.begin(115200);
    delay(1000);

    // Inicializa o gerador de intervalo aleatório
    randomSeed(micros());

    if (!endereco.iniciar())
    {
        Serial.println("Preferences indisponiveis; usando endereco padrao em RAM.");
    }
    Serial.print("Endereco local: ");
    Serial.println(endereco.obter());
    if (!configuracaoWeb.iniciar())
        Serial.println("Falha ao iniciar WiFi de configuracao.");
    tela.iniciar();
    enderecoMostrado = endereco.obter();
    tela.configurarRede(enderecoMostrado, configuracaoWeb.nomeRede());

    tela.mostrarInicializacao();
    if (!controlePaginas.iniciar())
    {
        Serial.println("Falha ao iniciar navegacao de paginas.");
        tela.mostrarErro("ERRO NAS PAGINAS", -1);
        while (true)
            delay(1000);
    }

    Serial.println();
    Serial.println("==================================");
    Serial.println(" TRANSMISSOR LoRa COM ACK");
    Serial.println("==================================");
    Serial.println("Inicializando o SX1262...");

    /*
      Parâmetros LoRa:

      Frequência:       915 MHz
      Bandwidth:        125 kHz
      Spreading Factor: SF9
      Coding Rate:      4/5
      Sync Word:        0x12
      Potência:         20 dBm
      Preâmbulo:        8 símbolos

      O receptor deve usar os mesmos parâmetros.
    */
    int state = radio.iniciar();

    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.print("Erro ao iniciar o LoRa: ");
        Serial.println(state);

        tela.mostrarErro("ERRO NO RADIO", state);

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("LoRa inicializado com sucesso!");
    Serial.println();
    Serial.println("Configuracao:");
    Serial.println("Frequencia: 915 MHz");
    Serial.println("Bandwidth: 125 kHz");
    Serial.println("Spreading Factor: SF9");
    Serial.println("Coding Rate: 4/5");
    Serial.println("Sync Word: 0x12");
    Serial.println("Potencia: 20 dBm");
    Serial.println("Preambulo: 8");
    Serial.println("ACK: habilitado");
    Serial.println("Tentativas: 3");
    Serial.println("Timeout ACK: 3000 ms");

    tela.mostrarPronto();

    delay(2000);
    if (!comunicacao.iniciar())
    {
        Serial.println("Falha ao iniciar tarefa LoRa no core 1.");
        tela.mostrarErro("ERRO TAREFA LORA", -1);
        while (true)
            delay(1000);
    }
}

void AplicacaoTransmissor::executar()
{
    tela.sincronizarEstatisticas();
    /*
      O número é incrementado apenas para criar
      um novo pacote.

      As retransmissões realizadas dentro de
      enviarComACK() mantêm o mesmo ID.
    */
    numeroPacote++;

    String mensagem = "Ola Mundo";

    bool entregue = enviarComACK(numeroPacote, mensagem);

    Serial.println();
    Serial.println("----------------------------------");

    if (entregue)
    {
        Serial.print("Envio confirmado do pacote: ");
        Serial.println(numeroPacote);
    }
    else
    {
        Serial.print("Pacote nao confirmado: ");
        Serial.println(numeroPacote);
    }

    Serial.println();
    Serial.println("ESTATISTICAS");

    Serial.print("Confirmados: ");
    Serial.println(estatisticas.pacotesConfirmados);

    Serial.print("Nao confirmados: ");
    Serial.println(estatisticas.pacotesNaoConfirmados);

    Serial.print("Retransmissoes: ");
    Serial.println(estatisticas.totalRetransmissoes);

    Serial.print("ACKs invalidos: ");
    Serial.println(estatisticas.acksInvalidos);

    Serial.print("Erros de CRC: ");
    Serial.println(estatisticas.errosCRC);

    Serial.println("----------------------------------");
    Serial.println("Aguardando o proximo pacote...");

    delay(Configuracao::INTERVALO_PACOTES);
}

void AplicacaoTransmissor::atualizarTela(void* contexto, bool avancar)
{
    static_cast<AplicacaoTransmissor*>(contexto)->tela.atualizar(avancar);
}

void AplicacaoTransmissor::executarComunicacao(void* contexto)
{
    static_cast<AplicacaoTransmissor*>(contexto)->executar();
}

void AplicacaoTransmissor::atenderConfiguracao()
{
    configuracaoWeb.atender();
    const String atual = endereco.obter();
    if (atual != enderecoMostrado)
    {
        enderecoMostrado = atual;
        tela.configurarRede(atual, configuracaoWeb.nomeRede());
        Serial.print("Novo endereco local: ");
        Serial.println(atual);
    }
}
