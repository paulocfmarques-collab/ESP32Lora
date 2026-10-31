#pragma once
#include <Arduino.h>
#include <cmath>
#include <freertos/FreeRTOS.h>

// Amostragem sem esperas bloqueantes; leitura por snapshot entre tarefas.
class LeitorBateria
{
public:
    struct Amostra { bool valida = false; float volts = NAN; uint32_t adcMV = 0, lidaEm = 0; };
    void iniciar();
    void atualizar();
    Amostra obter() const;
private:
    mutable portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
    Amostra dados;
    bool medindo = false;
    uint8_t amostras = 0;
    uint64_t soma = 0;
    uint32_t inicio = 0, ultimaAmostra = 0;
};
