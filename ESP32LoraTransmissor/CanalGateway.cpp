#include "CanalGateway.h"
#include "Configuracao.h"
#include <esp_system.h>
static_assert(Configuracao::RPC_MAX_COMANDO <= 128, "Aumente o buffer da fila para comandos maiores.");
static_assert(Configuracao::RPC_TAMANHO_RESPOSTA <= 1400, "Aumente o buffer da fila para respostas maiores.");

bool CanalGateway::iniciar()
{
    pedidos = xQueueCreate(4, sizeof(Pedido));
    respostas = xQueueCreate(4, sizeof(Resposta));
    if (!pedidos || !respostas)
    {
        if (pedidos) vQueueDelete(pedidos);
        if (respostas) vQueueDelete(respostas);
        pedidos = respostas = nullptr;
        return false;
    }
    proximoID = esp_random();
    return true;
}
void CanalGateway::definirPronto(bool valor)
{
    portENTER_CRITICAL(&mutex); disponivel = valor && pedidos && respostas; portEXIT_CRITICAL(&mutex);
}
bool CanalGateway::pronto() const
{
    portENTER_CRITICAL(&mutex); const bool valor = disponivel; portEXIT_CRITICAL(&mutex); return valor;
}
bool CanalGateway::enfileirar(const String& comando, const String& destino, uint32_t ip, uint16_t porta)
{
    if (!pronto() || !pedidos || comando.length() > Configuracao::RPC_MAX_COMANDO) return false;
    Pedido pedido;
    if (++proximoID == 0) ++proximoID;
    pedido.id = proximoID; pedido.criadoEm = millis(); pedido.ip = ip; pedido.porta = porta;
    comando.toCharArray(pedido.comando, sizeof(pedido.comando));
    destino.toCharArray(pedido.destino, sizeof(pedido.destino));
    return xQueueSend(pedidos, &pedido, 0) == pdTRUE;
}
bool CanalGateway::proximo(Pedido& pedido)
{
    return pedidos && xQueueReceive(pedidos, &pedido, portMAX_DELAY) == pdTRUE;
}
bool CanalGateway::responder(const Pedido& pedido, const String& texto)
{
    if (!respostas) return false;
    Resposta resposta;
    resposta.ip = pedido.ip; resposta.porta = pedido.porta;
    const String limitada = texto.length() <= Configuracao::RPC_TAMANHO_RESPOSTA ? texto : String("ERRO: resposta LoRa excedeu o limite.\n");
    limitada.toCharArray(resposta.texto, sizeof(resposta.texto));
    return xQueueSend(respostas, &resposta, pdMS_TO_TICKS(100)) == pdTRUE;
}
bool CanalGateway::receberResposta(Resposta& resposta)
{
    return respostas && xQueueReceive(respostas, &resposta, 0) == pdTRUE;
}
