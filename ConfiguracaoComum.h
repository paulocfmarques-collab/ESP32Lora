#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>

namespace Configuracao
{
constexpr uint8_t TOTAL_PAGINAS_OLED = 4;
constexpr size_t RPC_TAMANHO_RESPOSTA = 1400;
constexpr size_t RPC_TAMANHO_FRAGMENTO = 160;
constexpr size_t RPC_MAX_COMANDO = 128;
constexpr uint8_t RPC_MAX_FRAGMENTOS = 9;
constexpr uint32_t RPC_TIMEOUT_MS = 60000;
constexpr uint32_t RPC_TIMEOUT_FRAGMENTO_MS = 2500;
constexpr uint32_t RPC_ESPERA_RESPOSTA_MS = 5000;
constexpr uint32_t RPC_TROCA_RADIO_MS = 120;
constexpr uint8_t RPC_TENTATIVAS = 3;

constexpr uint16_t PORTA_UDP = 4210;
// Heltec V3: ADC1 GPIO1, divisor 390k/100k, chave de leitura ativa em LOW.
constexpr int BATERIA_ADC = 1;
constexpr int BATERIA_ENABLE = 37;
constexpr uint8_t BATERIA_ENABLE_NIVEL = LOW;
constexpr float DIVISOR_BATERIA = 4.9f;
constexpr float CALIBRACAO_BATERIA = 1.0f;
constexpr uint32_t INTERVALO_BATERIA_MS = 5000;
constexpr uint32_t ESTABILIZACAO_BATERIA_MS = 200;
// Parametros usados pelo SX1262 e exibidos na pagina de radio.
constexpr float FREQUENCIA_MHZ = 915.0f;
constexpr float LARGURA_BANDA_KHZ = 125.0f;
constexpr uint8_t SPREADING_FACTOR = 9;
constexpr uint8_t CODING_RATE = 5;
constexpr uint8_t SYNC_WORD = 0x12;
constexpr int8_t POTENCIA_DBM = 20;
constexpr uint16_t PREAMBULO = 8;

constexpr char SENHA_WIFI_CONFIG[] = "configlora";
static_assert(sizeof(SENHA_WIFI_CONFIG) == 1 ||
              (sizeof(SENHA_WIFI_CONFIG) >= 9 && sizeof(SENHA_WIFI_CONFIG) <= 64),
              "SENHA_WIFI_CONFIG: use 8 a 63 caracteres ou vazio para uma rede aberta.");
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
} // namespace Configuracao
