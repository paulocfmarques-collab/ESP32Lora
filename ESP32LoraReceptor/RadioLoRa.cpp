#include "RadioLoRa.h"
#include "Configuracao.h"

RadioLoRa::RadioLoRa()
    : modulo(Configuracao::LORA_NSS, Configuracao::LORA_DIO1, Configuracao::LORA_RST,
             Configuracao::LORA_BUSY),
      dispositivo(&modulo)
{
}

int RadioLoRa::iniciar()
{
    return dispositivo.begin(915.0, 125.0, 9, 5, 0x12, 20, 8);
}

int RadioLoRa::standby()
{
    return dispositivo.standby();
}

int RadioLoRa::transmit(String& mensagem)
{
    return dispositivo.transmit(mensagem);
}

int RadioLoRa::receive(uint8_t* buffer, size_t tamanho, uint32_t timeout)
{
    return dispositivo.receive(buffer, tamanho, timeout);
}

size_t RadioLoRa::getPacketLength()
{
    return dispositivo.getPacketLength();
}

float RadioLoRa::getRSSI()
{
    return dispositivo.getRSSI();
}

float RadioLoRa::getSNR()
{
    return dispositivo.getSNR();
}

float RadioLoRa::getFrequencyError()
{
    return dispositivo.getFrequencyError();
}
