#pragma once
#include <Arduino.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../EnderecoDispositivo.h"
#include "../DiagnosticoDispositivo.h"
#include "CanalGateway.h"
class TelaTransmissor;

class ComandosUDP
{
public:
    ComandosUDP(EnderecoDispositivo& endereco, DiagnosticoDispositivo& diagnostico,
                CanalGateway& canal, TelaTransmissor& tela);
    bool iniciar();
private:
    static void executarTarefa(void* contexto);
    void executar();
    void atenderRede();
    void atenderPacotes();
    void devolverRespostas();
    void responder(const String& texto, const IPAddress& ip, uint16_t porta);
    String consultaLocal(const String& comando);
    EnderecoDispositivo& endereco;
    DiagnosticoDispositivo& diagnostico;
    CanalGateway& canal;
    TelaTransmissor& tela;
    WiFiUDP udp;
    TaskHandle_t tarefa = nullptr;
    bool aberto = false;
    uint32_t ipSTA = 0, ipAP = 0, ultimaRede = 0;
    uint32_t recebidos = 0, enviados = 0, erros = 0;
    static constexpr size_t MAX_COMANDO = 255;
    static constexpr size_t MAX_RESPOSTA = 1400;
};
