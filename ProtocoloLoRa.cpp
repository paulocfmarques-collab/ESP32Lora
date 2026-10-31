#include "ProtocoloLoRa.h"
#include "ConfiguracaoComum.h"
#include "EnderecoDispositivo.h"
#include <stdint.h>
static_assert(Configuracao::RPC_TAMANHO_FRAGMENTO + 64 <= 255, "Fragmento excede o quadro LoRa.");
static_assert((Configuracao::RPC_TAMANHO_RESPOSTA + Configuracao::RPC_TAMANHO_FRAGMENTO - 1) /
                      Configuracao::RPC_TAMANHO_FRAGMENTO <=
                  Configuracao::RPC_MAX_FRAGMENTOS,
              "Fragmentos insuficientes.");

bool ProtocoloLoRa::numero(const String& texto, uint32_t& valor)
{
    if (texto.isEmpty())
        return false;
    uint32_t n = 0;
    for (size_t i = 0; i < texto.length(); ++i)
    {
        if (texto[i] < '0' || texto[i] > '9')
            return false;
        const uint32_t d = texto[i] - '0';
        if (n > (UINT32_MAX - d) / 10)
            return false;
        n = n * 10 + d;
    }
    valor = n;
    return true;
}

String ProtocoloLoRa::requisicao(const String& origem, const String& destino, uint32_t id,
                                 const String& comando)
{
    return String("REQ|") + origem + "|" + destino + "|" + id + "|" + comando;
}

String ProtocoloLoRa::resposta(const String& origem, const String& destino, uint32_t id,
                               uint8_t parte, uint8_t total, const String& conteudo)
{
    return String("RES|") + origem + "|" + destino + "|" + id + "|" + parte + "|" + total + "|" +
           conteudo;
}

String ProtocoloLoRa::confirmar(const String& origem, const String& destino, uint32_t id,
                                uint8_t parte)
{
    return String("RAK|") + origem + "|" + destino + "|" + id + "|" + parte;
}

bool ProtocoloLoRa::interpretar(const String& pacote, Quadro& quadro)
{
    if (pacote.length() > 255)
        return false;
    const bool resposta = pacote.startsWith("RES|");
    if (!resposta && !pacote.startsWith("REQ|") && !pacote.startsWith("RAK|"))
        return false;
    String campos[7];
    const uint8_t separadores = resposta ? 6 : 4;
    int inicio = 0;
    for (uint8_t i = 0; i < separadores; ++i)
    {
        const int fim = pacote.indexOf('|', inicio);
        if (fim < 0)
            return false;
        campos[i] = pacote.substring(inicio, fim);
        inicio = fim + 1;
    }
    campos[separadores] = pacote.substring(inicio);
    if (!EnderecoDispositivo::valido(campos[1]) || !EnderecoDispositivo::valido(campos[2]))
        return false;
    uint32_t id;
    if (!numero(campos[3], id) || !id)
        return false;
    Quadro novo;
    novo.origem = campos[1];
    novo.destino = campos[2];
    novo.id = id;
    if (resposta)
    {
        uint32_t parte, total;
        if (!numero(campos[4], parte) || !numero(campos[5], total) || !parte || !total ||
            parte > total || total > Configuracao::RPC_MAX_FRAGMENTOS || campos[6].isEmpty() ||
            campos[6].length() > Configuracao::RPC_TAMANHO_FRAGMENTO ||
            (parte < total && campos[6].length() != Configuracao::RPC_TAMANHO_FRAGMENTO))
            return false;
        novo.tipo = Tipo::Resposta;
        novo.parte = parte;
        novo.total = total;
        novo.conteudo = campos[6];
    }
    else if (campos[0] == "RAK")
    {
        uint32_t parte;
        if (!numero(campos[4], parte) || !parte || parte > Configuracao::RPC_MAX_FRAGMENTOS)
            return false;
        novo.tipo = Tipo::Confirmacao;
        novo.parte = parte;
    }
    else
    {
        novo.conteudo = campos[4];
        novo.conteudo.trim();
        novo.conteudo.toLowerCase();
        if (novo.conteudo.isEmpty() || novo.conteudo.length() > Configuracao::RPC_MAX_COMANDO)
            return false;
        for (size_t i = 0; i < novo.conteudo.length(); ++i)
            if (uint8_t(novo.conteudo[i]) < 32 || uint8_t(novo.conteudo[i]) > 126)
                return false;
        novo.tipo = Tipo::Requisicao;
    }
    quadro = novo;
    return true;
}
