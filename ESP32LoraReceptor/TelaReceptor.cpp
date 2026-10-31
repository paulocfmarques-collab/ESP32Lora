#include "TelaReceptor.h"
#include "Configuracao.h"

TelaReceptor::TelaReceptor(const EstatisticasReceptor& contadores)
    : estatisticas(contadores),
      display(U8G2_R0, Configuracao::OLED_SCL, Configuracao::OLED_SDA, Configuracao::OLED_RST)
{
}

void TelaReceptor::iniciar()
{
    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr)
    {
        Serial.println("Falha ao criar protecao do OLED.");
        while (true)
            delay(1000);
    }
    pinMode(Configuracao::OLED_VEXT, OUTPUT);
    digitalWrite(Configuracao::OLED_VEXT, LOW);
    delay(100);
    display.begin();
    display.setContrast(255);
}

void TelaReceptor::mostrarInicializacao()
{
    publicarStatus("HELTEC V3", "Receptor LoRa", "Com resposta ACK");
}

void TelaReceptor::mostrarAguardando()
{
    publicarStatus("RECEPTOR LoRa", "Aguardando pacote",
                   String("Recebidos: ") + estatisticas.pacotesNovos,
                   String("ACKs: ") + estatisticas.acksEnviados);
}

void TelaReceptor::mostrarPacote(unsigned long id, const String& mensagem, float rssi,
                                 bool duplicado)
{
    publicarStatus(duplicado ? "PACOTE REPETIDO" : "PACOTE RECEBIDO", String("ID: ") + id,
                   String("Msg: ") + mensagem.substring(0, 14),
                   String("RSSI: ") + String(rssi, 0) + " dBm");
}

void TelaReceptor::mostrarEnviandoACK(unsigned long id)
{
    publicarStatus("ENVIANDO ACK", String("Pacote: ") + id, "Aguarde...");
}

void TelaReceptor::mostrarACKEnviado(unsigned long id, bool duplicado)
{
    publicarStatus("ACK ENVIADO!", String("Pacote: ") + id,
                   duplicado ? "Tipo: repetido" : "Tipo: novo",
                   String("Total: ") + estatisticas.acksEnviados);
}

void TelaReceptor::mostrarErro(const char* titulo, int codigo)
{
    publicarStatus(titulo, String("Codigo: ") + codigo, "Veja o Serial");
}

void TelaReceptor::publicarStatus(const char* titulo, const String& linha1, const String& linha2,
                                  const String& linha3)
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

void TelaReceptor::sincronizarEstatisticas()
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    estatisticasPagina = estatisticas;
    xSemaphoreGive(mutex);
}

void TelaReceptor::atualizar(bool avancar)
{
    const uint32_t agora = millis();
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (avancar)
        pagina = (pagina + 1) % 3;
    if (!primeiroDesenho && !redesenhar && !avancar && uint32_t(agora - ultimaAtualizacao) < 250)
    {
        xSemaphoreGive(mutex);
        return;
    }
    // Copia consistente; o envio I2C nao segura o mutex da comunicacao.
    const uint8_t selecionada = pagina;
    const EstadoTela estado = status;
    const EstatisticasReceptor contadores = estatisticasPagina;
    redesenhar = false;
    primeiroDesenho = false;
    ultimaAtualizacao = agora;
    xSemaphoreGive(mutex);

    desenharPagina(selecionada, estado, contadores);
    if (avancar)
    {
        Serial.print("OLED Receptor - pagina ");
        Serial.print(selecionada + 1);
        Serial.println("/3 - quadro enviado");
    }
}

void TelaReceptor::desenharPagina(uint8_t selecionada, const EstadoTela& estado,
                                  const EstatisticasReceptor& contadores)
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
        display.print("Novos: ");
        display.print(contadores.pacotesNovos);
        display.setCursor(0, 30);
        display.print("Repetidos: ");
        display.print(contadores.pacotesDuplicados);
        display.setCursor(0, 40);
        display.print("ACKs: ");
        display.print(contadores.acksEnviados);
        display.setCursor(0, 50);
        display.print("Erros ACK: ");
        display.print(contadores.errosACK);
        display.setCursor(0, 60);
        display.print("Timeouts: ");
        display.print(contadores.timeouts);
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

void TelaReceptor::configurarRede(const String& endereco, const String& rede)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    status.endereco = endereco;
    status.rede = rede;
    redesenhar = true;
    xSemaphoreGive(mutex);
}
