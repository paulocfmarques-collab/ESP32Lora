#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>

namespace Configuracao
{
constexpr char ENDERECO_PADRAO[] = "TX01";
constexpr char SENHA_WIFI_CONFIG[] = "configlora";
constexpr int LORA_NSS = 8;
constexpr int LORA_DIO1 = 14;
constexpr int LORA_RST = 12;
constexpr int LORA_BUSY = 13;
constexpr int OLED_SDA = 17;
constexpr int OLED_SCL = 18;
constexpr int OLED_RST = 21;
constexpr int OLED_VEXT = 36;
constexpr int BOTAO_PAGINA = 0;
constexpr BaseType_t CORE_INTERFACE = 0;
constexpr BaseType_t CORE_COMUNICACAO = 1;
constexpr uint32_t DEBOUNCE_BOTAO_MS = 40;
// Número máximo de envios do mesmo pacote
const uint8_t MAX_TENTATIVAS = 3;

// Tempo máximo aguardando o ACK, em milissegundos
const uint32_t TIMEOUT_ACK = 3000;

// Intervalo entre pacotes diferentes
const unsigned long INTERVALO_PACOTES = 5000;

// Tamanho máximo da resposta ACK
const size_t TAMANHO_BUFFER_ACK = 64;

} // namespace Configuracao
