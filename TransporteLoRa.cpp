#include "TransporteLoRa.h"
int TransporteLoRa::enviar(const String& quadro)
{
    if (quadro.isEmpty() || quadro.length() > 255) return FORMATO_INVALIDO;
    const int codigo = radio.standby();
    if (codigo != RADIOLIB_ERR_NONE) return codigo;
    String texto = quadro;
    return radio.transmit(texto);
}

int TransporteLoRa::receber(ProtocoloLoRa::Quadro& quadro, uint32_t timeout)
{
    uint8_t buffer[256] = {};
    const int codigo = radio.receive(buffer, 255, timeout);
    if (codigo != RADIOLIB_ERR_NONE) return codigo;
    size_t tamanho = radio.getPacketLength();
    if (tamanho > 255) return FORMATO_INVALIDO;
    buffer[tamanho] = 0;
    const String pacote(reinterpret_cast<const char*>(buffer));
    if (pacote.length() != tamanho || !ProtocoloLoRa::interpretar(pacote, quadro)) return FORMATO_INVALIDO;
    return RADIOLIB_ERR_NONE;
}
