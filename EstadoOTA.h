#pragma once
#include <Arduino.h>
enum class ModoOTA { Disponivel, Web, IDE, Reiniciando, Falha };
struct EstadoOTA
{
    ModoOTA modo = ModoOTA::Disponivel;
    uint8_t progresso = 0;
    bool ocupado = false;
    bool ideDisponivel = false;
    bool implementado = false;
    size_t capacidade = 0;
};
