#include "ServicoOTA.h"
#include "PaginaOTA.h"
#include "Configuracao.h"
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <cstring>

size_t ServicoOTA::capacidade() const
{
    const auto* atual = esp_ota_get_running_partition();
    const auto* proxima = esp_ota_get_next_update_partition(nullptr);
    const auto* dadosOTA = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, nullptr);
    return atual && proxima && dadosOTA && atual->address != proxima->address ? proxima->size : 0;
}

void ServicoOTA::iniciar(WebServer& servidor, const String& nonce, Progresso progresso, void* usuario)
{
    token = nonce;
    callback = progresso;
    contexto = usuario;
    const char* cabecalhos[] = {"X-OTA-Token", "X-Firmware-Size"};
    servidor.collectHeaders(cabecalhos, 2);
    servidor.on("/ota", HTTP_GET, [this, &servidor]() { mostrarPagina(servidor); });
    servidor.on("/ota", HTTP_POST, [this, &servidor]() { finalizar(servidor); },
                [this, &servidor]() { receber(servidor); });
    capacidadeInicial = capacidade();
    configurarArduinoOTA();
    publicarEstado();
    Serial.print("[OTA] Sem senha em /ota. Capacidade: "); Serial.println(capacidade());
}

void ServicoOTA::mostrarPagina(WebServer& servidor)
{
    String pagina(PAGINA_OTA);
    pagina.replace("{{TOKEN}}", token);
    pagina.replace("{{HOSTNAME}}", Configuracao::HOSTNAME_OTA);
    pagina.replace("{{MINIMO}}", String((unsigned long)sizeof(cabecalho)));
    pagina.replace("{{CAPACIDADE}}", String((unsigned long)capacidade()));
    pagina.replace("{{DISABLED}}", capacidade() ? "" : "disabled");
    pagina.replace("{{ESTADO}}", capacidade() ?
        "Selecione o firmware de aplicacao .ino.bin do transmissor." :
        "Sem particao OTA disponivel. Grave por USB com uma tabela de duas particoes de aplicacao.");
    servidor.sendHeader("Cache-Control", "no-store");
    servidor.send(200, "text/html; charset=utf-8", pagina);
}

void ServicoOTA::notificar(uint8_t valor, const String& mensagem, bool ativo)
{
    percentual = valor > 100 ? 100 : valor;
    publicarEstado();
    if (callback) callback(contexto, percentual, mensagem, ativo);
}

void ServicoOTA::falhar(const String& mensagem, int codigo)
{
    falha = mensagem;
    codigoFalha = codigo;
    if (Update.isRunning()) Update.abort();
    recebendo = false;
    recebendoIDE = false;
    fimRecebido = false;
    liberarTela = true;
    falhaEm = millis();
    Serial.print("[OTA] ERRO: "); Serial.println(falha);
    notificar(percentual, "Falha na atualizacao");
}

bool ServicoOTA::lerTamanho(const String& texto)
{
    const size_t limite = capacidade();
    if (texto.isEmpty() || !limite) return false;
    uint64_t valor = 0;
    for (size_t i = 0; i < texto.length(); ++i)
    {
        if (texto[i] < '0' || texto[i] > '9') return false;
        valor = valor * 10 + (texto[i] - '0');
        if (valor > limite) return false;
    }
    tamanhoEsperado = size_t(valor);
    return tamanhoEsperado >= sizeof(cabecalho);
}

