#include "ServidorConfiguracao.h"
#include "Configuracao.h"
#include "PaginaConfiguracao.h"
#include <WiFi.h>
#include <esp_system.h>

ServidorConfiguracao::ServidorConfiguracao(EnderecoDispositivo& endereco, const char* tipo)
    : endereco(endereco), tipo(tipo) {}

bool ServidorConfiguracao::iniciar()
{
    const uint32_t sufixo = uint32_t(ESP.getEfuseMac() >> 24) & 0xFFFFFF;
    char identificador[7];
    snprintf(identificador, sizeof(identificador), "%06lX", (unsigned long)sufixo);
    ssid = String("LoRa-") + tipo + "-" + identificador;
    token = String(esp_random(), HEX) + String(esp_random(), HEX);
    WiFi.persistent(false);
    if (!WiFi.mode(WIFI_AP_STA))
    {
        Serial.println("[WIFI] Falha ao ativar modo AP + STA.");
        return false;
    }
    WiFi.setSleep(false);
    const IPAddress ip(192, 168, 4, 1);
    if (!WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0)))
    {
        Serial.println("[WIFI] Falha ao configurar IP do ponto de acesso.");
        return false;
    }
    const char* senhaAP = Configuracao::SENHA_WIFI_CONFIG[0] ? Configuracao::SENHA_WIFI_CONFIG : nullptr;
    if (!WiFi.softAP(ssid.c_str(), senhaAP))
    {
        Serial.println("[WIFI] Falha ao criar o ponto de acesso. Servidor HTTP nao iniciado.");
        return false;
    }
    Serial.print("[WIFI] AP ativo: "); Serial.println(WiFi.softAPSSID());
    if (!wifi.iniciar()) Serial.println("Preferences WiFi indisponiveis; acesso local mantido.");
    auto pagina = [this]()
    {
        String aviso;
        if (servidor.arg("salvo") == "1") aviso = "Endereço salvo na placa.";
        if (servidor.arg("wifi") == "1") aviso = "Redes salvas. O transmissor tentará conectar; a rede de configuração permanece disponível.";
        mostrarPagina(aviso);
    };
    servidor.on("/", HTTP_GET, pagina);
    servidor.on("/config", HTTP_GET, pagina);
    servidor.on("/endereco", HTTP_POST, [this]() { salvarEndereco(); });
    servidor.on("/wifi", HTTP_POST, [this]() { salvarWiFi(); });
    servidor.on("/api/wifi", HTTP_GET, [this]() { responderStatusWiFi(); });
    servidor.on("/api/wifi/scan", HTTP_POST, [this]()
    {
        if (ota.ocupado()) { servidor.send(409, "text/plain; charset=utf-8", "Atualizacao OTA em andamento."); return; }
        if (!tokenValido()) { servidor.send(403, "text/plain; charset=utf-8", "Abra a página novamente para buscar redes."); return; }
        if (!wifi.buscar()) { servidor.send(409, "text/plain; charset=utf-8", "Busca indisponível agora. Aguarde alguns segundos e tente novamente."); return; }
        servidor.send(202, "text/plain; charset=utf-8", "Busca iniciada.");
    });
    ota.iniciar(servidor, token, progressoOTA, contextoOTA);
    servidor.onNotFound([this]() { servidor.send(404, "text/plain; charset=utf-8", "Página não encontrada."); });
    servidor.begin();
    ativo = true;
    Serial.print("WiFi de configuracao: "); Serial.println(ssid);
    Serial.print("Configuracao web: http://"); Serial.println(WiFi.softAPIP());
    return true;
}

String ServidorConfiguracao::nomeRede() const
{
    return ativo ? ssid : String("WiFi indisponivel");
}

void ServidorConfiguracao::atender()
{
    if (!ativo) return;
    ota.atender();
    if (!ota.ocupado()) wifi.processar();
    servidor.handleClient();
    ota.atender();
}

