#pragma once
#include <Arduino.h>
#include "../EnderecoDispositivo.h"
class TelaReceptor;

// Console local independente do Wi-Fi, com leitura nao bloqueante por linhas.
class ComandosSerial
{
public:
    ComandosSerial(Stream& porta, EnderecoDispositivo& endereco, TelaReceptor& tela);
    void iniciar();
    void atender();
private:
    void executarLinha();
    void mostrarAjuda();
    Stream& porta;
    EnderecoDispositivo& endereco;
    TelaReceptor& tela;
    static constexpr size_t TAMANHO_BUFFER = 64;
    char buffer[TAMANHO_BUFFER] = {};
    size_t tamanho = 0;
    bool descartarLinha = false;
};
