#pragma once
#include "../RadioLoRa.h"
#include "ComandosSerial.h"
#include "ServidorConfiguracao.h"
#include "../ControlePaginas.h"
#include "../TarefaComunicacao.h"
#include "TelaReceptor.h"
#include "../MonitorComunicacao.h"
#include "../DiagnosticoDispositivo.h"
#include "../TransporteLoRa.h"

class AplicacaoReceptor
{
public:
    AplicacaoReceptor();
    void iniciar();
    void atenderConfiguracao();
private:
    void executar();
    bool enviarResposta(const ProtocoloLoRa::Quadro& pedido, const String& resposta);
    bool aguardarConfirmacao(const ProtocoloLoRa::Quadro& pedido, uint8_t parte);
    void publicarEstado(const char* fase, const String& peer = String());
    static void executarComunicacao(void* contexto);
    static void atualizarTela(void* contexto, bool avancar);
    EnderecoDispositivo endereco;
    ComandosSerial comandosSerial;
    ServidorConfiguracao configuracaoWeb;
    EstatisticasReceptor estatisticas;
    EstatisticasComunicacao estatisticasRPC;
    RadioLoRa radio;
    TelaReceptor tela;
    ControlePaginas controlePaginas;
    TarefaComunicacao comunicacao;
    MonitorComunicacao monitor;
    DiagnosticoDispositivo diagnostico;
    TransporteLoRa transporte;
    String enderecoMostrado;
    bool temCache = false;
    ProtocoloLoRa::Quadro pedidoCache;
    String respostaCache;
    uint32_t numeroPacote = 0;
    uint8_t tentativaAtual = 0;
};
