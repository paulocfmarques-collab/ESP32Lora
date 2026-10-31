#include "AplicacaoTransmissor.h"
#include "Configuracao.h"

namespace
{
uint32_t restante(const CanalGateway::Pedido& pedido)
{
    const uint32_t idade = millis() - pedido.criadoEm;
    return idade >= Configuracao::RPC_TIMEOUT_MS ? 0 : Configuracao::RPC_TIMEOUT_MS - idade;
}
} // namespace

AplicacaoTransmissor::AplicacaoTransmissor()
    : endereco(Configuracao::ENDERECO_PADRAO),
      configuracaoWeb(endereco, "TX"), tela(estatisticas), controlePaginas(atualizarTela, this),
      comunicacao(executarComunicacao, this),
      diagnostico(endereco, radio, monitor, configuracaoWeb, true),
      comandosUDP(endereco, diagnostico, canal, tela), transporte(radio)
{
}

void AplicacaoTransmissor::iniciar()
{
    Serial.begin(115200);
    delay(1000);
    if (!endereco.iniciar())
        Serial.println("Preferences indisponiveis; usando endereco padrao.");
    Serial.print("Endereco do gateway: ");
    Serial.println(endereco.obter());
    configuracaoWeb.definirProgressoOTA(atualizarOTA, this);
    if (!configuracaoWeb.iniciar())
        Serial.println("Falha ao iniciar WiFi de configuracao.");
    tela.iniciar();
    enderecoMostrado = endereco.obter();
    const auto rede = configuracaoWeb.informacoesRede();
    redeMostrada = rede.ssid;
    ipMostrado = rede.ip;
    redeConectada = rede.conectada;
    tela.configurarRede(enderecoMostrado, redeMostrada, ipMostrado, redeConectada);
    if (!canal.iniciar())
        Serial.println("[GW] Falha ao criar filas de consultas.");
    if (!comandosUDP.iniciar())
        Serial.println("[UDP] Falha ao criar tarefa de consultas.");
    publicarEstado("inicializando");
    tela.mostrarInicializacao();
    if (!controlePaginas.iniciar())
    {
        publicarEstado("erro_interface");
        return;
    }
    const int codigo = radio.iniciar();
    if (codigo != RADIOLIB_ERR_NONE)
    {
        tela.mostrarErro("ERRO NO RADIO", codigo);
        publicarEstado("erro_inicializacao");
        return;
    }
    tela.mostrarGateway("Aguardando comandos", "UDP 4210");
    if (!comunicacao.iniciar())
    {
        publicarEstado("erro_tarefa_lora");
        return;
    }
    Serial.println("[GW] Sem mensagens periodicas. Use status:RX01 pela porta UDP 4210.");
}

void AplicacaoTransmissor::publicarEstado(const char* fase, const String& receptor)
{
    monitor.publicar(estatisticas, numeroPacote, tentativaAtual, fase, receptor);
}

void AplicacaoTransmissor::confirmar(const CanalGateway::Pedido& pedido, const String& origem,
                                     uint8_t parte)
{
    delay(Configuracao::RPC_TROCA_RADIO_MS);
    const int codigo =
        transporte.enviar(ProtocoloLoRa::confirmar(origem, pedido.destino, pedido.id, parte));
    if (codigo == RADIOLIB_ERR_NONE)
        ++estatisticas.fragmentosEnviados;
    else
        ++estatisticas.errosTransmissao;
}

bool AplicacaoTransmissor::consultarReceptor(const CanalGateway::Pedido& pedido,
                                             const String& origem, String& resposta)
{
    uint8_t proximaParte = 1, total = 0;
    uint32_t ultimoEnvio = 0, ultimaResposta = millis();
    tentativaAtual = 0;
    ultimaParte = 0;
    resposta = "";
    while (restante(pedido))
    {
        if (!total && (!tentativaAtual ||
                       uint32_t(millis() - ultimoEnvio) >= Configuracao::RPC_ESPERA_RESPOSTA_MS))
        {
            if (tentativaAtual >= Configuracao::RPC_TENTATIVAS)
                return false;
            if (tentativaAtual)
                ++estatisticas.totalRetransmissoes;
            ++tentativaAtual;
            publicarEstado("enviando_consulta");
            const int codigo = transporte.enviar(
                ProtocoloLoRa::requisicao(origem, pedido.destino, pedido.id, pedido.comando));
            ultimoEnvio = millis();
            if (codigo != RADIOLIB_ERR_NONE)
            {
                ++estatisticas.errosTransmissao;
                continue;
            }
            ++estatisticas.fragmentosEnviados;
        }
        const uint32_t disponivel = restante(pedido);
        if (!disponivel)
            break;
        uint32_t espera =
            total ? Configuracao::RPC_TIMEOUT_FRAGMENTO_MS : Configuracao::RPC_ESPERA_RESPOSTA_MS;
        if (espera > disponivel)
            espera = disponivel;
        ProtocoloLoRa::Quadro quadro;
        publicarEstado("aguardando_resposta");
        const int codigo = transporte.receber(quadro, espera);
        if (codigo == RADIOLIB_ERR_RX_TIMEOUT)
        {
            ++estatisticas.timeoutsACK;
            if (total &&
                uint32_t(millis() - ultimaResposta) > Configuracao::RPC_TIMEOUT_FRAGMENTO_MS * 5)
                return false;
            continue;
        }
        if (codigo != RADIOLIB_ERR_NONE)
        {
            if (codigo == RADIOLIB_ERR_CRC_MISMATCH)
                ++estatisticas.errosCRC;
            else
                ++estatisticas.acksInvalidos;
            continue;
        }
        if (quadro.tipo != ProtocoloLoRa::Tipo::Resposta || quadro.id != pedido.id ||
            quadro.origem != pedido.destino || quadro.destino != origem)
            continue;
        if (quadro.parte > proximaParte)
        {
            ++estatisticas.acksInvalidos;
            continue;
        }
        if (!total)
            total = quadro.total;
        if (quadro.total != total)
        {
            ++estatisticas.acksInvalidos;
            continue;
        }
        ++estatisticas.fragmentosRecebidos;
        if (quadro.parte == proximaParte)
        {
            if (resposta.length() + quadro.conteudo.length() > Configuracao::RPC_TAMANHO_RESPOSTA)
                return false;
            resposta += quadro.conteudo;
            ++proximaParte;
        }
        else
        {
            const size_t inicio = (quadro.parte - 1) * Configuracao::RPC_TAMANHO_FRAGMENTO;
            if (resposta.substring(inicio, inicio + quadro.conteudo.length()) != quadro.conteudo)
            {
                ++estatisticas.acksInvalidos;
                continue;
            }
            ++estatisticas.pacotesDuplicados;
        }
        ultimaResposta = millis();
        ultimaParte = quadro.parte;
        confirmar(pedido, origem, quadro.parte);
        if (proximaParte > total)
            return true;
    }
    return false;
}

