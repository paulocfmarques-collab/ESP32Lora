#include "GerenciadorWiFi.h"
#include <WiFi.h>
#include <cstring>

bool GerenciadorWiFi::senhaValida(const String& senha)
{
    for (size_t i = 0; i < senha.length(); ++i) if (senha[i] == '\0') return false;
    if (senha.isEmpty()) return true;
    if (senha.length() >= 8 && senha.length() <= 63) return true;
    if (senha.length() != 64) return false;
    for (size_t i = 0; i < senha.length(); ++i)
    {
        const char c = senha[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            return false;
    }
    return true;
}

bool GerenciadorWiFi::dadosValidos(const Dados& valor)
{
    if (valor.versao != 1) return false;
    for (const auto& rede : valor.redes)
    {
        if (memchr(rede.ssid, '\0', sizeof(rede.ssid)) == nullptr ||
            memchr(rede.senha, '\0', sizeof(rede.senha)) == nullptr) return false;
        if (!senhaValida(String(rede.senha))) return false;
        if (rede.ssid[0] == '\0' && rede.senha[0] != '\0') return false;
    }
    return true;
}

bool GerenciadorWiFi::iniciar()
{
    armazenamentoPronto = preferencias.begin("lora-wifi", false);
    if (armazenamentoPronto && preferencias.getBytesLength("redes") == sizeof(Dados))
    {
        Dados salvos;
        if (preferencias.getBytes("redes", &salvos, sizeof(salvos)) == sizeof(salvos) && dadosValidos(salvos))
            dados = salvos;
    }
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    proximaTentativa = millis();
    return armazenamentoPronto;
}

const GerenciadorWiFi::RedeSalva& GerenciadorWiFi::rede(size_t indice) const
{
    return dados.redes[indice < MAX_REDES ? indice : 0];
}

const GerenciadorWiFi::RedeEncontrada& GerenciadorWiFi::encontrada(size_t indice) const
{
    return resultados[indice < MAX_RESULTADOS ? indice : 0];
}

bool GerenciadorWiFi::temRedes() const
{
    for (const auto& rede : dados.redes) if (rede.ssid[0]) return true;
    return false;
}

bool GerenciadorWiFi::salvar(const RedeSalva* novas)
{
    if (!armazenamentoPronto) return false;
    Dados candidato;
    for (size_t i = 0; i < MAX_REDES; ++i) candidato.redes[i] = novas[i];
    if (!dadosValidos(candidato)) return false;
    // Uma unica entrada NVS evita aplicar apenas parte das cinco redes.
    if (preferencias.putBytes("redes", &candidato, sizeof(candidato)) != sizeof(candidato)) return false;
    dados = candidato;
    reconectar = true;
    proximaRede = 0;
    // Deixa a resposta HTTP sair antes de mudar a conexao de estacao.
    proximaTentativa = millis() + 750;
    return true;
}

bool GerenciadorWiFi::buscar()
{
    if (buscaEmAndamento || reconectar) return false;
    if (conectando)
    {
        WiFi.disconnect(false, false);
        conectando = false;
        proximaTentativa = millis() + 1000;
    }
    erroBusca = "";
    WiFi.scanDelete();
    const int resultado = WiFi.scanNetworks(true, false, false, 120);
    if (resultado == WIFI_SCAN_FAILED) { erroBusca = "Não foi possível iniciar a busca."; return false; }
    inicioBusca = millis();
    buscaEmAndamento = true;
    return true;
}

void GerenciadorWiFi::concluirBusca()
{
    if (!buscaEmAndamento) return;
    const int quantidade = WiFi.scanComplete();
    if (quantidade == WIFI_SCAN_RUNNING && uint32_t(millis() - inicioBusca) < 20000) return;
    buscaEmAndamento = false;
    if (quantidade < 0)
    {
        erroBusca = "A busca não foi concluída. Tente novamente; os últimos resultados foram mantidos.";
        WiFi.scanDelete();
        return;
    }
    quantidadeResultados = 0;
    // Guarda as redes mais fortes primeiro e remove SSIDs duplicados.
    for (int i = 0; i < quantidade; ++i)
    {
        const String nome = WiFi.SSID(i);
        if (nome.isEmpty()) continue;
        bool repetida = false;
        for (size_t j = 0; j < quantidadeResultados; ++j)
            if (resultados[j].ssid == nome) repetida = true;
        if (repetida) continue;
        const int32_t sinal = WiFi.RSSI(i);
        size_t posicao = 0;
        while (posicao < quantidadeResultados && resultados[posicao].rssi >= sinal) ++posicao;
        if (posicao >= MAX_RESULTADOS) continue;
        const size_t limite = quantidadeResultados < MAX_RESULTADOS ? quantidadeResultados : MAX_RESULTADOS - 1;
        for (size_t j = limite; j > posicao; --j) resultados[j] = resultados[j - 1];
        resultados[posicao].ssid = nome;
        resultados[posicao].rssi = sinal;
        resultados[posicao].aberta = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
        if (quantidadeResultados < MAX_RESULTADOS) ++quantidadeResultados;
    }
    WiFi.scanDelete();
}

String GerenciadorWiFi::estado() const
{
    if (reconectar) return "Aplicando redes salvas";
    if (WiFi.status() == WL_CONNECTED) return "Conectado";
    if (conectando) return "Conectando";
    if (!temRedes()) return "Nenhuma rede cadastrada";
    if (buscaEmAndamento) return "Buscando redes";
    return "Aguardando nova tentativa";
}

void GerenciadorWiFi::processar()
{
    concluirBusca();
    const uint32_t agora = millis();
    if (reconectar)
    {
        if (int32_t(agora - proximaTentativa) < 0) return;
        WiFi.disconnect(false, false); // Preserva o ponto de acesso de configuracao.
        reconectar = false;
        conectando = false;
        estavaConectado = false;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!estavaConectado)
        {
            Serial.print("WiFi conectado: "); Serial.println(WiFi.SSID());
            Serial.print("IP na rede: "); Serial.println(WiFi.localIP());
        }
        estavaConectado = true;
        conectando = false;
        return;
    }
    if (estavaConectado)
    {
        estavaConectado = false;
        proximaTentativa = agora + 1000;
    }
    if (conectando)
    {
        if (uint32_t(agora - inicioTentativa) < 13000) return;
        WiFi.disconnect(false, false);
        conectando = false;
        proximaTentativa = agora + 750;
    }
    if (buscaEmAndamento || int32_t(agora - proximaTentativa) < 0 || !temRedes()) return;
    for (size_t i = 0; i < MAX_REDES; ++i)
    {
        const size_t indice = (proximaRede + i) % MAX_REDES;
        const auto& alvo = dados.redes[indice];
        if (!alvo.ssid[0]) continue;
        proximaRede = (indice + 1) % MAX_REDES;
        inicioTentativa = agora;
        conectando = true;
        WiFi.begin(alvo.ssid, alvo.senha);
        Serial.print("WiFi: tentando rede "); Serial.println(alvo.ssid);
        return;
    }
}
