#pragma once
#include "../RadioLoRa.h"
#include "ServidorConfiguracao.h"
#include "../ControlePaginas.h"
#include "../TarefaComunicacao.h"
#include "TelaTransmissor.h"
#include "../MonitorComunicacao.h"
#include "../DiagnosticoDispositivo.h"
#include "CanalGateway.h"
#include "ComandosUDP.h"
#include "../TransporteLoRa.h"

class AplicacaoTransmissor
{
public:
    AplicacaoTransmissor();
    void iniciar();
    void atenderConfiguracao();
private:
    void executar();
    bool consultarReceptor(const CanalGateway::Pedido& pedido, const String& origem, String& resposta);
    void confirmar(const CanalGateway::Pedido& pedido, const String& origem, uint8_t parte);
    void concluirConfirmacoes(const CanalGateway::Pedido& pedido, const String& origem, uint8_t parte);
    void publicarEstado(const char* fase, const String& receptor = String());
    static void executarComunicacao(void* contexto);
    static void atualizarOTA(void* contexto, uint8_t percentual, const String& mensagem, bool ativo);
    static void atualizarTela(void* contexto, bool avancar);
    EnderecoDispositivo endereco;
    ServidorConfiguracao configuracaoWeb;
    EstatisticasTransmissor estatisticas;
    RadioLoRa radio;
    TelaTransmissor tela;
    ControlePaginas controlePaginas;
    TarefaComunicacao comunicacao;
    MonitorComunicacao monitor;
    DiagnosticoDispositivo diagnostico;
    CanalGateway canal;
    ComandosUDP comandosUDP;
    TransporteLoRa transporte;
    String enderecoMostrado, redeMostrada, ipMostrado;
    bool redeConectada = false;
    uint32_t ultimaAtualizacaoRede = 0;
    uint32_t numeroPacote = 0;
    uint8_t tentativaAtual = 0, ultimaParte = 0;
};
