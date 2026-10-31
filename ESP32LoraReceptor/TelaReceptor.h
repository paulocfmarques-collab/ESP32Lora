#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "EstatisticasReceptor.h"

class TelaReceptor
{
  public:
    explicit TelaReceptor(const EstatisticasReceptor& contadores);
    void iniciar();
    void configurarRede(const String& endereco, const String& rede);
    void atualizar(bool avancar);
    void sincronizarEstatisticas();
    void mostrarInicializacao();
    void mostrarAguardando();
    void mostrarPacote(unsigned long id, const String& mensagem, float rssi, bool duplicado);
    void mostrarEnviandoACK(unsigned long id);
    void mostrarACKEnviado(unsigned long id, bool duplicado);
    void mostrarErro(const char* titulo, int codigo);

  private:
    struct EstadoTela
    {
        String endereco;
        String rede;
        String titulo;
        String linhas[3];
    };

    void publicarStatus(const char* titulo, const String& linha1 = String(),
                        const String& linha2 = String(), const String& linha3 = String());
    void desenharPagina(uint8_t selecionada, const EstadoTela& estado,
                        const EstatisticasReceptor& contadores);
    SemaphoreHandle_t mutex = nullptr;
    uint8_t pagina = 0;
    bool redesenhar = true;
    bool primeiroDesenho = true;
    uint32_t ultimaAtualizacao = 0;
    EstadoTela status;
    EstatisticasReceptor estatisticasPagina;
    const EstatisticasReceptor& estatisticas;
    U8G2_SSD1306_128X64_NONAME_F_SW_I2C display;
};
