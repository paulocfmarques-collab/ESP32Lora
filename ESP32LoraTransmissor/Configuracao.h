#pragma once
#include "../ConfiguracaoComum.h"

namespace Configuracao
{
constexpr char NOME_PROJETO[] = "ESP32LoraTransmissor";

constexpr char HOSTNAME_OTA[] = "LoRa-TX01";
constexpr uint16_t PORTA_OTA = 3232;

constexpr char ENDERECO_PADRAO[] = "TX01";
// Número máximo de envios do mesmo pacote
const uint8_t MAX_TENTATIVAS = 3;

// Tempo máximo aguardando o ACK, em milissegundos
const uint32_t TIMEOUT_ACK = 3000;

// Intervalo entre pacotes diferentes
const unsigned long INTERVALO_PACOTES = 5000;

// Tamanho máximo da resposta ACK
const size_t TAMANHO_BUFFER_ACK = 64;

} // namespace Configuracao
