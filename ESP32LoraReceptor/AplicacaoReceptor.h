#pragma once
#include "RadioLoRa.h"
#include "ServidorConfiguracao.h"
#include "ControlePaginas.h"
#include "TarefaComunicacao.h"
#include "TelaReceptor.h"

class AplicacaoReceptor
{
public:
    AplicacaoReceptor();
    void iniciar();
    void atenderConfiguracao();

private:
    void executar();
    static void executarComunicacao(void* contexto);
    static void atualizarTela(void* contexto, bool avancar);
    bool enviarACK(unsigned long idPacote, bool duplicado, const String& origemTransmissor);
    EnderecoDispositivo endereco;
    ServidorConfiguracao configuracaoWeb;
    String enderecoMostrado;
    EstatisticasReceptor estatisticas;
    RadioLoRa radio;
    TelaReceptor tela;
    ControlePaginas controlePaginas;
    TarefaComunicacao comunicacao;
    unsigned long ultimoIDRecebido = 0;
    bool existeUltimoID = false;
    String ultimaOrigemRecebida;
};
