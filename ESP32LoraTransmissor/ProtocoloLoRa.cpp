#include "ProtocoloLoRa.h"
#include "EnderecoDispositivo.h"
#include <limits.h>

bool ProtocoloLoRa::interpretarID(const String& texto, unsigned long& id)
{
    if (texto.isEmpty())
        return false;
    unsigned long valor = 0;
    for (size_t i = 0; i < texto.length(); ++i)
    {
        const char c = texto[i];
        if (c < '0' || c > '9')
            return false;
        const unsigned long digito = c - '0';
        if (valor > (ULONG_MAX - digito) / 10)
            return false;
        valor = valor * 10 + digito;
    }
    id = valor;
    return true;
}

String ProtocoloLoRa::criarDados(const String& origem, const String& destino, unsigned long id,
                                 const String& mensagem)
{
    return String("DATA|") + origem + "|" + destino + "|" + id + "|" + mensagem;
}

String ProtocoloLoRa::criarACK(const String& origem, const String& destino, unsigned long id)
{
    return String("ACK|") + origem + "|" + destino + "|" + id;
}

bool ProtocoloLoRa::interpretarPacote(const String& pacote, String& origem, String& destino,
                                      unsigned long& id, String& mensagem)
{
    if (!pacote.startsWith("DATA|"))
        return false;
    const int a = pacote.indexOf('|', 5);
    const int b = a < 0 ? -1 : pacote.indexOf('|', a + 1);
    const int c = b < 0 ? -1 : pacote.indexOf('|', b + 1);
    if (a < 0 || b < 0 || c < 0)
        return false;
    const String remetente = pacote.substring(5, a);
    const String destinatario = pacote.substring(a + 1, b);
    if (!EnderecoDispositivo::valido(remetente) ||
        (destinatario != "TODOS" && !EnderecoDispositivo::valido(destinatario)))
        return false;
    unsigned long numero;
    if (!interpretarID(pacote.substring(b + 1, c), numero))
        return false;
    origem = remetente;
    destino = destinatario;
    id = numero;
    mensagem = pacote.substring(c + 1);
    return true;
}

bool ProtocoloLoRa::interpretarACK(const String& pacote, String& origem, String& destino,
                                   unsigned long& id)
{
    if (!pacote.startsWith("ACK|"))
        return false;
    const int a = pacote.indexOf('|', 4);
    const int b = a < 0 ? -1 : pacote.indexOf('|', a + 1);
    if (a < 0 || b < 0)
        return false;
    const String remetente = pacote.substring(4, a);
    const String destinatario = pacote.substring(a + 1, b);
    if (!EnderecoDispositivo::valido(remetente) || !EnderecoDispositivo::valido(destinatario))
        return false;
    unsigned long numero;
    if (!interpretarID(pacote.substring(b + 1), numero))
        return false;
    origem = remetente;
    destino = destinatario;
    id = numero;
    return true;
}