String ServidorConfiguracao::escaparHTML(String valor)
{
    valor.replace("&", "&amp;"); valor.replace("<", "&lt;"); valor.replace(">", "&gt;");
    valor.replace("\"", "&quot;"); valor.replace("'", "&#39;");
    return valor;
}

String ServidorConfiguracao::escaparJSON(const String& valor)
{
    String saida = "\"";
    for (size_t i = 0; i < valor.length(); ++i)
    {
        const unsigned char c = valor[i];
        if (c == '"') saida += "\\\"";
        else if (c == '\\') saida += "\\\\";
        else if (c < 32)
        {
            char escape[7]; snprintf(escape, sizeof(escape), "\\u%04x", unsigned(c)); saida += escape;
        }
        else saida += char(c);
    }
    return saida + "\"";
}

bool ServidorConfiguracao::tokenValido()
{
    return servidor.hasArg("token") && servidor.arg("token") == token;
}

void ServidorConfiguracao::mostrarPagina(const String& aviso, int codigo)
{
    String campos;
    for (size_t i = 0; i < GerenciadorWiFi::MAX_REDES; ++i)
    {
        const String n = String((unsigned int)i);
        const auto& rede = wifi.rede(i);
        campos += String("<fieldset class='network'><legend>Rede ") + (i + 1) + "</legend>";
        campos += "<label for='ssid" + n + "'>Nome da rede (SSID)</label><input id='ssid" + n + "' name='ssid" + n + "' autocomplete='off' value='" + escaparHTML(String(rede.ssid)) + "'>";
        campos += "<label for='senha" + n + "'>Senha</label><input type='password' id='senha" + n + "' name='senha" + n + "' maxlength='64' autocomplete='new-password' placeholder='";
        campos += rede.senha[0] ? "Senha salva — deixe vazio para manter" : "Senha da rede ou vazio para rede aberta";
        campos += "'><label class='check'><input type='checkbox' id='aberta" + n + "' name='aberta" + n + "' value='1'>Rede aberta (remover senha)</label></fieldset>";
    }
    String pagina(PAGINA_CONFIGURACAO);
    // Insere campos por ultimo: SSIDs que contem chaves de template ficam literais.
    pagina.replace("{{ERRO}}", codigo >= 400 ? "error" : "");
    pagina.replace("{{AVISO}}", escaparHTML(aviso));
    pagina.replace("{{AP}}", escaparHTML(ssid));
    pagina.replace("{{ENDERECO}}", escaparHTML(endereco.obter()));
    pagina.replace("{{TOKEN}}", token);
    pagina.replace("{{REDES}}", campos);
    servidor.sendHeader("Cache-Control", "no-store");
    servidor.send(codigo, "text/html; charset=utf-8", pagina);
}

void ServidorConfiguracao::salvarEndereco()
{
    if (ota.ocupado()) { mostrarPagina("Atualização OTA em andamento. Aguarde o reinício.", 409); return; }
    if (!tokenValido()) { mostrarPagina("Solicitação expirada. Abra a página novamente.", 403); return; }
    String novo = servidor.arg("endereco"); novo.trim(); novo.toUpperCase();
    if (!EnderecoDispositivo::valido(novo))
    { mostrarPagina("Endereço inválido. Use de 1 a 16 caracteres permitidos; TODOS é reservado.", 400); return; }
    if (!endereco.salvar(novo))
    { mostrarPagina("Não foi possível gravar. O endereço anterior foi mantido.", 500); return; }
    servidor.sendHeader("Location", "/config?salvo=1");
    servidor.send(303, "text/plain", "Endereco salvo.");
}

