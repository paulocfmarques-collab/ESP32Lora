#pragma once
#include "RadioLoRa.h"
#include "ProtocoloLoRa.h"

// Usado somente pela tarefa LoRa, nunca pelo socket UDP ou pelo servidor web.
class TransporteLoRa
{
public:
    static constexpr int FORMATO_INVALIDO = -32000;
    explicit TransporteLoRa(RadioLoRa& radio) : radio(radio) {}
    int enviar(const String& quadro);
    int receber(ProtocoloLoRa::Quadro& quadro, uint32_t timeout);
private:
    RadioLoRa& radio;
};
