#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Mantem o fluxo LoRa no core dedicado a comunicacao.
class TarefaComunicacao
{
public:
    using ExecutarCiclo = void (*)(void*);
    TarefaComunicacao(ExecutarCiclo ciclo, void* contexto);
    TarefaComunicacao(const TarefaComunicacao&) = delete;
    TarefaComunicacao& operator=(const TarefaComunicacao&) = delete;
    bool iniciar();

private:
    static void executarTarefa(void* contexto);
    ExecutarCiclo ciclo;
    void* contexto;
    TaskHandle_t tarefa = nullptr;
};
