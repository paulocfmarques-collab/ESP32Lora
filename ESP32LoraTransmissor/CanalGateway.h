#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Filas limitadas preservam o cliente de origem de cada consulta concorrente.
class CanalGateway
{
public:
    struct Pedido
    {
        uint32_t id = 0, criadoEm = 0, ip = 0;
        uint16_t porta = 0;
        char destino[17] = {};
        char comando[129] = {};
    };
    struct Resposta { uint32_t ip = 0; uint16_t porta = 0; char texto[1401] = {}; };
    bool iniciar();
    void definirPronto(bool valor);
    bool pronto() const;
    bool enfileirar(const String& comando, const String& destino, uint32_t ip, uint16_t porta);
    bool proximo(Pedido& pedido);
    bool responder(const Pedido& pedido, const String& texto);
    bool receberResposta(Resposta& resposta);
private:
    QueueHandle_t pedidos = nullptr, respostas = nullptr;
    uint32_t proximoID = 0;
    mutable portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
    bool disponivel = false;
};
