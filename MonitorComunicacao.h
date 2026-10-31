#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include "EstatisticasComunicacao.h"
#include <cstring>

// Snapshot POD: nenhuma String e compartilhada entre as tarefas.
class MonitorComunicacao
{
public:
    struct Estado
    {
        EstatisticasComunicacao contadores;
        unsigned long pacote = 0;
        uint8_t tentativa = 0;
        char fase[24] = "inicializando";
        char ultimoPeer[17] = {};
        unsigned long ultimoACK = 0;
        bool temResultado = false;
        bool confirmado = false;
        uint32_t atualizadoEm = 0;
    };
    void publicar(const EstatisticasComunicacao& contadores, unsigned long pacote,
                  uint8_t tentativa, const char* fase, const String& receptor = String())
    {
        char nome[17] = {};
        receptor.toCharArray(nome, sizeof(nome));
        const uint32_t agora = millis();
        portENTER_CRITICAL(&mutex);
        dados.contadores = contadores;
        dados.pacote = pacote;
        dados.tentativa = tentativa;
        strncpy(dados.fase, fase, sizeof(dados.fase) - 1);
        dados.fase[sizeof(dados.fase) - 1] = 0;
        if (nome[0]) { memcpy(dados.ultimoPeer, nome, sizeof(nome)); dados.ultimoACK = pacote; }
        if (strcmp(fase, "novo_pacote") == 0) dados.temResultado = false;
        if (strcmp(fase, "confirmado") == 0 || strcmp(fase, "falha") == 0)
        { dados.temResultado = true; dados.confirmado = strcmp(fase, "confirmado") == 0; }
        dados.atualizadoEm = agora;
        portEXIT_CRITICAL(&mutex);
    }
    Estado obter() const
    {
        portENTER_CRITICAL(&mutex);
        const Estado copia = dados;
        portEXIT_CRITICAL(&mutex);
        return copia;
    }
private:
    mutable portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
    Estado dados;
};
