#include "ComandosSerial.h"
#include "TelaReceptor.h"

ComandosSerial::ComandosSerial(Stream& porta, EnderecoDispositivo& endereco, TelaReceptor& tela)
    : porta(porta), endereco(endereco), tela(tela)
{
}

void ComandosSerial::iniciar()
{
    porta.println("[CONFIG] Serial: endereco? | endereco RX01 | ajuda. Finalize com Enter.");
}

void ComandosSerial::mostrarAjuda()
{
    porta.println("[CONFIG] endereco?       Consulta o endereco atual.");
    porta.println("[CONFIG] endereco RX01   Salva e aplica o endereco sem reiniciar.");
    porta.println("[CONFIG] ajuda           Mostra estes comandos.");
    porta.println(
        "[CONFIG] Use enderecos diferentes: RX01 no primeiro receptor e RX02 no segundo.");
    porta.println("[CONFIG] 1 a 16 letras, numeros, hifen ou sublinhado; TODOS e reservado.");
}

void ComandosSerial::atender()
{
    // Limita o trabalho de cada ciclo para continuar atendendo a pagina web.
    size_t lidos = 0;
    while (porta.available() > 0 && lidos < TAMANHO_BUFFER)
    {
        const int recebido = porta.read();
        if (recebido < 0)
            break;
        ++lidos;
        const char c = static_cast<char>(recebido);
        if (c == '\r' || c == '\n')
        {
            if (descartarLinha)
                porta.println(
                    "[CONFIG] ERRO: linha invalida ou muito longa. Envie novamente com Enter.");
            else if (tamanho > 0)
            {
                tela.registrarAtividade();
                buffer[tamanho] = '\0';
                executarLinha();
            }
            tamanho = 0;
            descartarLinha = false;
            continue;
        }
        if (descartarLinha)
            continue;
        if (c == '\b' || c == 127)
        {
            if (tamanho > 0)
                --tamanho;
            continue;
        }
        if ((static_cast<uint8_t>(c) < 32 && c != '\t') || tamanho >= TAMANHO_BUFFER - 1)
        {
            // Nunca executa um prefixo truncado de um comando recebido.
            descartarLinha = true;
            continue;
        }
        buffer[tamanho++] = c;
    }
}

void ComandosSerial::executarLinha()
{
    String linha(buffer);
    linha.trim();
    linha.toLowerCase();
    if (linha.isEmpty())
        return;
    if (linha == "ajuda" || linha == "help" || linha == "?")
    {
        mostrarAjuda();
        return;
    }
    if (linha == "endereco" || linha == "endereco?")
    {
        porta.println(String("[CONFIG] Endereco atual: ") + endereco.obter());
        return;
    }
    int separador = 0;
    while (separador < static_cast<int>(linha.length()) && linha[separador] != ' ' &&
           linha[separador] != '\t')
        ++separador;
    if (linha.substring(0, separador) != "endereco")
    {
        porta.println("[CONFIG] ERRO: comando desconhecido. Digite ajuda.");
        return;
    }
    String novo = linha.substring(separador);
    novo.trim();
    novo.toUpperCase();
    if (!EnderecoDispositivo::valido(novo))
    {
        porta.println(
            "[CONFIG] ERRO: endereco invalido. Use 1 a 16 caracteres; TODOS e reservado.");
        return;
    }
    if (!endereco.salvar(novo))
    {
        porta.println("[CONFIG] ERRO: falha ao gravar nas Preferences. Endereco anterior mantido.");
        return;
    }
    porta.println(String("[CONFIG] OK: endereco ") + endereco.obter() +
                  " salvo nas Preferences e aplicado sem reiniciar.");
}
