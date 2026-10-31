#pragma once

// Contadores da aplicacao, sem dependencias de hardware.
struct EstatisticasReceptor
{
    unsigned long pacotesNovos = 0;
    unsigned long pacotesDuplicados = 0;
    unsigned long acksEnviados = 0;
    unsigned long errosACK = 0;
    unsigned long timeouts = 0;
};
