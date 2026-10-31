#include "ServidorConfiguracao.h"
#include "Configuracao.h"
#include <WiFi.h>
#include <esp_system.h>

ServidorConfiguracao::ServidorConfiguracao(EnderecoDispositivo& endereco, const char* tipo)
    : endereco(endereco), tipo(tipo)
{
}

bool ServidorConfiguracao::iniciar()
{
    // Sufixo fisico evita redes identicas nos receptores ainda com ID RXXX.
    const uint32_t sufixo = uint32_t(ESP.getEfuseMac() >> 24) & 0xFFFFFF;
    char identificador[7];
    snprintf(identificador, sizeof(identificador), "%06lX", (unsigned long)sufixo);
    ssid = String("LoRa-") + tipo + "-" + identificador;
    token = String(esp_random(), HEX) + String(esp_random(), HEX);
    WiFi.persistent(false);
    if (!WiFi.mode(WIFI_AP))
    {
        Serial.println("[WIFI] Falha ao ativar modo AP.");
        return false;
    }
    WiFi.setSleep(false);
    const IPAddress ip(192, 168, 4, 1);
    if (!WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0)))
    {
        Serial.println("[WIFI] Falha ao configurar IP do ponto de acesso.");
        return false;
    }
    const char* senhaAP =
        Configuracao::SENHA_WIFI_CONFIG[0] ? Configuracao::SENHA_WIFI_CONFIG : nullptr;
    if (!WiFi.softAP(ssid.c_str(), senhaAP))
    {
        Serial.println("[WIFI] Falha ao criar o ponto de acesso. Servidor HTTP nao iniciado.");
        return false;
    }
    Serial.print("[WIFI] AP ativo: ");
    Serial.println(WiFi.softAPSSID());
    servidor.on("/", HTTP_GET, [this]()
                { mostrarPagina(servidor.arg("salvo") == "1" ? "Endereço salvo na placa." : ""); });
    servidor.on("/endereco", HTTP_POST, [this]() { salvarEndereco(); });
    servidor.onNotFound(
        [this]() { servidor.send(404, "text/plain; charset=utf-8", "Pagina nao encontrada."); });
    servidor.begin();
    ativo = true;
    Serial.print("WiFi de configuracao: ");
    Serial.println(ssid);
    Serial.print("Configuracao web: http://");
    Serial.println(WiFi.softAPIP());
    return true;
}

String ServidorConfiguracao::nomeRede() const
{
    return ativo ? ssid : String("WiFi indisponivel");
}

void ServidorConfiguracao::atender()
{
    if (ativo)
        servidor.handleClient();
}

void ServidorConfiguracao::mostrarPagina(const String& aviso, int codigo)
{
    String pagina = R"HTML(<!doctype html><html lang="pt-BR"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Configuração LoRa</title><style>
*{box-sizing:border-box}body{margin:0;background:#101b2c;color:#eef4ff;font:16px system-ui,sans-serif;padding:24px}
main{max-width:480px;margin:6vh auto;background:#1b2a40;border:1px solid #354762;border-radius:20px;padding:28px}
small{color:#9edaca}h1{font-size:26px;margin:12px 0}p{line-height:1.55;color:#c4d1e4}
label{display:block;margin:24px 0 8px;font-weight:600}input,button{font:inherit;width:100%;padding:14px;border-radius:10px}
input{background:#101b2c;color:white;border:1px solid #647995;text-transform:uppercase}
button{margin-top:18px;background:#8ae0c4;border:0;font-weight:700;color:#10281f;cursor:pointer}
.aviso{padding:12px;background:#263d55;border-radius:10px}footer{font-size:13px;color:#aebed3;margin-top:24px}
</style></head><body><main><small>ESP32 · LORA</small><h1>Endereço da placa</h1><p>Dispositivo: )HTML";
    pagina += tipo;
    pagina += "</p>";
    if (aviso.length())
        pagina += String("<p class='aviso' role='status'>") + aviso + "</p>";
    pagina += "<form method='post' action='/endereco'><input type='hidden' name='token' value='";
    pagina += token;
    pagina += "'><label for='endereco'>Identificação</label><input id='endereco' name='endereco' "
              "maxlength='16' minlength='1' required autocomplete='off' value='";
    pagina += endereco.obter();
    pagina +=
        "'><p>Use de 1 a 16 letras, números, hífen ou sublinhado. Cada placa deve ter um endereço "
        "diferente. TODOS é reservado ao broadcast.</p><button type='submit'>Salvar "
        "endereço</button></form><footer>Salvo na memória da placa. O endereço permanece após "
        "desligar a placa e pode ser alterado sem reiniciar.</footer></main></body></html>";
    servidor.sendHeader("Cache-Control", "no-store");
    servidor.send(codigo, "text/html; charset=utf-8", pagina);
}

void ServidorConfiguracao::salvarEndereco()
{
    if (!servidor.hasArg("token") || servidor.arg("token") != token)
    {
        mostrarPagina("Solicitação expirada. Abra a página novamente.", 403);
        return;
    }
    String novo = servidor.arg("endereco");
    novo.trim();
    novo.toUpperCase();
    if (!EnderecoDispositivo::valido(novo))
    {
        mostrarPagina("Endereço inválido. Use de 1 a 16 caracteres permitidos; TODOS é reservado.",
                      400);
        return;
    }
    if (!endereco.salvar(novo))
    {
        mostrarPagina("Não foi possível gravar. O endereço anterior foi mantido.", 500);
        return;
    }
    servidor.sendHeader("Location", "/?salvo=1");
    servidor.send(303, "text/plain", "Endereco salvo.");
}
