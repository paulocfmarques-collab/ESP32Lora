#pragma once
#include <Arduino.h>
#include <IPAddress.h>
#include <cmath>
#include <freertos/FreeRTOS.h>
#include "EnderecoDispositivo.h"
#include "RadioLoRa.h"
#include "MonitorComunicacao.h"
#include "EstadoOTA.h"
#include "LeitorBateria.h"

class ServidorConfiguracao;

class DiagnosticoDispositivo
{
public:
    DiagnosticoDispositivo(EnderecoDispositivo& endereco, RadioLoRa& radio,
                          MonitorComunicacao& monitor, ServidorConfiguracao& web, bool udpLocal);
    void iniciar();
    void atualizar();
    String consultar(const String& comando);
private:
    String resumo(bool json);
    String redeInfo();
    String radioInfo();
    String contadoresInfo();
    String bateriaInfo();
    String otaInfo();
    static String json(const String& texto);
    static String numero(float valor, uint8_t casas = 2);
    static const char* modoOTA(ModoOTA modo);
    static const char* motivoReset();
    IPAddress ipAcesso() const;
    float temperaturaAtual() const;
    EnderecoDispositivo& endereco;
    RadioLoRa& radio;
    MonitorComunicacao& monitor;
    ServidorConfiguracao& web;
    bool udpLocal;
    LeitorBateria bateria;
    mutable portMUX_TYPE mutexSensor = portMUX_INITIALIZER_UNLOCKED;
    float temperatura = NAN;
    uint32_t ultimaTemperatura = 0;
    uint32_t flashBytes = 0, flashHz = 0, sketchBytes = 0, slotOTA = 0;
};
