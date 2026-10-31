#include "EnderecoDispositivo.h"
#include "Configuracao.h"

bool EnderecoDispositivo::valido(const String& valor)
{
    if (valor.length() < 1 || valor.length() > 16 || valor == "TODOS") return false;
    for (size_t i = 0; i < valor.length(); ++i)
    {
        const char c = valor[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_'))
            return false;
    }
    return true;
}

bool EnderecoDispositivo::iniciar()
{
    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) return false;
    endereco = Configuracao::ENDERECO_PADRAO;
    armazenamentoPronto = preferencias.begin("lora-config", false);
    if (!armazenamentoPronto) return false;
    const String salvo = preferencias.getString("endereco", "");
    if (valido(salvo))
    {
        endereco = salvo;
        return true;
    }
    // Persiste tambem o padrao na primeira inicializacao.
    return preferencias.putString("endereco", endereco) == endereco.length();
}

String EnderecoDispositivo::obter()
{
    if (mutex == nullptr) return Configuracao::ENDERECO_PADRAO;
    xSemaphoreTake(mutex, portMAX_DELAY);
    const String copia = endereco;
    xSemaphoreGive(mutex);
    return copia;
}

bool EnderecoDispositivo::salvar(String novoEndereco)
{
    novoEndereco.trim();
    novoEndereco.toUpperCase();
    if (!valido(novoEndereco) || mutex == nullptr || !armazenamentoPronto) return false;
    xSemaphoreTake(mutex, portMAX_DELAY);
    // Atualiza a RAM somente depois de confirmar a gravacao na NVS.
    const bool salvo = preferencias.putString("endereco", novoEndereco) == novoEndereco.length();
    if (salvo) endereco = novoEndereco;
    xSemaphoreGive(mutex);
    return salvo;
}
