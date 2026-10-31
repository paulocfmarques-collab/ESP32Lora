#include "AplicacaoTransmissor.h"

AplicacaoTransmissor aplicacao;

void setup()
{
    aplicacao.iniciar();
}

void loop()
{
    // O servidor web atende enquanto LoRa e OLED executam em suas tarefas.
    aplicacao.atenderConfiguracao();
    delay(2);
}
