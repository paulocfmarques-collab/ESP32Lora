#include "AplicacaoReceptor.h"
#include "Configuracao.h"
#include "ProtocoloLoRa.h"

AplicacaoReceptor::AplicacaoReceptor()
    : configuracaoWeb(endereco, "RX"), tela(estatisticas), controlePaginas(atualizarTela, this),
      comunicacao(executarComunicacao, this)
{
}

bool AplicacaoReceptor::enviarACK(unsigned long idPacote, bool duplicado,
                                  const String& origemTransmissor)
{
    String ack = ProtocoloLoRa::criarACK(endereco.obter(), origemTransmissor, idPacote);

    Serial.println();
    Serial.println("Preparando resposta...");

    Serial.print("ACK: ");
    Serial.println(ack);

    tela.mostrarEnviandoACK(idPacote);

    /*
      Aguarda o transmissor terminar o envio
      e entrar no modo de recepção.
    */
    delay(Configuracao::ATRASO_ANTES_ACK);

    // Encerra o modo de recepção
    radio.standby();

    // Envia a confirmação
    int state = radio.transmit(ack);

    if (state == RADIOLIB_ERR_NONE)
    {
        estatisticas.acksEnviados++;

        Serial.println("ACK enviado com sucesso!");

        Serial.print("Pacote confirmado: ");
        Serial.println(idPacote);

        Serial.print("Total de ACKs enviados: ");
        Serial.println(estatisticas.acksEnviados);

        tela.mostrarACKEnviado(idPacote, duplicado);

        return true;
    }

    estatisticas.errosACK++;

    Serial.print("Erro ao enviar ACK: ");
    Serial.println(state);

    tela.mostrarErro("ERRO NO ACK", state);

    return false;
}

void AplicacaoReceptor::iniciar()
{
    Serial.begin(115200);
    delay(1000);

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
    Serial.println(" RECEPTOR LoRa COM ACK");
    Serial.println("==================================");
    Serial.println("Inicializando o SX1262...");

    /*
      Estes parâmetros devem ser iguais aos
      utilizados pelo transmissor.
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
    Serial.println("Frequencia: 915 MHz");
    Serial.println("Bandwidth: 125 kHz");
    Serial.println("Spreading Factor: SF9");
    Serial.println("Coding Rate: 4/5");
    Serial.println("Sync Word: 0x12");
    Serial.println("Potencia do ACK: 20 dBm");
    Serial.println("Preambulo: 8");
    Serial.println();
    Serial.println("Aguardando pacotes...");

    tela.mostrarAguardando();

    delay(1000);
    if (!comunicacao.iniciar())
    {
        Serial.println("Falha ao iniciar tarefa LoRa no core 1.");
        tela.mostrarErro("ERRO TAREFA LORA", -1);
        while (true)
            delay(1000);
    }
}

void AplicacaoReceptor::executar()
{
    tela.sincronizarEstatisticas();
    uint8_t buffer[Configuracao::TAMANHO_BUFFER];

    memset(buffer, 0, sizeof(buffer));

    tela.mostrarAguardando();

    Serial.println();
    Serial.println("----------------------------------");
    Serial.println("Aguardando pacote LoRa...");

    /*
      Aguarda um pacote por até cinco segundos.

      A leitura fica limitada a capacidade do buffer, reservando
      um byte para o terminador de texto.
    */
    int state =
        radio.receive(buffer, Configuracao::TAMANHO_BUFFER - 1, Configuracao::TIMEOUT_RECEPCAO);

    if (state == RADIOLIB_ERR_NONE)
    {
        size_t tamanho = radio.getPacketLength();

        if (tamanho >= Configuracao::TAMANHO_BUFFER)
        {
            tamanho = Configuracao::TAMANHO_BUFFER - 1;
        }

        buffer[tamanho] = '\0';

        String pacoteRecebido = String((char*)buffer);

        pacoteRecebido.trim();

        float rssi = radio.getRSSI();
        float snr = radio.getSNR();
        float erroFrequencia = radio.getFrequencyError();

        Serial.println("Pacote recebido!");

        Serial.print("Conteudo: ");
        Serial.println(pacoteRecebido);

        Serial.print("Tamanho: ");
        Serial.print(tamanho);
        Serial.println(" bytes");

        Serial.print("RSSI: ");
        Serial.print(rssi);
        Serial.println(" dBm");

        Serial.print("SNR: ");
        Serial.print(snr);
        Serial.println(" dB");

        Serial.print("Erro de frequencia: ");
        Serial.print(erroFrequencia);
        Serial.println(" Hz");

        unsigned long idPacote = 0;
        String mensagem;

        String origem, destino;
        bool valido =
            ProtocoloLoRa::interpretarPacote(pacoteRecebido, origem, destino, idPacote, mensagem);

        if (!valido)
        {
            Serial.println("Pacote ignorado: formato invalido.");

            tela.mostrarErro("PACOTE INVALIDO", -1);

            delay(1500);
            return;
        }

        if (destino != "TODOS" && destino != endereco.obter())
        {
            Serial.println("Pacote ignorado: destinado a outro receptor.");
            return;
        }
        Serial.print("Remetente: ");
        Serial.println(origem);
        bool duplicado =
            existeUltimoID && idPacote == ultimoIDRecebido && origem == ultimaOrigemRecebida;

        if (duplicado)
        {
            estatisticas.pacotesDuplicados++;

            Serial.println();
            Serial.println("Pacote duplicado detectado.");
            Serial.println("O ACK anterior pode ter sido perdido.");
            Serial.println("Enviando o ACK novamente.");
        }
        else
        {
            estatisticas.pacotesNovos++;

            ultimoIDRecebido = idPacote;
            ultimaOrigemRecebida = origem;
            existeUltimoID = true;

            Serial.println();
            Serial.println("Novo pacote recebido.");
        }

        Serial.print("ID: ");
        Serial.println(idPacote);

        Serial.print("Mensagem: ");
        Serial.println(mensagem);

        Serial.print("Pacotes novos: ");
        Serial.println(estatisticas.pacotesNovos);

        Serial.print("Pacotes duplicados: ");
        Serial.println(estatisticas.pacotesDuplicados);

        tela.mostrarPacote(idPacote, mensagem, rssi, duplicado);

        // Responde ACK mesmo para um pacote duplicado
        bool resultadoACK = enviarACK(idPacote, duplicado, origem);

        if (resultadoACK)
        {
            Serial.println("Confirmacao concluida.");
        }
        else
        {
            Serial.println("Falha ao enviar a confirmacao.");
        }

        delay(1000);
    }
    else if (state == RADIOLIB_ERR_RX_TIMEOUT)
    {
        estatisticas.timeouts++;

        Serial.print("Timeout de recepcao. Total: ");
        Serial.println(estatisticas.timeouts);
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
        Serial.println("Pacote recebido com erro de CRC.");

        tela.mostrarErro("ERRO DE CRC", state);

        delay(1500);
    }
    else
    {
        Serial.print("Erro durante a recepcao: ");
        Serial.println(state);

        tela.mostrarErro("ERRO RECEPCAO", state);

        delay(1500);
    }
}

void AplicacaoReceptor::atualizarTela(void* contexto, bool avancar)
{
    static_cast<AplicacaoReceptor*>(contexto)->tela.atualizar(avancar);
}

void AplicacaoReceptor::executarComunicacao(void* contexto)
{
    static_cast<AplicacaoReceptor*>(contexto)->executar();
}

void AplicacaoReceptor::atenderConfiguracao()
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
