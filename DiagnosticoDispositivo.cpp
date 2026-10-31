#include "DiagnosticoDispositivo.h"
#include <WiFi.h>
#include <esp_ota_ops.h>
#include <cmath>

DiagnosticoDispositivo::DiagnosticoDispositivo(EnderecoDispositivo& endereco, RadioLoRa& radio,
                                               MonitorComunicacao& monitor,
                                               ServidorConfiguracao& web, bool udpLocal)
    : endereco(endereco), radio(radio), monitor(monitor), web(web), udpLocal(udpLocal)
{
}

void DiagnosticoDispositivo::iniciar()
{
    bateria.iniciar();
    flashBytes = ESP.getFlashChipSize();
    flashHz = ESP.getFlashChipSpeed();
    sketchBytes = ESP.getSketchSize();
    const auto* atual = esp_ota_get_running_partition();
    const auto* proxima = esp_ota_get_next_update_partition(nullptr);
    slotOTA = atual && proxima && atual->address != proxima->address ? proxima->size : 0;
    ultimaTemperatura = millis() - 1000;
}

void DiagnosticoDispositivo::atualizar()
{
    bateria.atualizar();
    if (uint32_t(millis() - ultimaTemperatura) < 1000)
        return;
    ultimaTemperatura = millis();
    const float nova = temperatureRead();
    portENTER_CRITICAL(&mutexSensor);
    temperatura = nova;
    portEXIT_CRITICAL(&mutexSensor);
}

float DiagnosticoDispositivo::temperaturaAtual() const
{
    portENTER_CRITICAL(&mutexSensor);
    const float copia = temperatura;
    portEXIT_CRITICAL(&mutexSensor);
    return copia;
}

String DiagnosticoDispositivo::json(const String& texto)
{
    String saida = "\"";
    for (size_t i = 0; i < texto.length(); ++i)
    {
        const uint8_t c = texto[i];
        if (c == '"')
            saida += "\\\"";
        else if (c == '\\')
            saida += "\\\\";
        else if (c < 32)
        {
            char escape[7];
            snprintf(escape, sizeof(escape), "\\u%04x", unsigned(c));
            saida += escape;
        }
        else
            saida += char(c);
    }
    return saida + "\"";
}

String DiagnosticoDispositivo::numero(float valor, uint8_t casas)
{
    return std::isfinite(valor) ? String(valor, static_cast<unsigned int>(casas)) : String("null");
}

const char* DiagnosticoDispositivo::modoOTA(ModoOTA modo)
{
    switch (modo)
    {
    case ModoOTA::Web:
        return "web";
    case ModoOTA::IDE:
        return "arduino_ide";
    case ModoOTA::Reiniciando:
        return "reiniciando";
    case ModoOTA::Falha:
        return "falha";
    default:
        return "ocioso";
    }
}

IPAddress DiagnosticoDispositivo::ipAcesso() const
{
    const IPAddress estacao = WiFi.localIP();
    return WiFi.status() == WL_CONNECTED && uint32_t(estacao) != 0 ? estacao : WiFi.softAPIP();
}
