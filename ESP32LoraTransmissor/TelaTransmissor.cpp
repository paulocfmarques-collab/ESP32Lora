#include "TelaTransmissor.h"
#include "Configuracao.h"

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

void TelaTransmissor::atualizar(bool avancar)
{
    const uint32_t agora = millis();
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (avancar) pagina = (pagina + 1) % 3;
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

    desenharPagina(selecionada, estado, contadores);
    if (avancar)
    {
        Serial.print("OLED Transmissor - pagina ");
        Serial.print(selecionada + 1);
        Serial.println("/3 - quadro enviado");
    }
}

void TelaTransmissor::desenharPagina(uint8_t selecionada, const EstadoTela& estado,
                                const EstatisticasTransmissor& contadores)
{
    // Chamado exclusivamente pela tarefa OLED, no core 0.
    display.clearBuffer();
    display.setDrawColor(1);
    display.setFont(u8g2_font_6x10_tf);
    if (selecionada == 0)
    {
        display.drawStr(0, 10, estado.titulo.c_str());
        for (uint8_t i = 0; i < 3; ++i)
        {
            display.drawStr(0, 27 + i * 16, estado.linhas[i].c_str());
        }
    }
    else if (selecionada == 1)
    {
        display.drawStr(0, 10, "ESTATISTICAS");
        display.setCursor(0, 20);
        display.print("Confirmados: ");
        display.print(contadores.pacotesConfirmados);
        display.setCursor(0, 30);
        display.print("Sem ACK: ");
        display.print(contadores.pacotesNaoConfirmados);
        display.setCursor(0, 40);
        display.print("Reenvios: ");
        display.print(contadores.totalRetransmissoes);
        display.setCursor(0, 50);
        display.print("ACKs invalidos: ");
        display.print(contadores.acksInvalidos);
        display.setCursor(0, 60);
        display.print("Erros CRC: ");
        display.print(contadores.errosCRC);
    }
    else
    {
        display.drawStr(0, 10, "CONFIGURACAO");
        display.drawStr(0, 23, (String("ID: ") + estado.endereco).c_str());
        display.drawStr(0, 35, estado.rede.c_str());
        display.drawStr(0, 47, "Web: 192.168.4.1");
        display.drawStr(0, 59, "915 MHz / SF9 / CR4/5");
    }
    display.setDrawColor(0);
    display.drawBox(108, 0, 20, 12);
    display.setDrawColor(1);
    display.setCursor(110, 10);
    display.print(selecionada + 1);
    display.print("/3");
    display.sendBuffer();
}

void TelaTransmissor::configurarRede(const String& endereco, const String& rede)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    status.endereco = endereco;
    status.rede = rede;
    redesenhar = true;
    xSemaphoreGive(mutex);
}
