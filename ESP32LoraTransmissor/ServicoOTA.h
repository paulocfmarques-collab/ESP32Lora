#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <esp_app_format.h>
#include <esp_app_desc.h>
#include "../EstadoOTA.h"
#include <freertos/FreeRTOS.h>

// OTA HTTP e Arduino IDE, sem senha, com progresso compartilhado no OLED.
class ServicoOTA
{
public:
    using Modo = ModoOTA;
    using Estado = EstadoOTA;
    Estado obterEstado() const;
    using Progresso = void (*)(void*, uint8_t, const String&, bool);
    void iniciar(WebServer& servidor, const String& token, Progresso callback, void* contexto);
    void atender();
    bool ocupado() const { return recebendo || recebendoIDE || fimRecebido || reinicioPendente; }
    size_t capacidade() const;
private:
    void publicarEstado();
    mutable portMUX_TYPE mutexEstado = portMUX_INITIALIZER_UNLOCKED;
    Estado estadoPublicado;
    size_t capacidadeInicial = 0;
    void configurarArduinoOTA();
    void atenderArduinoOTA();
    void mostrarPagina(WebServer& servidor);
    void receber(WebServer& servidor);
    void finalizar(WebServer& servidor);
    void falhar(const String& mensagem, int codigo = 400);
    void notificar(uint8_t percentual, const String& mensagem, bool ativo = true);
    bool lerTamanho(const String& texto);
    String token;
    String falha;
    Progresso callback = nullptr;
    void* contexto = nullptr;
    bool requisicaoIniciada = false;
    bool recebendo = false;
    bool recebendoIDE = false;
    bool arduinoOTAIniciado = false;
    uint32_t ipArduinoOTA = 0;
    bool fimRecebido = false;
    bool reinicioPendente = false;
    bool liberarTela = false;
    size_t tamanhoEsperado = 0;
    size_t recebidos = 0;
    size_t tamanhoCabecalho = 0;
    static constexpr size_t TAMANHO_CABECALHO = sizeof(esp_image_header_t) +
        sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t);
    uint8_t cabecalho[TAMANHO_CABECALHO] = {};
    uint8_t percentual = 0;
    int codigoFalha = 400;
    uint32_t ultimaAtividade = 0;
    uint32_t reinicioEm = 0;
    uint32_t falhaEm = 0;
};
