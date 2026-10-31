#include "TelaTransmissor.h"
#include "Configuracao.h"
#include "../LayoutTela.h"

TelaTransmissor::TelaTransmissor(const EstatisticasTransmissor& contadores)
    : estatisticas(contadores),
      display(U8G2_R0, Configuracao::OLED_SCL, Configuracao::OLED_SDA, Configuracao::OLED_RST) {}

void TelaTransmissor::iniciar()
{
    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr)
    {
        Serial.println("Falha ao criar protecao do OLED.");
        while (true) delay(1000);
    }
    pinMode(Configuracao::OLED_VEXT, OUTPUT);
    digitalWrite(Configuracao::OLED_VEXT, LOW);
    delay(100);
    display.begin();
    display.setContrast(255);
    protecao.registrarAtividade(millis());
}

void TelaTransmissor::mostrarInicializacao()
{
    publicarStatus("HELTEC V3", "Transmissor LoRa", "Inicializando...");
}

void TelaTransmissor::mostrarPronto()
{
    publicarStatus("TRANSMISSOR LoRa", "Radio inicializado",
                   "Frequencia: 915", "ACK habilitado");
}

void TelaTransmissor::mostrarEnviando(unsigned long id, uint8_t tentativa)
{
    publicarStatus("ENVIANDO PACOTE", String("ID: ") + id,
                   String("Tentativa: ") + tentativa + "/" + Configuracao::MAX_TENTATIVAS,
                   "Transmitindo...");
}

void TelaTransmissor::mostrarAguardandoACK(unsigned long id, uint8_t tentativa)
{
    publicarStatus("AGUARDANDO ACK", String("Pacote: ") + id,
                   String("Tentativa: ") + tentativa,
                   String("Timeout: ") + (Configuracao::TIMEOUT_ACK / 1000) + " s");
}

void TelaTransmissor::mostrarACKRecebido(unsigned long id, float rssi, float snr, const String& receptor)
{
    publicarStatus("ACK RECEBIDO!", String("De: ") + receptor,
                   String("RSSI: ") + String(rssi, 0) + " dBm",
                   String("SNR: ") + String(snr, 1) + " dB");
}

void TelaTransmissor::mostrarACKInvalido(unsigned long esperado, unsigned long recebido)
{
    publicarStatus("ACK INCORRETO", String("Esperado: ") + esperado,
                   String("Recebido: ") + recebido, "Sera reenviado");
}

void TelaTransmissor::mostrarRetransmissao(unsigned long id, uint8_t proximaTentativa)
{
    publicarStatus("ACK NAO RECEBIDO", String("Pacote: ") + id,
                   String("Nova tentativa: ") + proximaTentativa, "Reenviando...");
}

void TelaTransmissor::mostrarFalha(unsigned long id)
{
    publicarStatus("FALHA NO ENVIO", String("Pacote: ") + id,
                   String("Tentativas: ") + Configuracao::MAX_TENTATIVAS, "Sem confirmacao");
}

void TelaTransmissor::mostrarErro(const char* titulo, int codigo)
{
    publicarStatus(titulo, String("Codigo: ") + codigo, "Veja o Serial");
}

void TelaTransmissor::publicarStatus(const char* titulo, const String& linha1,
                                const String& linha2, const String& linha3)
{
    // A tarefa LoRa publica dados; somente a tarefa OLED usa o objeto display.
    xSemaphoreTake(mutex, portMAX_DELAY);
    status.titulo = titulo;
    status.linhas[0] = linha1;
    status.linhas[1] = linha2;
    status.linhas[2] = linha3;
    estatisticasPagina = estatisticas;
    redesenhar = true;
    xSemaphoreGive(mutex);
}

void TelaTransmissor::sincronizarEstatisticas()
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    estatisticasPagina = estatisticas;
    xSemaphoreGive(mutex);
}

void TelaTransmissor::registrarAtividade()
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    protecao.registrarAtividade(millis());
    redesenhar = true;
    xSemaphoreGive(mutex);
}

void TelaTransmissor::atualizar(bool avancar)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    const uint32_t agora = millis();
    if (avancar)
    {
        redesenhar = true;
        avancar = protecao.tocar(agora);
    }
    if (status.otaAtiva)
        protecao.registrarAtividade(agora);
    const bool proteger = protecao.atualizar(agora);
    if (avancar && !status.otaAtiva) pagina = (pagina + 1) % Configuracao::TOTAL_PAGINAS_OLED;
    if (!primeiroDesenho && !redesenhar && !avancar &&
        uint32_t(agora - ultimaAtualizacao) < 250)
    {
        xSemaphoreGive(mutex);
        return;
    }
    // Copia consistente; o envio I2C nao segura o mutex da comunicacao.
    const uint8_t selecionada = pagina;
    const EstadoTela estado = status;
    const EstatisticasTransmissor contadores = estatisticasPagina;
    redesenhar = false;
    primeiroDesenho = false;
    ultimaAtualizacao = agora;
    xSemaphoreGive(mutex);

    if (proteger)
        ProtecaoTela::desenhar(display, agora);
    else
        desenharPagina(selecionada, estado, contadores);
    if (avancar && !estado.otaAtiva)
    {
        Serial.print("OLED Transmissor - pagina ");
        Serial.print(selecionada + 1);
        Serial.print("/");
        Serial.print(Configuracao::TOTAL_PAGINAS_OLED);
        Serial.println(" - quadro enviado");
    }
}

