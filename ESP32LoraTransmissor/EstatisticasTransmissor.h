#pragma once

// Contadores da aplicacao, sem dependencias de hardware.
struct EstatisticasTransmissor
{
    unsigned long pacotesConfirmados = 0;
    unsigned long pacotesNaoConfirmados = 0;
    unsigned long totalRetransmissoes = 0;
    unsigned long acksInvalidos = 0;
    unsigned long errosCRC = 0;
};
