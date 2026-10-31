#pragma once
#include <Arduino.h>
#include <RadioLib.h>

// Encapsula o SX1262 e sua configuracao na Heltec V3.
class RadioLoRa
{
  public:
    RadioLoRa();
    RadioLoRa(const RadioLoRa&) = delete;
    RadioLoRa& operator=(const RadioLoRa&) = delete;
    int iniciar();
    int standby();
    int transmit(String& mensagem);
    int receive(uint8_t* buffer, size_t tamanho, uint32_t timeout);
    size_t getPacketLength();
    float getRSSI();
    float getSNR();
    float getFrequencyError();

  private:
    Module modulo;
    SX1262 dispositivo;
};
