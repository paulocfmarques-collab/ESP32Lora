#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "EstatisticasTransmissor.h"
#include "../ProtecaoTela.h"

class TelaTransmissor
{
public:
    explicit TelaTransmissor(const EstatisticasTransmissor& contadores);
    void iniciar();
    void configurarRede(const String& endereco, const String& rede, const String& ip, bool conectada);
    void atualizar(bool avancar);
    void registrarAtividade();
    void mostrarProgressoOTA(uint8_t percentual, const String& mensagem, bool ativo);
    void sincronizarEstatisticas();
    void mostrarInicializacao();
    void mostrarPronto();
    void mostrarGateway(const String& fase, const String& detalhe);
    void mostrarEnviando(unsigned long id, uint8_t tentativa);
    void mostrarAguardandoACK(unsigned long id, uint8_t tentativa);
    void mostrarACKRecebido(unsigned long id, float rssi, float snr, const String& receptor);
    void mostrarACKInvalido(unsigned long esperado, unsigned long recebido);
    void mostrarRetransmissao(unsigned long id, uint8_t proximaTentativa);
    void mostrarFalha(unsigned long id);
    void mostrarErro(const char* titulo, int codigo);

private:
    struct EstadoTela
    {
        bool otaAtiva = false;
        uint8_t progressoOTA = 0;
        String mensagemOTA;
        String endereco;
        String rede;
        String ip;
        bool conectada = false;
        String titulo;
        String linhas[3];
    };
    void publicarStatus(const char* titulo, const String& linha1 = String(),
                        const String& linha2 = String(), const String& linha3 = String());
    void desenharPagina(uint8_t selecionada, const EstadoTela& estado,
                        const EstatisticasTransmissor& contadores);
    SemaphoreHandle_t mutex = nullptr;
    uint8_t pagina = 0;
    ProtecaoTela protecao;
    bool redesenhar = true;
    bool primeiroDesenho = true;
    uint32_t ultimaAtualizacao = 0;
    EstadoTela status;
    EstatisticasTransmissor estatisticasPagina;
    const EstatisticasTransmissor& estatisticas;
    U8G2_SSD1306_128X64_NONAME_F_SW_I2C display;
};
