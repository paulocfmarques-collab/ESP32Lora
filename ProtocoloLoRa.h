#pragma once
#include <Arduino.h>

// Protocolo RPC enderecado: requisicao, resposta fragmentada e confirmacao de fragmento.
class ProtocoloLoRa
{
public:
    enum class Tipo { Requisicao, Resposta, Confirmacao };
    struct Quadro
    {
        Tipo tipo = Tipo::Requisicao;
        String origem, destino;
        uint32_t id = 0;
        uint8_t parte = 0, total = 0;
        String conteudo;
    };
    static String requisicao(const String& origem, const String& destino, uint32_t id, const String& comando);
    static String resposta(const String& origem, const String& destino, uint32_t id,
                           uint8_t parte, uint8_t total, const String& conteudo);
    static String confirmar(const String& origem, const String& destino, uint32_t id, uint8_t parte);
    static bool interpretar(const String& pacote, Quadro& quadro);
private:
    static bool numero(const String& texto, uint32_t& valor);
};
