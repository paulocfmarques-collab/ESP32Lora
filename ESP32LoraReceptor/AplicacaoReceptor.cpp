#include "AplicacaoReceptor.h"
#include "Configuracao.h"

AplicacaoReceptor::AplicacaoReceptor()
    : endereco(Configuracao::ENDERECO_PADRAO),
      comandosSerial(Serial, endereco, tela), configuracaoWeb(endereco, "RX"), tela(estatisticas),
      controlePaginas(atualizarTela, this), comunicacao(executarComunicacao, this),
      diagnostico(endereco, radio, monitor, configuracaoWeb, false), transporte(radio) {}

void AplicacaoReceptor::iniciar()
{
    Serial.begin(115200); delay(1000);
    if (!endereco.iniciar()) Serial.println("Preferences indisponiveis; usando endereco padrao.");
    Serial.print("Endereco do receptor: "); Serial.println(endereco.obter());
    comandosSerial.iniciar();
    if (!configuracaoWeb.iniciar()) Serial.println("Falha ao iniciar WiFi de configuracao.");
    tela.iniciar();
    enderecoMostrado = endereco.obter();
    tela.configurarRede(enderecoMostrado, configuracaoWeb.nomeRede());
    diagnostico.iniciar();
    tela.mostrarInicializacao();
    if (!controlePaginas.iniciar()) { publicarEstado("erro_interface"); return; }
    const int codigo = radio.iniciar();
    if (codigo != RADIOLIB_ERR_NONE)
    {
        tela.mostrarErro("ERRO NO RADIO", codigo);
        publicarEstado("erro_inicializacao");
        return;
    }
    tela.mostrarAguardando();
    if (!comunicacao.iniciar()) { publicarEstado("erro_tarefa_lora"); return; }
    Serial.println("[RX] Esperando comandos enderecados. Nenhuma transmissao periodica.");
}

void AplicacaoReceptor::publicarEstado(const char* fase, const String& peer)
{
    monitor.publicar(estatisticasRPC, numeroPacote, tentativaAtual, fase, peer);
}

bool AplicacaoReceptor::aguardarConfirmacao(const ProtocoloLoRa::Quadro& pedido, uint8_t parte)
{
    const uint32_t inicio = millis();
    while (uint32_t(millis() - inicio) < Configuracao::RPC_TIMEOUT_FRAGMENTO_MS)
    {
        const uint32_t idade = millis() - inicio;
        if (idade >= Configuracao::RPC_TIMEOUT_FRAGMENTO_MS) break;
        ProtocoloLoRa::Quadro quadro;
        const int codigo = transporte.receber(quadro, Configuracao::RPC_TIMEOUT_FRAGMENTO_MS - idade);
        if (codigo == RADIOLIB_ERR_RX_TIMEOUT) break;
        if (codigo != RADIOLIB_ERR_NONE)
        {
            if (codigo == RADIOLIB_ERR_CRC_MISMATCH) ++estatisticasRPC.errosCRC;
            else ++estatisticasRPC.acksInvalidos;
            continue;
        }
        if (quadro.id != pedido.id || quadro.origem != pedido.origem || quadro.destino != pedido.destino) continue;
        if (quadro.tipo == ProtocoloLoRa::Tipo::Confirmacao && quadro.parte == parte)
        { ++estatisticasRPC.fragmentosRecebidos; return true; }
        if (quadro.tipo == ProtocoloLoRa::Tipo::Requisicao && quadro.conteudo == pedido.conteudo)
        {
            tela.registrarAtividade();
            ++estatisticasRPC.fragmentosRecebidos;
            ++estatisticas.pacotesDuplicados; ++estatisticasRPC.pacotesDuplicados;
            return false; // Reenvia o fragmento atual, sem executar novamente o comando.
        }
    }
    ++estatisticas.timeouts; ++estatisticasRPC.timeoutsACK;
    return false;
}

