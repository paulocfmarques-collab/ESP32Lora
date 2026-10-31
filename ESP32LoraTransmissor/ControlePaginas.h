#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// O botao e lido sem aguardar as operacoes do OLED.
class ControlePaginas
{
public:
    using AtualizarTela = void (*)(void*, bool);
    ControlePaginas(AtualizarTela atualizar, void* contexto);
    ControlePaginas(const ControlePaginas&) = delete;
    ControlePaginas& operator=(const ControlePaginas&) = delete;
    bool iniciar();

private:
    static void executarBotao(void* contexto);
    static void executarInterface(void* contexto);
    void lerBotao();
    AtualizarTela atualizar;
    void* contexto;
    TaskHandle_t tarefaInterface = nullptr;
    TaskHandle_t tarefaBotao = nullptr;
};
