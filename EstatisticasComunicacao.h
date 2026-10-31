#pragma once

// Contadores da aplicacao, sem dependencias de hardware.
struct EstatisticasComunicacao
{
    unsigned long pacotesConfirmados = 0;
    unsigned long pacotesNaoConfirmados = 0;
    unsigned long totalRetransmissoes = 0;
    unsigned long acksInvalidos = 0;
    unsigned long errosCRC = 0;
    unsigned long timeoutsACK = 0;
    unsigned long errosTransmissao = 0;
    unsigned long comandosRecebidos = 0;
    unsigned long respostasEnviadas = 0;
    unsigned long fragmentosRecebidos = 0;
    unsigned long fragmentosEnviados = 0;
    unsigned long pacotesDuplicados = 0;
};
