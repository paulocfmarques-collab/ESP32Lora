#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "../EnderecoDispositivo.h"
#include "GerenciadorWiFi.h"
#include "ServicoOTA.h"

class ServidorConfiguracao
{
public:
    struct InformacoesRede
    {
        String ssid;
        String ip;
        bool conectada = false;
    };
    ServidorConfiguracao(EnderecoDispositivo& endereco, const char* tipo);
    bool iniciar();
    void definirProgressoOTA(ServicoOTA::Progresso callback, void* contexto);
    void atender();
    String nomeRede() const;
    InformacoesRede informacoesRede() const;
    ServicoOTA::Estado estadoOTA() const { return ota.obterEstado(); }
private:
    void mostrarPagina(const String& aviso = String(), int codigo = 200);
    void salvarEndereco();
    void salvarWiFi();
    void responderStatusWiFi();
    bool tokenValido();
    static String escaparHTML(String valor);
    static String escaparJSON(const String& valor);
    EnderecoDispositivo& endereco;
    const char* tipo;
    GerenciadorWiFi wifi;
    ServicoOTA ota;
    ServicoOTA::Progresso progressoOTA = nullptr;
    void* contextoOTA = nullptr;
    WebServer servidor{80};
    String ssid;
    String token;
    bool ativo = false;
};
