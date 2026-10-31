#include "RadioLoRa.h"
#include "ConfiguracaoComum.h"

RadioLoRa::RadioLoRa()
    : modulo(Configuracao::LORA_NSS, Configuracao::LORA_DIO1,
             Configuracao::LORA_RST, Configuracao::LORA_BUSY), dispositivo(&modulo) {}

void RadioLoRa::registrar(Modo modo, int codigo)
{
    const uint32_t agora = millis();
    portENTER_CRITICAL(&mutex);
    estado.modo = modo;
    estado.codigo = codigo;
    estado.atualizadoEm = agora;
    portEXIT_CRITICAL(&mutex);
}

RadioLoRa::Estado RadioLoRa::obterEstado() const
{
    portENTER_CRITICAL(&mutex);
    const Estado copia = estado;
    portEXIT_CRITICAL(&mutex);
    return copia;
}

const char* RadioLoRa::nomeModo(Modo modo)
{
    switch (modo)
    {
        case Modo::Inicializando: return "inicializando";
        case Modo::Standby: return "standby";
        case Modo::Transmitindo: return "transmitindo";
        case Modo::Recebendo: return "recebendo";
        case Modo::Erro: return "erro";
        default: return "nao_inicializado";
    }
}

int RadioLoRa::iniciar()
{
    registrar(Modo::Inicializando, 0);
    const int codigo = dispositivo.begin(Configuracao::FREQUENCIA_MHZ,
        Configuracao::LARGURA_BANDA_KHZ, Configuracao::SPREADING_FACTOR,
        Configuracao::CODING_RATE, Configuracao::SYNC_WORD,
        Configuracao::POTENCIA_DBM, Configuracao::PREAMBULO);
    portENTER_CRITICAL(&mutex);
    estado.inicializado = codigo == RADIOLIB_ERR_NONE;
    portEXIT_CRITICAL(&mutex);
    registrar(codigo == RADIOLIB_ERR_NONE ? Modo::Standby : Modo::Erro, codigo);
    return codigo;
}

int RadioLoRa::standby()
{
    const int codigo = dispositivo.standby();
    registrar(codigo == RADIOLIB_ERR_NONE ? Modo::Standby : Modo::Erro, codigo);
    return codigo;
}

int RadioLoRa::transmit(String& mensagem)
{
    registrar(Modo::Transmitindo, 0);
    const int codigo = dispositivo.transmit(mensagem);
    registrar(codigo == RADIOLIB_ERR_NONE ? Modo::Standby : Modo::Erro, codigo);
    return codigo;
}

int RadioLoRa::receive(uint8_t* buffer, size_t tamanho, uint32_t timeout)
{
    registrar(Modo::Recebendo, 0);
    const int codigo = dispositivo.receive(buffer, tamanho, timeout);
    if (codigo == RADIOLIB_ERR_NONE)
    {
        // Apenas a tarefa LoRa consulta o driver; UDP recebe uma copia desses valores.
        const float rssi = dispositivo.getRSSI(), snr = dispositivo.getSNR();
        const float erro = dispositivo.getFrequencyError();
        const uint32_t agora = millis();
        portENTER_CRITICAL(&mutex);
        estado.rssi = rssi; estado.snr = snr; estado.erroFrequencia = erro;
        estado.temRecepcao = true; estado.ultimaRecepcaoEm = agora;
        portEXIT_CRITICAL(&mutex);
    }
    const bool retornoNormal = codigo == RADIOLIB_ERR_NONE || codigo == RADIOLIB_ERR_RX_TIMEOUT || codigo == RADIOLIB_ERR_CRC_MISMATCH;
    registrar(retornoNormal ? Modo::Standby : Modo::Erro, codigo);
    return codigo;
}
size_t RadioLoRa::getPacketLength() { return dispositivo.getPacketLength(); }
float RadioLoRa::getRSSI() { return dispositivo.getRSSI(); }
float RadioLoRa::getSNR() { return dispositivo.getSNR(); }
float RadioLoRa::getFrequencyError() { return dispositivo.getFrequencyError(); }
