#include "ComandosUDP.h"
#include "Configuracao.h"
#include "TelaTransmissor.h"
#include <WiFi.h>

ComandosUDP::ComandosUDP(EnderecoDispositivo& endereco, DiagnosticoDispositivo& diagnostico,
                         CanalGateway& canal, TelaTransmissor& tela)
    : endereco(endereco), diagnostico(diagnostico), canal(canal), tela(tela)
{
}

bool ComandosUDP::iniciar()
{
    if (tarefa != nullptr)
        return true;
    diagnostico.iniciar();
    return xTaskCreatePinnedToCore(executarTarefa, "consultas_udp", 8192, this, 1, &tarefa,
                                   Configuracao::CORE_INTERFACE) == pdPASS;
}

void ComandosUDP::executarTarefa(void* contexto)
{
    static_cast<ComandosUDP*>(contexto)->executar();
}

void ComandosUDP::executar()
{
    ultimaRede = millis() - 5000;
    for (;;)
    {
        diagnostico.atualizar();
        atenderRede();
        if (aberto)
        {
            atenderPacotes();
            devolverRespostas();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void ComandosUDP::atenderRede()
{
    if (uint32_t(millis() - ultimaRede) < (aberto ? 1000U : 5000U))
        return;
    ultimaRede = millis();
    const uint32_t novoSTA = WiFi.status() == WL_CONNECTED ? uint32_t(WiFi.localIP()) : 0;
    const uint32_t novoAP = (WiFi.getMode() & WIFI_AP) ? uint32_t(WiFi.softAPIP()) : 0;
    if (aberto && novoSTA == ipSTA && novoAP == ipAP)
        return;
    udp.stop();
    aberto = false;
    ipSTA = novoSTA;
    ipAP = novoAP;
    if (!novoSTA && !novoAP)
        return;
    aberto = udp.begin(Configuracao::PORTA_UDP) == 1;
    if (aberto)
    {
        Serial.print("[UDP] Gateway disponivel na porta ");
        Serial.println(Configuracao::PORTA_UDP);
    }
    else
    {
        ++erros;
        Serial.println("[UDP] Falha no socket; nova tentativa em 5 segundos.");
    }
}

String ComandosUDP::consultaLocal(const String& comando)
{
    if (comando == "udp_status")
        return String("UDP_STATUS\nPorta: ") + Configuracao::PORTA_UDP +
               "\nRecebidos: " + recebidos + "\nEnviados: " + enviados +
               "\nErros socket envio: " + erros + "\n";
    return diagnostico.consultar(comando);
}

void ComandosUDP::responder(const String& texto, const IPAddress& ip, uint16_t porta)
{
    const String resposta =
        texto.length() <= MAX_RESPOSTA ? texto : String("ERRO: resposta excedeu 1400 bytes.\n");
    if (udp.beginPacket(ip, porta) != 1)
    {
        ++erros;
        return;
    }
    const size_t escritos =
        udp.write(reinterpret_cast<const uint8_t*>(resposta.c_str()), resposta.length());
    const bool enviado = udp.endPacket() == 1;
    if (escritos == resposta.length() && enviado)
        ++enviados;
    else
        ++erros;
}

void ComandosUDP::devolverRespostas()
{
    CanalGateway::Resposta resposta;
    for (uint8_t i = 0; i < 4 && canal.receberResposta(resposta); ++i)
        responder(String(resposta.texto), IPAddress(resposta.ip), resposta.porta);
}

void ComandosUDP::atenderPacotes()
{
    for (uint8_t i = 0; i < 4; ++i)
    {
        const int tamanho = udp.parsePacket();
        if (tamanho <= 0)
            return;
        const IPAddress origem = udp.remoteIP();
        const uint16_t porta = udp.remotePort();
        ++recebidos;
        tela.registrarAtividade();
        if (tamanho > int(MAX_COMANDO))
        {
            udp.clear();
            responder("ERRO: comando excede 255 bytes.\n", origem, porta);
            continue;
        }
        char buffer[MAX_COMANDO + 1] = {};
        const int lidos = udp.read(reinterpret_cast<uint8_t*>(buffer), tamanho);
        bool valido = lidos == tamanho;
        for (int j = 0; j < lidos; ++j)
        {
            const uint8_t c = uint8_t(buffer[j]);
            if ((c < 32 && c != '\r' && c != '\n' && c != '\t') || c > 126)
                valido = false;
        }
        if (!valido)
        {
            responder("ERRO: envie um comando ASCII por datagrama.\n", origem, porta);
            continue;
        }
        String comando(buffer);
        comando.trim();
        comando.toLowerCase();
        const int separador = comando.indexOf(':');
        if (separador < 0)
        {
            responder(consultaLocal(comando), origem, porta);
            continue;
        }
        String destino = comando.substring(separador + 1);
        destino.trim();
        destino.toUpperCase();
        comando = comando.substring(0, separador);
        comando.trim();
        if (comando.isEmpty() || comando.length() > Configuracao::RPC_MAX_COMANDO ||
            !EnderecoDispositivo::valido(destino))
        {
            responder("ERRO: use comando:RX01, sem broadcast.\n", origem, porta);
            continue;
        }
        if (destino == endereco.obter())
        {
            responder(consultaLocal(comando), origem, porta);
            continue;
        }
        if (!canal.pronto())
        {
            responder("ERRO: radio do gateway indisponivel.\n", origem, porta);
            continue;
        }
        if (!canal.enfileirar(comando, destino, uint32_t(origem), porta))
            responder("ERRO: fila do gateway cheia. Tente novamente.\n", origem, porta);
        // A resposta sera enviada ao mesmo cliente apos a tarefa LoRa terminar a consulta.
    }
}
