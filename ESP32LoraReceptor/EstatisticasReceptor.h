#pragma once

// Contadores da aplicacao, sem dependencias de hardware.
struct EstatisticasReceptor
{
    unsigned long comandosRecebidos = 0;
    unsigned long pacotesDuplicados = 0;
    unsigned long respostasEnviadas = 0;
    unsigned long errosResposta = 0;
    unsigned long timeouts = 0;
};
