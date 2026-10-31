#include "LeitorBateria.h"
#include "ConfiguracaoComum.h"

void LeitorBateria::iniciar()
{
    pinMode(Configuracao::BATERIA_ENABLE, OUTPUT);
    digitalWrite(Configuracao::BATERIA_ENABLE, !Configuracao::BATERIA_ENABLE_NIVEL);
    pinMode(Configuracao::BATERIA_ADC, INPUT);
    analogSetPinAttenuation(Configuracao::BATERIA_ADC, ADC_2_5db);
    inicio = millis() - Configuracao::INTERVALO_BATERIA_MS;
}

void LeitorBateria::atualizar()
{
    const uint32_t agora = millis();
    if (!medindo)
    {
        if (uint32_t(agora - inicio) < Configuracao::INTERVALO_BATERIA_MS)
            return;
        inicio = agora;
        soma = 0;
        amostras = 0;
        medindo = true;
        digitalWrite(Configuracao::BATERIA_ENABLE, Configuracao::BATERIA_ENABLE_NIVEL);
        return;
    }
    if (uint32_t(agora - inicio) < Configuracao::ESTABILIZACAO_BATERIA_MS ||
        uint32_t(agora - ultimaAmostra) < 5)
        return;
    ultimaAmostra = agora;
    soma += analogReadMilliVolts(Configuracao::BATERIA_ADC);
    if (++amostras < 8)
        return;
    digitalWrite(Configuracao::BATERIA_ENABLE, !Configuracao::BATERIA_ENABLE_NIVEL);
    Amostra nova;
    nova.adcMV = uint32_t(soma / amostras);
    nova.volts = (float(soma) / amostras / 1000.0f) * Configuracao::DIVISOR_BATERIA *
                 Configuracao::CALIBRACAO_BATERIA;
    nova.valida = nova.adcMV > 0 && std::isfinite(nova.volts) && nova.volts <= 5.5f;
    nova.lidaEm = agora;
    portENTER_CRITICAL(&mutex);
    dados = nova;
    portEXIT_CRITICAL(&mutex);
    medindo = false;
}

LeitorBateria::Amostra LeitorBateria::obter() const
{
    portENTER_CRITICAL(&mutex);
    const Amostra copia = dados;
    portEXIT_CRITICAL(&mutex);
    return copia;
}
