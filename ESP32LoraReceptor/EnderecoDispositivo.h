#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Persistencia e acesso sincronizado ao endereco local.
class EnderecoDispositivo
{
  public:
    bool iniciar();
    String obter();
    bool salvar(String novoEndereco);
    static bool valido(const String& endereco);

  private:
    Preferences preferencias;
    SemaphoreHandle_t mutex = nullptr;
    String endereco;
    bool armazenamentoPronto = false;
};