bool AplicacaoReceptor::enviarResposta(const ProtocoloLoRa::Quadro& pedido, const String& resposta)
{
    const uint8_t total = (resposta.length() + Configuracao::RPC_TAMANHO_FRAGMENTO - 1) /
        Configuracao::RPC_TAMANHO_FRAGMENTO;
    for (uint8_t parte = 1; parte <= total; ++parte)
    {
        const size_t inicio = (parte - 1) * Configuracao::RPC_TAMANHO_FRAGMENTO;
        const String conteudo = resposta.substring(inicio, inicio + Configuracao::RPC_TAMANHO_FRAGMENTO);
        const String pacote = ProtocoloLoRa::resposta(pedido.destino, pedido.origem, pedido.id, parte, total, conteudo);
        bool confirmado = false;
        for (tentativaAtual = 1; tentativaAtual <= Configuracao::RPC_TENTATIVAS; ++tentativaAtual)
        {
            if (tentativaAtual > 1) ++estatisticasRPC.totalRetransmissoes;
            publicarEstado("enviando_resposta");
            delay(Configuracao::RPC_TROCA_RADIO_MS);
            const int codigo = transporte.enviar(pacote);
            if (codigo != RADIOLIB_ERR_NONE) { ++estatisticasRPC.errosTransmissao; continue; }
            ++estatisticasRPC.fragmentosEnviados;
            publicarEstado("aguardando_confirmacao");
            if (aguardarConfirmacao(pedido, parte)) { confirmado = true; break; }
        }
        if (!confirmado) return false;
    }
    return true;
}

void AplicacaoReceptor::executar()
{
    publicarEstado("aguardando_comando");
    ProtocoloLoRa::Quadro pedido;
    const int codigo = transporte.receber(pedido, Configuracao::TIMEOUT_RECEPCAO);
    if (codigo == RADIOLIB_ERR_RX_TIMEOUT) return; // Silencio no canal nao e erro de consulta.
    if (codigo != RADIOLIB_ERR_NONE)
    {
        if (codigo == RADIOLIB_ERR_CRC_MISMATCH) ++estatisticasRPC.errosCRC;
        return;
    }
    if (pedido.tipo != ProtocoloLoRa::Tipo::Requisicao || pedido.destino != endereco.obter()) return;
    tela.registrarAtividade();
    numeroPacote = pedido.id; tentativaAtual = 0;
    ++estatisticasRPC.fragmentosRecebidos;
    if (temCache && pedido.id == pedidoCache.id && pedido.origem == pedidoCache.origem &&
        pedido.destino == pedidoCache.destino && pedido.conteudo != pedidoCache.conteudo) return;
    const bool duplicado = temCache && pedido.id == pedidoCache.id && pedido.origem == pedidoCache.origem &&
        pedido.destino == pedidoCache.destino && pedido.conteudo == pedidoCache.conteudo;
    if (duplicado)
    {
        ++estatisticas.pacotesDuplicados; ++estatisticasRPC.pacotesDuplicados;
    }
    else
    {
        ++estatisticas.comandosRecebidos; ++estatisticasRPC.comandosRecebidos;
        publicarEstado("novo_pacote");
        publicarEstado("executando_comando");
        pedidoCache = pedido;
        respostaCache = diagnostico.consultar(pedido.conteudo);
        if (respostaCache.isEmpty()) respostaCache = "\n";
        if (respostaCache.length() > Configuracao::RPC_TAMANHO_RESPOSTA)
            respostaCache = "ERRO: resposta excedeu 1400 bytes. Consulte os subsistemas separadamente.\n";
        temCache = true;
    }
    tela.mostrarPacote(pedido.id, pedido.conteudo, radio.getRSSI(), duplicado);
    tela.mostrarEnviandoResposta(pedido.id);
    if (enviarResposta(pedidoCache, respostaCache))
    {
        ++estatisticas.respostasEnviadas; ++estatisticasRPC.respostasEnviadas;
        ++estatisticasRPC.pacotesConfirmados;
        publicarEstado("confirmado", pedido.origem);
        tela.mostrarRespostaEnviada(pedido.id, duplicado);
    }
    else
    {
        ++estatisticas.errosResposta; ++estatisticasRPC.pacotesNaoConfirmados;
        publicarEstado("falha");
        tela.mostrarErro("RESPOSTA SEM CONFIRM", -1);
    }
    radio.standby();
}

void AplicacaoReceptor::atenderConfiguracao()
{
    comandosSerial.atender();
    diagnostico.atualizar();
    configuracaoWeb.atender();
    const String atual = endereco.obter();
    if (atual != enderecoMostrado)
    {
        enderecoMostrado = atual;
        tela.configurarRede(atual, configuracaoWeb.nomeRede());
        Serial.print("Novo endereco local: "); Serial.println(atual);
    }
}
void AplicacaoReceptor::atualizarTela(void* contexto, bool avancar) { static_cast<AplicacaoReceptor*>(contexto)->tela.atualizar(avancar); }
void AplicacaoReceptor::executarComunicacao(void* contexto) { static_cast<AplicacaoReceptor*>(contexto)->executar(); }