void ServicoOTA::receber(WebServer& servidor)
{
    HTTPUpload& arquivo = servidor.upload();
    if (arquivo.status == UPLOAD_FILE_START)
    {
        if (reinicioPendente) return;
        if (requisicaoIniciada) { falhar("Envie apenas um arquivo por atualizacao."); return; }
        requisicaoIniciada = true;
        falha = "";
        recebendo = fimRecebido = liberarTela = false;
        recebidos = tamanhoCabecalho = tamanhoEsperado = 0;
        percentual = 0;
        codigoFalha = 400;
        if (servidor.header("X-OTA-Token") != token)
        { falhar("Pagina expirada. Abra /ota novamente.", 403); return; }
        String nome = arquivo.filename; nome.toLowerCase();
        if (arquivo.name != "firmware" || !nome.endsWith(".bin") ||
            nome.indexOf("bootloader") >= 0 || nome.indexOf("partition") >= 0 ||
            nome.indexOf("merged") >= 0 || nome.indexOf("receptor") >= 0)
        { falhar("Use somente o .ino.bin de aplicacao do transmissor."); return; }
        if (!capacidade())
        { falhar("Sem particao OTA. Instale uma tabela compativel por USB primeiro.", 409); return; }
        if (!lerTamanho(servidor.header("X-Firmware-Size")))
        { falhar("Tamanho ausente, invalido ou maior que a particao OTA.", 413); return; }
        if (!Update.begin(tamanhoEsperado, U_FLASH))
        { falhar(String(Update.errorString()), 500); return; }
        recebendo = true;
        ultimaAtividade = millis();
        notificar(0, "Recebendo firmware");
        Serial.println("[OTA] Inicio da gravacao.");
    }
    else if (arquivo.status == UPLOAD_FILE_WRITE && recebendo)
    {
        ultimaAtividade = millis();
        if (recebidos > tamanhoEsperado || arquivo.currentSize > tamanhoEsperado - recebidos)
        { falhar("Arquivo excedeu o tamanho informado.", 413); return; }
        size_t posicao = 0;
        if (tamanhoCabecalho < sizeof(cabecalho))
        {
            size_t copiar = sizeof(cabecalho) - tamanhoCabecalho;
            if (copiar > arquivo.currentSize) copiar = arquivo.currentSize;
            memcpy(cabecalho + tamanhoCabecalho, arquivo.buf, copiar);
            tamanhoCabecalho += copiar;
            posicao = copiar;
            if (tamanhoCabecalho == sizeof(cabecalho))
            {
                esp_image_header_t imagem;
                memcpy(&imagem, cabecalho, sizeof(imagem));
                esp_image_segment_header_t segmento;
                esp_app_desc_t descricao;
                memcpy(&segmento, cabecalho + sizeof(imagem), sizeof(segmento));
                memcpy(&descricao, cabecalho + sizeof(imagem) + sizeof(segmento), sizeof(descricao));
                if (imagem.magic != ESP_IMAGE_HEADER_MAGIC || imagem.chip_id != ESP_CHIP_ID_ESP32S3 ||
                    segmento.data_len < sizeof(descricao) || descricao.magic_word != ESP_APP_DESC_MAGIC_WORD)
                { falhar("Use uma imagem de aplicacao valida para ESP32-S3."); return; }
                if (Update.write(cabecalho, sizeof(cabecalho)) != sizeof(cabecalho))
                { falhar(String(Update.errorString()), 500); return; }
            }
        }
        if (posicao < arquivo.currentSize &&
            Update.write(arquivo.buf + posicao, arquivo.currentSize - posicao) != arquivo.currentSize - posicao)
        { falhar(String(Update.errorString()), 500); return; }
        recebidos += arquivo.currentSize;
        const uint8_t valor = uint8_t((uint64_t(recebidos) * 99) / tamanhoEsperado);
        if (valor != percentual) notificar(valor, "Gravando firmware");
    }
    else if (arquivo.status == UPLOAD_FILE_END && recebendo)
    {
        if (recebidos != tamanhoEsperado || tamanhoCabecalho != sizeof(cabecalho))
        { falhar("Upload incompleto; firmware atual mantido."); return; }
        fimRecebido = true;
        notificar(99, "Validando firmware");
    }
    else if (arquivo.status == UPLOAD_FILE_ABORTED)
    {
        if (!reinicioPendente) falhar("Envio interrompido; firmware atual mantido.");
        requisicaoIniciada = false;
    }
}

void ServicoOTA::finalizar(WebServer& servidor)
{
    servidor.sendHeader("Cache-Control", "no-store");
    servidor.sendHeader("Connection", "close");
    if (reinicioPendente || recebendoIDE)
    { servidor.send(409, "text/plain; charset=utf-8", "Atualizacao em andamento ou reinicio agendado."); return; }
    if (!requisicaoIniciada || !recebendo || !fimRecebido)
    {
        if (recebendo) falhar("Upload incompleto; firmware atual mantido.");
        servidor.send(codigoFalha, "text/plain; charset=utf-8", falha.isEmpty() ? "Nenhum firmware valido recebido." : falha);
        requisicaoIniciada = false;
        return;
    }
    // end() exige o tamanho exato e valida a imagem antes de mudar a particao de boot.
    if (!Update.end())
    {
        falhar(String(Update.errorString()), 400);
        servidor.send(codigoFalha, "text/plain; charset=utf-8", falha);
        requisicaoIniciada = false;
        return;
    }
    recebendo = fimRecebido = requisicaoIniciada = false;
    liberarTela = false;
    reinicioPendente = true;
    reinicioEm = millis();
    notificar(100, "Concluido! Reiniciando");
    Serial.println("[OTA] Firmware validado. Reiniciando em 2 segundos.");
    servidor.send(200, "text/plain; charset=utf-8", "Firmware gravado e validado. O transmissor vai reiniciar.");
}

void ServicoOTA::atender()
{
    const uint32_t agora = millis();
    if (reinicioPendente)
    {
        if (uint32_t(agora - reinicioEm) >= 2000) ESP.restart();
        return;
    }
    if (recebendo && uint32_t(agora - ultimaAtividade) >= 30000)
    {
        falhar("Tempo de upload esgotado; firmware atual mantido.");
        requisicaoIniciada = false;
    }
    if (liberarTela && uint32_t(agora - falhaEm) >= 5000)
    {
        liberarTela = false;
        notificar(percentual, "", false);
    }
    atenderArduinoOTA();
    publicarEstado();
}

