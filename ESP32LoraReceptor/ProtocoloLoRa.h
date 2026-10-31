#pragma once
#include <Arduino.h>

// DATA|origem|destino|id|mensagem e ACK|origem|destino|id.
class ProtocoloLoRa
{
public:
    static String criarDados(const String& origem, const String& destino,
                             unsigned long id, const String& mensagem);
    static String criarACK(const String& origem, const String& destino, unsigned long id);
    static bool interpretarPacote(const String& pacote, String& origem, String& destino,
                                  unsigned long& id, String& mensagem);
    static bool interpretarACK(const String& pacote, String& origem, String& destino,
                               unsigned long& id);
private:
    static bool interpretarID(const String& texto, unsigned long& id);
};