void TelaTransmissor::desenharPagina(uint8_t selecionada, const EstadoTela& estado,
                                const EstatisticasTransmissor& contadores)
{
    // Chamado exclusivamente pela tarefa OLED, no core 0.
    display.clearBuffer();
    display.setDrawColor(1);
    display.setFont(u8g2_font_5x8_tf);
    if (estado.otaAtiva)
    {
        LayoutTela::cabecalho(display, "ATUALIZANDO", LayoutTela::Icone::Atualizacao,
                             selecionada, Configuracao::TOTAL_PAGINAS_OLED);
        display.setCursor(52, 26);
        display.print(estado.progressoOTA);
        display.print("%");
        display.drawFrame(3, 32, 122, 16);
        const uint8_t largura = (118 * estado.progressoOTA) / 100;
        if (largura > 0) display.drawBox(5, 34, largura, 12);
        LayoutTela::texto(display, 2, 60, estado.mensagemOTA);
        display.sendBuffer();
        return;
    }
    const char* titulos[] = {"", "ESTATISTICAS", "REDE / WIFI", "RADIO LoRa"};
    const LayoutTela::Icone icones[] = {LayoutTela::Icone::Radio,
        LayoutTela::Icone::Estatisticas, LayoutTela::Icone::Rede, LayoutTela::Icone::Radio};
    LayoutTela::cabecalho(display, selecionada == 0 ? estado.titulo : String(titulos[selecionada]),
                         icones[selecionada], selecionada, Configuracao::TOTAL_PAGINAS_OLED);
    if (selecionada == 0)
    {
        LayoutTela::texto(display, 2, 24, String("GATEWAY  ") + estado.endereco);
        for (uint8_t i = 0; i < 3; ++i)
        {
            LayoutTela::texto(display, 2, 36 + i * 12, estado.linhas[i]);
        }
    }
    else if (selecionada == 1)
    {
        LayoutTela::campo(display, 24, "Confirmados", contadores.pacotesConfirmados);
        LayoutTela::campo(display, 33, "Sem ACK", contadores.pacotesNaoConfirmados);
        LayoutTela::campo(display, 42, "Reenvios", contadores.totalRetransmissoes);
        LayoutTela::campo(display, 51, "ACKs invalidos", contadores.acksInvalidos);
        LayoutTela::campo(display, 60, "Erros CRC", contadores.errosCRC);
    }
    else if (selecionada == 2)
    {
        LayoutTela::texto(display, 2, 24, String("ID: ") + estado.endereco);
        LayoutTela::texto(display, 2, 36, estado.rede);
        LayoutTela::texto(display, 2, 48, String("IP: ") + estado.ip);
        LayoutTela::texto(display, 2, 60, estado.conectada ? "WiFi: conectado" : "WiFi: acesso local");
    }
    else
    {
        display.setCursor(2, 24);
        display.print("Freq: ");
        display.print(Configuracao::FREQUENCIA_MHZ, 1);
        display.print(" MHz");
        display.setCursor(2, 33);
        display.print("BW: ");
        display.print(Configuracao::LARGURA_BANDA_KHZ, 1);
        display.print(" kHz");
        display.setCursor(2, 42);
        display.print("SF: ");
        display.print(Configuracao::SPREADING_FACTOR);
        display.print(" / CR: 4/");
        display.print(Configuracao::CODING_RATE);
        display.setCursor(2, 51);
        display.print("Sync:0x");
        display.print(Configuracao::SYNC_WORD, HEX);
        display.print(" / ");
        display.print(Configuracao::POTENCIA_DBM);
        display.print("dBm");
        display.setCursor(2, 60);
        display.print("Preambulo: ");
        display.print(Configuracao::PREAMBULO);
    }
    display.sendBuffer();
}

void TelaTransmissor::configurarRede(const String& endereco, const String& rede,
                                    const String& ip, bool conectada)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    status.endereco = endereco;
    status.rede = rede;
    status.ip = ip;
    status.conectada = conectada;
    redesenhar = true;
    xSemaphoreGive(mutex);
}

void TelaTransmissor::mostrarProgressoOTA(uint8_t percentual, const String& mensagem, bool ativo)
{
    // Callback do upload publica dados; o desenho continua na tarefa OLED do core 0.
    xSemaphoreTake(mutex, portMAX_DELAY);
    status.otaAtiva = ativo;
    status.progressoOTA = percentual > 100 ? 100 : percentual;
    status.mensagemOTA = mensagem;
    redesenhar = true;
    xSemaphoreGive(mutex);
}

void TelaTransmissor::mostrarGateway(const String& fase, const String& detalhe)
{
    publicarStatus("GATEWAY LoRa", fase, detalhe,
                   String("Consultas OK: ") + estatisticas.pacotesConfirmados);
}