void ServicoOTA::configurarArduinoOTA()
{
    ArduinoOTA.setHostname(Configuracao::HOSTNAME_OTA);
    ArduinoOTA.setPort(Configuracao::PORTA_OTA);
    ArduinoOTA.setMdnsEnabled(true);
    // Nao chamar setPassword: a autenticacao permanece desativada.
    ArduinoOTA.setRebootOnSuccess(false);
    ArduinoOTA.onStart([this]()
    {
        recebendoIDE = true;
        liberarTela = false;
        falha = "";
        percentual = 0;
        if (ArduinoOTA.getCommand() != U_FLASH)
        {
            falhar("ArduinoOTA aceita somente firmware de aplicacao.");
            return;
        }
        notificar(0, "Recebendo pela IDE");
        Serial.println("[ArduinoOTA] Inicio da gravacao pela IDE.");
    });
    ArduinoOTA.onProgress([this](unsigned int recebido, unsigned int total)
    {
        if (!recebendoIDE) return;
        const uint8_t valor = total ? uint8_t((uint64_t(recebido) * 99) / total) : 0;
        if (valor != percentual) notificar(valor, "Gravando pela IDE");
    });
    ArduinoOTA.onEnd([this]()
    {
        recebendoIDE = false;
        liberarTela = false;
        reinicioPendente = true;
        reinicioEm = millis();
        notificar(100, "Concluido! Reiniciando");
        Serial.println("[ArduinoOTA] Firmware validado. Reiniciando em 2 segundos.");
    });
    ArduinoOTA.onError([this](ota_error_t erro)
    {
        String mensagem;
        switch (erro)
        {
            case OTA_AUTH_ERROR: mensagem = "Falha de autenticacao ArduinoOTA"; break;
            case OTA_BEGIN_ERROR: mensagem = "Falha ao iniciar gravacao ArduinoOTA"; break;
            case OTA_CONNECT_ERROR: mensagem = "Falha de conexao com o computador da IDE"; break;
            case OTA_RECEIVE_ERROR: mensagem = "Falha na recepcao do firmware pela IDE"; break;
            case OTA_END_ERROR: mensagem = "Falha na validacao final ArduinoOTA"; break;
            default: mensagem = "Falha ArduinoOTA"; break;
        }
        if (Update.hasError()) { mensagem += ": "; mensagem += Update.errorString(); }
        falhar(mensagem);
    });
}

void ServicoOTA::atenderArduinoOTA()
{
    // Ambos usam a instancia global Update; nunca atende IDE durante OTA HTTP.
    if (recebendo || fimRecebido || reinicioPendente) return;
    const bool disponivel = WiFi.status() == WL_CONNECTED && capacidade() > 0;
    const uint32_t ip = disponivel ? uint32_t(WiFi.localIP()) : 0;
    if (arduinoOTAIniciado && (!disponivel || ip != ipArduinoOTA))
    {
        ArduinoOTA.end();
        arduinoOTAIniciado = false;
        ipArduinoOTA = 0;
    }
    if (!disponivel || ip == 0) return;
    if (!arduinoOTAIniciado)
    {
        ArduinoOTA.begin();
        arduinoOTAIniciado = true;
        ipArduinoOTA = ip;
        Serial.print("[ArduinoOTA] Porta de rede: "); Serial.print(Configuracao::HOSTNAME_OTA);
        Serial.print(" | IP: "); Serial.print(WiFi.localIP());
        Serial.print(" | Porta: "); Serial.print(Configuracao::PORTA_OTA);
        Serial.println(" | Sem senha");
    }
    // A biblioteca recebe o firmware aqui e atualiza a tela pelos callbacks.
    ArduinoOTA.handle();
}

void ServicoOTA::publicarEstado()
{
    Estado copia;
    copia.implementado = true;
    copia.modo = reinicioPendente ? Modo::Reiniciando : recebendoIDE ? Modo::IDE :
        (recebendo || fimRecebido) ? Modo::Web : liberarTela ? Modo::Falha : Modo::Disponivel;
    copia.progresso = percentual;
    copia.ocupado = ocupado();
    copia.ideDisponivel = arduinoOTAIniciado;
    copia.capacidade = capacidadeInicial;
    portENTER_CRITICAL(&mutexEstado);
    estadoPublicado = copia;
    portEXIT_CRITICAL(&mutexEstado);
}

ServicoOTA::Estado ServicoOTA::obterEstado() const
{
    portENTER_CRITICAL(&mutexEstado);
    const Estado copia = estadoPublicado;
    portEXIT_CRITICAL(&mutexEstado);
    return copia;
}
