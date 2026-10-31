#pragma once
#include "RadioLoRa.h"
#include "ServidorConfiguracao.h"
#include "ControlePaginas.h"
#include "TarefaComunicacao.h"
#include "TelaTransmissor.h"

class AplicacaoTransmissor
{
  public:
    AplicacaoTransmissor();
    void iniciar();
    void atenderConfiguracao();

  private:
    void executar();
    static void executarComunicacao(void* contexto);
    static void atualizarTela(void* contexto, bool avancar);
    bool aguardarACK(unsigned long idPacote, uint8_t tentativa, const String& origemPacote);
    bool enviarComACK(unsigned long idPacote, const String& mensagem);
    EnderecoDispositivo endereco;
    ServidorConfiguracao configuracaoWeb;
    String enderecoMostrado;
    EstatisticasTransmissor estatisticas;
    RadioLoRa radio;
    TelaTransmissor tela;
    ControlePaginas controlePaginas;
    TarefaComunicacao comunicacao;
    unsigned long numeroPacote = 0;
};
