#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "../EnderecoDispositivo.h"
#include "../EstadoOTA.h"

class ServidorConfiguracao
{
  public:
    ServidorConfiguracao(EnderecoDispositivo& endereco, const char* tipo);
    bool iniciar();
    void atender();
    String nomeRede() const;
    EstadoOTA estadoOTA() const { return EstadoOTA{}; }

  private:
    void mostrarPagina(const String& aviso = String(), int codigo = 200);
    void salvarEndereco();
    EnderecoDispositivo& endereco;
    const char* tipo;
    WebServer servidor{80};
    String ssid;
    String token;
    bool ativo = false;
};
