#pragma once
#include <Arduino.h>
#include <Preferences.h>

// Chamado somente pelo loop web; conexao e busca nao bloqueiam o LoRa.
class GerenciadorWiFi
{
public:
    static constexpr size_t MAX_REDES = 5;
    static constexpr size_t MAX_RESULTADOS = 20;
    struct RedeSalva { char ssid[33] = {}; char senha[65] = {}; };
    struct RedeEncontrada { String ssid; int32_t rssi = -127; bool aberta = false; };
    bool iniciar();
    void processar();
    bool salvar(const RedeSalva* novas);
    const RedeSalva& rede(size_t indice) const;
    static bool senhaValida(const String& senha);
    bool buscar();
    bool buscando() const { return buscaEmAndamento; }
    String erroDaBusca() const { return erroBusca; }
    size_t quantidadeEncontrada() const { return quantidadeResultados; }
    const RedeEncontrada& encontrada(size_t indice) const;
    String estado() const;
private:
    struct Dados { uint32_t versao = 1; RedeSalva redes[MAX_REDES]; };
    static bool dadosValidos(const Dados& valor);
    bool temRedes() const;
    void concluirBusca();
    Preferences preferencias;
    Dados dados;
    RedeEncontrada resultados[MAX_RESULTADOS];
    size_t quantidadeResultados = 0;
    size_t proximaRede = 0;
    bool armazenamentoPronto = false;
    bool conectando = false;
    bool reconectar = false;
    bool buscaEmAndamento = false;
    bool estavaConectado = false;
    String erroBusca;
    uint32_t inicioBusca = 0;
    uint32_t inicioTentativa = 0;
    uint32_t proximaTentativa = 0;
};
