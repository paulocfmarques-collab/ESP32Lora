#pragma once
#include "../ConfiguracaoComum.h"

namespace Configuracao
{
constexpr char NOME_PROJETO[] = "ESP32LoraReceptor";

constexpr char HOSTNAME_OTA[] = "nao_implementado";
constexpr uint16_t PORTA_OTA = 0;

constexpr char ENDERECO_PADRAO[] = "RXXX";
const size_t TAMANHO_BUFFER = 128;

// Tempo máximo esperando um pacote
const uint32_t TIMEOUT_RECEPCAO = 5000;

// Tempo para o transmissor mudar de TX para RX
const unsigned long ATRASO_ANTES_ACK = 250;

} // namespace Configuracao
