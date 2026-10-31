#include "TelaReceptor.h"
#include "Configuracao.h"
#include "../LayoutTela.h"

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
    protecao.registrarAtividade(millis());
}

void TelaReceptor::mostrarInicializacao()
{
    publicarStatus("HELTEC V3", "Receptor LoRa", "Comandos via gateway");
}

void TelaReceptor::mostrarAguardando()
{
    publicarStatus("RECEPTOR LoRa", "Esperando comandos",
                   String("Consultas: ") + estatisticas.comandosRecebidos,
                   String("Respostas: ") + estatisticas.respostasEnviadas);
}

void TelaReceptor::mostrarPacote(unsigned long id, const String& mensagem, float rssi,
                                 bool duplicado)
{
    publicarStatus(duplicado ? "COMANDO REPETIDO" : "COMANDO RECEBIDO", String("ID: ") + id,
                   String("Cmd: ") + mensagem.substring(0, 14),
                   String("RSSI: ") + String(rssi, 0) + " dBm");
}

void TelaReceptor::mostrarEnviandoResposta(unsigned long id)
{
    publicarStatus("ENVIANDO RESPOSTA", String("Pacote: ") + id, "Aguarde...");
}

void TelaReceptor::mostrarRespostaEnviada(unsigned long id, bool duplicado)
{
    publicarStatus("RESPOSTA ENVIADA", String("Pacote: ") + id,
                   duplicado ? "Tipo: repetido" : "Tipo: novo",
                   String("Total: ") + estatisticas.respostasEnviadas);
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

void TelaReceptor::registrarAtividade()
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    protecao.registrarAtividade(millis());
    redesenhar = true;
    xSemaphoreGive(mutex);
}

void TelaReceptor::atualizar(bool avancar)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    const uint32_t agora = millis();
    if (avancar)
    {
        redesenhar = true;
        avancar = protecao.tocar(agora);
    }
    const bool proteger = protecao.atualizar(agora);
    if (avancar)
        pagina = (pagina + 1) % Configuracao::TOTAL_PAGINAS_OLED;
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

    if (proteger)
        ProtecaoTela::desenhar(display, agora);
    else
        desenharPagina(selecionada, estado, contadores);
    if (avancar)
    {
        Serial.print("OLED Receptor - pagina ");
        Serial.print(selecionada + 1);
        Serial.print("/");
        Serial.print(Configuracao::TOTAL_PAGINAS_OLED);
        Serial.println(" - quadro enviado");
    }
}

void TelaReceptor::desenharPagina(uint8_t selecionada, const EstadoTela& estado,
                                  const EstatisticasReceptor& contadores)
{
    // Chamado exclusivamente pela tarefa OLED, no core 0.
    const char* titulos[] = {"", "ESTATISTICAS", "REDE / WIFI", "RADIO LoRa"};
    const LayoutTela::Icone icones[] = {LayoutTela::Icone::Radio,
        LayoutTela::Icone::Estatisticas, LayoutTela::Icone::Rede, LayoutTela::Icone::Radio};
    LayoutTela::cabecalho(display, selecionada == 0 ? estado.titulo : String(titulos[selecionada]),
                         icones[selecionada], selecionada, Configuracao::TOTAL_PAGINAS_OLED);
    if (selecionada == 0)
    {
        LayoutTela::texto(display, 2, 24, String("RECEPTOR  ") + estado.endereco);
        for (uint8_t i = 0; i < 3; ++i)
        {
            LayoutTela::texto(display, 2, 36 + i * 12, estado.linhas[i]);
        }
    }
    else if (selecionada == 1)
    {
        LayoutTela::campo(display, 24, "Consultas", contadores.comandosRecebidos);
        LayoutTela::campo(display, 33, "Repetidos", contadores.pacotesDuplicados);
        LayoutTela::campo(display, 42, "Respostas", contadores.respostasEnviadas);
        LayoutTela::campo(display, 51, "Falhas", contadores.errosResposta);
        LayoutTela::campo(display, 60, "Timeouts", contadores.timeouts);
    }
    else if (selecionada == 2)
    {
        LayoutTela::texto(display, 2, 24, String("ID: ") + estado.endereco);
        LayoutTela::texto(display, 2, 36, estado.rede);
        LayoutTela::texto(display, 2, 48, "Web: 192.168.4.1");
        LayoutTela::texto(display, 2, 60, "WiFi: acesso local");
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

void TelaReceptor::configurarRede(const String& endereco, const String& rede)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    status.endereco = endereco;
    status.rede = rede;
    redesenhar = true;
    xSemaphoreGive(mutex);
}
