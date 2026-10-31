#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>

namespace Configuracao
{
constexpr char ENDERECO_PADRAO[] = "RXXX";
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
const size_t TAMANHO_BUFFER = 128;

// Tempo máximo esperando um pacote
const uint32_t TIMEOUT_RECEPCAO = 5000;

// Tempo para o transmissor mudar de TX para RX
const unsigned long ATRASO_ANTES_ACK = 250;

} // namespace Configuracao