void AplicacaoTransmissor::concluirConfirmacoes(const CanalGateway::Pedido& pedido,
                                                const String& origem, uint8_t parte)
{
    // Se a ultima confirmacao se perder, confirma de novo o ultimo fragmento.
    const uint32_t inicio = millis();
    const uint32_t limite = Configuracao::RPC_TIMEOUT_FRAGMENTO_MS + 1500;
    while (uint32_t(millis() - inicio) < limite)
    {
        ProtocoloLoRa::Quadro quadro;
        const uint32_t idade = millis() - inicio;
        if (idade >= limite)
            break;
        const uint32_t espera = limite - idade;
        const int codigo = transporte.receber(quadro, espera);
        if (codigo == RADIOLIB_ERR_RX_TIMEOUT)
            break;
        if (codigo == RADIOLIB_ERR_NONE && quadro.tipo == ProtocoloLoRa::Tipo::Resposta &&
            quadro.id == pedido.id && quadro.origem == pedido.destino && quadro.destino == origem &&
            quadro.parte == parte)
        {
            ++estatisticas.fragmentosRecebidos;
            ++estatisticas.pacotesDuplicados;
            confirmar(pedido, origem, parte);
        }
    }
}

void AplicacaoTransmissor::executar()
{
    canal.definirPronto(true);
    publicarEstado("aguardando_cliente");
    CanalGateway::Pedido pedido;
    if (!canal.proximo(pedido))
        return;
    numeroPacote = pedido.id;
    tentativaAtual = 0;
    ++estatisticas.comandosRecebidos;
    publicarEstado("novo_pacote");
    const String origem = endereco.obter();
    tela.mostrarGateway(String("Consultando ") + pedido.destino, pedido.comando);
    String resposta;
    const bool sucesso = restante(pedido) && consultarReceptor(pedido, origem, resposta);
    if (sucesso)
    {
        ++estatisticas.pacotesConfirmados;
        ++estatisticas.respostasEnviadas;
        publicarEstado("confirmado", pedido.destino);
        tela.mostrarGateway(String("Resposta de ") + pedido.destino,
                            String(resposta.length()) + " bytes");
    }
    else
    {
        ++estatisticas.pacotesNaoConfirmados;
        publicarEstado("falha");
        resposta =
            String("ERRO: ") + pedido.destino + " nao respondeu ou a resposta ficou incompleta.\n";
        tela.mostrarGateway(String("Falha em ") + pedido.destino, "Consulta expirada");
    }
    if (!canal.responder(pedido, resposta))
        Serial.println("[GW] Fila de respostas UDP cheia.");
    if (sucesso)
    {
        publicarEstado("finalizando");
        concluirConfirmacoes(pedido, origem, ultimaParte);
    }
    radio.standby();
    publicarEstado("aguardando_cliente");
}

void AplicacaoTransmissor::atenderConfiguracao()
{
    configuracaoWeb.atender();
    const uint32_t agora = millis();
    if (uint32_t(agora - ultimaAtualizacaoRede) < 500)
        return;
    ultimaAtualizacaoRede = agora;
    const String atual = endereco.obter();
    const auto rede = configuracaoWeb.informacoesRede();
    if (atual != enderecoMostrado || rede.ssid != redeMostrada || rede.ip != ipMostrado ||
        rede.conectada != redeConectada)
    {
        enderecoMostrado = atual;
        redeMostrada = rede.ssid;
        ipMostrado = rede.ip;
        redeConectada = rede.conectada;
        tela.configurarRede(atual, redeMostrada, ipMostrado, redeConectada);
        Serial.print("[REDE] ");
        Serial.print(redeMostrada);
        Serial.print(" | Web: http://");
        Serial.println(ipMostrado);
    }
}

void AplicacaoTransmissor::atualizarTela(void* contexto, bool avancar)
{
    static_cast<AplicacaoTransmissor*>(contexto)->tela.atualizar(avancar);
}

void AplicacaoTransmissor::executarComunicacao(void* contexto)
{
    static_cast<AplicacaoTransmissor*>(contexto)->executar();
}

void AplicacaoTransmissor::atualizarOTA(void* contexto, uint8_t percentual, const String& mensagem,
                                        bool ativo)
{
    static_cast<AplicacaoTransmissor*>(contexto)->tela.mostrarProgressoOTA(percentual, mensagem,
                                                                           ativo);
}