void ServidorConfiguracao::salvarWiFi()
{
    if (ota.ocupado()) { mostrarPagina("Atualização OTA em andamento. Aguarde o reinício.", 409); return; }
    if (!tokenValido()) { mostrarPagina("Solicitação expirada. Abra a página novamente.", 403); return; }
    GerenciadorWiFi::RedeSalva novas[GerenciadorWiFi::MAX_REDES] = {};
    for (size_t i = 0; i < GerenciadorWiFi::MAX_REDES; ++i)
    {
        const String n = String((unsigned int)i);
        if (!servidor.hasArg("ssid" + n) || !servidor.hasArg("senha" + n))
        { mostrarPagina("Formulário incompleto. Abra a página novamente.", 400); return; }
        const String nome = servidor.arg("ssid" + n);
        String senha = servidor.arg("senha" + n);
        if (nome.isEmpty()) continue; // Apaga este slot e a senha associada.
        bool nomeValido = nome.length() <= 32;
        for (size_t j = 0; j < nome.length(); ++j) if (nome[j] == '\0') nomeValido = false;
        if (!nomeValido)
        { mostrarPagina("SSID inválido: o nome deve ter até 32 bytes.", 400); return; }
        const auto& anterior = wifi.rede(i);
        if (servidor.arg("aberta" + n) == "1") senha = "";
        else if (senha.isEmpty() && nome == anterior.ssid) senha = anterior.senha;
        if (!GerenciadorWiFi::senhaValida(senha))
        { mostrarPagina("Senha inválida: use 8 a 63 caracteres, 64 dígitos hexadecimais ou uma rede aberta.", 400); return; }
        nome.toCharArray(novas[i].ssid, sizeof(novas[i].ssid));
        senha.toCharArray(novas[i].senha, sizeof(novas[i].senha));
    }
    if (!wifi.salvar(novas))
    { mostrarPagina("Não foi possível gravar as redes. A configuração anterior foi mantida.", 500); return; }
    servidor.sendHeader("Location", "/config?wifi=1");
    servidor.send(303, "text/plain", "Redes salvas.");
}

void ServidorConfiguracao::responderStatusWiFi()
{
    const bool conectado = WiFi.status() == WL_CONNECTED;
    String json = String("{\"conectado\":") + (conectado ? "true" : "false");
    json += ",\"estado\":" + escaparJSON(wifi.estado());
    json += ",\"rede\":" + escaparJSON(conectado ? WiFi.SSID() : String());
    json += ",\"ip\":" + escaparJSON(conectado ? WiFi.localIP().toString() : String());
    json += ",\"rssi\":" + String(conectado ? WiFi.RSSI() : -127);
    json += String(",\"buscando\":") + (wifi.buscando() ? "true" : "false");
    json += ",\"erroBusca\":" + escaparJSON(wifi.erroDaBusca());
    json += ",\"redes\":[";
    for (size_t i = 0; i < wifi.quantidadeEncontrada(); ++i)
    {
        if (i) json += ",";
        const auto& rede = wifi.encontrada(i);
        json += "{\"ssid\":" + escaparJSON(rede.ssid) + ",\"rssi\":" + String(rede.rssi);
        json += String(",\"aberta\":") + (rede.aberta ? "true" : "false") + "}";
    }
    json += "]}";
    servidor.sendHeader("Cache-Control", "no-store");
    servidor.send(200, "application/json; charset=utf-8", json);
}

void ServidorConfiguracao::definirProgressoOTA(ServicoOTA::Progresso callback, void* contexto)
{
    progressoOTA = callback;
    contextoOTA = contexto;
}

ServidorConfiguracao::InformacoesRede ServidorConfiguracao::informacoesRede() const
{
    InformacoesRede info;
    const IPAddress ipEstacao = WiFi.localIP();
    if (ativo && WiFi.status() == WL_CONNECTED && uint32_t(ipEstacao) != 0)
    {
        info.ssid = WiFi.SSID();
        info.ip = ipEstacao.toString();
        info.conectada = true;
    }
    else
    {
        const IPAddress ipAP = WiFi.softAPIP();
        const bool apAtivo = ativo && (WiFi.getMode() & WIFI_AP) && uint32_t(ipAP) != 0;
        info.ssid = apAtivo ? ssid : String("WiFi indisponivel");
        info.ip = apAtivo ? ipAP.toString() : String("sem IP");
    }
    return info;
}
