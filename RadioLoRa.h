#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include <freertos/FreeRTOS.h>

// Encapsula o SX1262 e sua configuracao na Heltec V3.
class RadioLoRa
{
public:
    enum class Modo { NaoInicializado, Inicializando, Standby, Transmitindo, Recebendo, Erro };
    struct Estado
    {
        Modo modo = Modo::NaoInicializado;
        bool inicializado = false;
        int codigo = 0;
        bool temRecepcao = false;
        float rssi = 0, snr = 0, erroFrequencia = 0;
        uint32_t atualizadoEm = 0, ultimaRecepcaoEm = 0;
    };
    Estado obterEstado() const;
    static const char* nomeModo(Modo modo);
    RadioLoRa();
    RadioLoRa(const RadioLoRa&) = delete;
    RadioLoRa& operator=(const RadioLoRa&) = delete;
    int iniciar();
    int standby();
    int transmit(String& mensagem);
    int receive(uint8_t* buffer, size_t tamanho, uint32_t timeout);
    size_t getPacketLength();
    float getRSSI();
    float getSNR();
    float getFrequencyError();
private:
    void registrar(Modo modo, int codigo);
    mutable portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
    Estado estado;
    Module modulo;
    SX1262 dispositivo;
};
