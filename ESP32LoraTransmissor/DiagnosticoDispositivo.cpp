#include "../DiagnosticoDispositivo.h"
#include "ServidorConfiguracao.h"
#include "Configuracao.h"
#include <WiFi.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_ota_ops.h>
#include <cmath>

const char* DiagnosticoDispositivo::motivoReset()
{
    switch (esp_reset_reason())
    {
    case ESP_RST_POWERON:
        return "PowerOn";
    case ESP_RST_SW:
        return "Software";
    case ESP_RST_PANIC:
        return "Panic";
    case ESP_RST_INT_WDT:
        return "Int_watchdog";
    case ESP_RST_TASK_WDT:
        return "Task_watchdog";
    case ESP_RST_WDT:
        return "Watchdog";
    case ESP_RST_BROWNOUT:
        return "Brownout";
    case ESP_RST_DEEPSLEEP:
        return "Deep_sleep";
    case ESP_RST_EXT:
        return "External";
    default:
        return "Unknown";
    }
}

String DiagnosticoDispositivo::redeInfo()
{
    const bool conectado = WiFi.status() == WL_CONNECTED;
    String r = String("Net_Info\nConectado: ") + (conectado ? "sim" : "nao");
    r += "\nSSID: " + json(conectado ? WiFi.SSID() : WiFi.softAPSSID());
    r += "\nIP: " + ipAcesso().toString();
    r += "\nGateway_STA: " + WiFi.gatewayIP().toString();
    r += "\nMascara_STA: " + WiFi.subnetMask().toString();
    r += "\nDNS_STA: " + WiFi.dnsIP().toString();
    r += "\nMAC_STA: " + WiFi.macAddress();
    r += "\nAP_SSID: " + json(WiFi.softAPSSID());
    r += "\nAP_IP: " + WiFi.softAPIP().toString();
    r += "\nAP_Clientes: " + String(WiFi.softAPgetStationNum());
    r += "\nRSSI_dBm: " + (conectado ? String(WiFi.RSSI()) : String("N/D"));
    r +=
        "\nPorta_UDP: " + (udpLocal ? String(Configuracao::PORTA_UDP) : String("N/D via gateway")) +
        "\n";
    return r;
}

String DiagnosticoDispositivo::radioInfo()
{
    const auto estado = radio.obterEstado();
    const auto app = monitor.obter();
    String r = String("Radio\nInicializado: ") + (estado.inicializado ? "sim" : "nao");
    r += "\nModo: " + String(RadioLoRa::nomeModo(estado.modo));
    r += "\nUltimo_Codigo: " + String(estado.codigo);
    r += "\nFase_Aplicacao: " + String(app.fase);
    r += "\nPacote: " + String(app.pacote) + "\nTentativa: " + String(app.tentativa);
    r += "\nFrequencia_MHz: " + String(Configuracao::FREQUENCIA_MHZ, 1);
    r += "\nBandwidth_KHz: " + String(Configuracao::LARGURA_BANDA_KHZ, 1);
    r += "\nSF: " + String(Configuracao::SPREADING_FACTOR);
    r += "\nCR: 4/" + String(Configuracao::CODING_RATE);
    r += "\nSync_Word: 0x" + String(Configuracao::SYNC_WORD, HEX);
    r += "\nPotencia_dBm: " + String(int(Configuracao::POTENCIA_DBM));
    r += "\nPreambulo: " + String(Configuracao::PREAMBULO);
    r += "\nUltimo_RSSI_dBm: " + (estado.temRecepcao ? numero(estado.rssi) : String("N/D"));
    r += "\nUltimo_SNR_dB: " + (estado.temRecepcao ? numero(estado.snr) : String("N/D"));
    r += "\nErro_Frequencia_Hz: " +
         (estado.temRecepcao ? numero(estado.erroFrequencia) : String("N/D"));
    r +=
        "\nRecepcao_Idade_ms: " +
        (estado.temRecepcao ? String(uint32_t(millis() - estado.ultimaRecepcaoEm)) : String("N/D"));
    r += "\nUltimo_Peer: " + String(app.ultimoPeer) + "\nUltimo_ACK_ID: " + String(app.ultimoACK) +
         "\n";
    return r;
}

String DiagnosticoDispositivo::contadoresInfo()
{
    const auto c = monitor.obter().contadores;
    return String("Counters\nConfirmados: ") + c.pacotesConfirmados +
           "\nNao_Confirmados: " + c.pacotesNaoConfirmados +
           "\nRetransmissoes: " + c.totalRetransmissoes + "\nAcks_Invalidos: " + c.acksInvalidos +
           "\nErros_CRC: " + c.errosCRC + "\nTimeouts_Ack: " + c.timeoutsACK +
           "\nErros_Tx: " + c.errosTransmissao + "\nComandos: " + c.comandosRecebidos +
           "\nRespostas: " + c.respostasEnviadas + "\nFragmentos_Rx: " + c.fragmentosRecebidos +
           "\nFragmentos_Tx: " + c.fragmentosEnviados + "\nDuplicados: " + c.pacotesDuplicados +
           "\n";
}

String DiagnosticoDispositivo::bateriaInfo()
{
    const auto b = bateria.obter();
    return String("Battery\nLeitura_Valida: ") + (b.valida ? "sim" : "nao") +
           "\nTensao_V: " + (b.valida ? numero(b.volts, 3) : String("N/D")) +
           "\nADC_mV: " + String(b.adcMV) +
           "\nIdade_ms: " + (b.lidaEm ? String(uint32_t(millis() - b.lidaEm)) : String("N/D")) +
           "\nDivisor: " + String(Configuracao::DIVISOR_BATERIA, 3) +
           "\nCalibracao: " + String(Configuracao::CALIBRACAO_BATERIA, 4) + "\n";
}

String DiagnosticoDispositivo::otaInfo()
{
    const auto ota = web.estadoOTA();
    return String("OTA_Status\nMODO: ") + modoOTA(ota.modo) + "\nProgresso: " + ota.progresso +
           "\nSuportado: " + (ota.implementado ? "sim" : "nao") +
           "\nOcupado: " + (ota.ocupado ? "sim" : "nao") +
           "\nArduino_IDE: " + (ota.ideDisponivel ? "disponivel" : "indisponivel") +
           "\nHostname: " + Configuracao::HOSTNAME_OTA + "\nPorta_IDE: " + Configuracao::PORTA_OTA +
           "\nCapacidade_Bytes: " + String((unsigned long)ota.capacidade) + "\n";
}

String DiagnosticoDispositivo::resumo(bool formatoJSON)
{
    const auto r = radio.obterEstado();
    const auto a = monitor.obter();
    const auto b = bateria.obter();
    const auto o = web.estadoOTA();
    const bool conectado = WiFi.status() == WL_CONNECTED;
    const String rede = conectado ? WiFi.SSID() : WiFi.softAPSSID();
    const unsigned long segundos = (unsigned long)(esp_timer_get_time() / 1000000LL);
    if (!formatoJSON)
    {
        return String("Status\nName: ") + endereco.obter() + "\nUptime_s: " + segundos +
               "\nReset: " + motivoReset() + "\nCPU_Temp_C: " + numero(temperaturaAtual(), 1) +
               "\nHeap_Livre_Bytes: " + ESP.getFreeHeap() + "\nFlash_Bytes: " + flashBytes +
               "\nWiFi: " + (conectado ? "conectado" : "acesso_local") + "\nSSID: " + json(rede) +
               "\nIP: " + ipAcesso().toString() + "\nRadio: " + RadioLoRa::nomeModo(r.modo) +
               "\nRadio_Codigo: " + r.codigo + "\nFase: " + a.fase + "\nPacote: " + a.pacote +
               "\nConfirmados: " + a.contadores.pacotesConfirmados +
               "\nBateria_V: " + (b.valida ? numero(b.volts, 3) : String("N/D")) +
               "\nOTA: " + (o.implementado ? modoOTA(o.modo) : "nao_implementado") +
               "\nOTA_Progresso: " + o.progresso + "\n";
    }
    String s = String("{\"id\":") + json(endereco.obter()) + ",\"up_s\":" + segundos;
    s += ",\"reset\":" + json(motivoReset()) + ",\"cpu_c\":" + numero(temperaturaAtual(), 1);
    s += String(",\"ram\":{\"free\":") + ESP.getFreeHeap() + ",\"total\":" + ESP.getHeapSize() +
         ",\"min\":" + ESP.getMinFreeHeap() + ",\"largest\":" + ESP.getMaxAllocHeap() + "}";
    s += String(",\"flash\":{\"bytes\":") + flashBytes + ",\"sketch\":" + sketchBytes + "}";
    s += String(",\"wifi\":{\"connected\":") + (conectado ? "true" : "false") +
         ",\"ssid\":" + json(rede) + ",\"ip\":" + json(ipAcesso().toString()) +
         ",\"rssi\":" + (conectado ? String(WiFi.RSSI()) : String("null")) + "}";
    s += String(",\"radio\":{\"initialized\":") + (r.inicializado ? "true" : "false") +
         ",\"mode\":" + json(RadioLoRa::nomeModo(r.modo)) + ",\"code\":" + r.codigo +
         ",\"rssi\":" + (r.temRecepcao ? numero(r.rssi) : String("null")) +
         ",\"snr\":" + (r.temRecepcao ? numero(r.snr) : String("null")) + "}";
    s += String(",\"app\":{\"phase\":") + json(a.fase) + ",\"packet\":" + a.pacote +
         ",\"attempt\":" + a.tentativa + ",\"last_peer\":" + json(a.ultimoPeer) +
         ",\"ack_id\":" + a.ultimoACK +
         ",\"result\":" + (a.temResultado ? (a.confirmado ? "true" : "false") : "null") + "}";
    const auto& c = a.contadores;
    s += String(",\"counts\":{\"ok\":") + c.pacotesConfirmados +
         ",\"failed\":" + c.pacotesNaoConfirmados + ",\"retry\":" + c.totalRetransmissoes +
         ",\"bad_ack\":" + c.acksInvalidos + ",\"crc\":" + c.errosCRC +
         ",\"timeout\":" + c.timeoutsACK + ",\"tx_error\":" + c.errosTransmissao + "}";
    s += String(",\"battery\":{\"valid\":") + (b.valida ? "true" : "false") +
         ",\"v\":" + (b.valida ? numero(b.volts, 3) : String("null")) +
         ",\"age_ms\":" + (b.lidaEm ? String(uint32_t(millis() - b.lidaEm)) : String("null")) + "}";
    s += String(",\"ota\":{\"supported\":") + (o.implementado ? "true" : "false") +
         ",\"mode\":" + json((o.implementado ? modoOTA(o.modo) : "nao_implementado")) +
         ",\"progress\":" + o.progresso + ",\"ide\":" + (o.ideDisponivel ? "true" : "false") +
         "}}\n";
    return s;
}

String DiagnosticoDispositivo::consultar(const String& comando)
{
    String cmd = comando;
    cmd.trim();
    cmd.toLowerCase();
    if (cmd == "help" || cmd == "ajuda" || cmd == "?")
        return "Comandos\nhelp\nname\nalive\ninfo\nstatus\nstatus_json\ninfo_"
               "json\nversion\nbuild\nreason\nuptime\ntemp\ncpu\nram\nheap\nflash\nnet_"
               "info\nip\nmac\nrssi\nradio\nradio_status\ncounters\nbattery\nvbat\nota_status\nudp_"
               "status\n";
    if (cmd == "name")
        return endereco.obter() + "\n";
    if (cmd == "alive")
        return "alive\n";
    if (cmd == "info" || cmd == "status")
        return resumo(false);
    if (cmd == "status_json" || cmd == "info_json")
        return resumo(true);
    if (cmd == "build" || cmd == "version")
        return String(Configuracao::NOME_PROJETO) + " " + __DATE__ + " " + __TIME__ + "\n";
    if (cmd == "uptime")
        return String("Uptime: ") + (unsigned long)(esp_timer_get_time() / 1000000LL) + " s\n";
    if (cmd == "reason")
        return String("RESET: ") + motivoReset() + "\n";
    if (cmd == "temp")
        return String("CPU_Temperatura: ") + numero(temperaturaAtual(), 1) + " C\n";
    if (cmd == "cpu")
        return String("CPU\nModelo: ") + ESP.getChipModel() +
               "\nRevisao: " + ESP.getChipRevision() + "\nNucleos: " + ESP.getChipCores() +
               "\nFrequencia_MHz: " + ESP.getCpuFreqMHz() +
               "\nTemperatura: " + numero(temperaturaAtual(), 1) + " C\n";
    if (cmd == "heap")
        return String("Heap livre: ") + (ESP.getFreeHeap() / 1024) + " KB\n";
    if (cmd == "ram")
    {
        const uint32_t total = ESP.getHeapSize(), livre = ESP.getFreeHeap();
        return String("RAM\nHeap_Total_Bytes: ") + total + "\nHeap_Livre_Bytes: " + livre +
               "\nHeap_Usado_Bytes: " + (total >= livre ? total - livre : 0) +
               "\nHeap_Minimo_Bytes: " + ESP.getMinFreeHeap() +
               "\nMaior_Bloco_Bytes: " + ESP.getMaxAllocHeap() +
               "\nPSRAM_Total_Bytes: " + ESP.getPsramSize() +
               "\nPSRAM_Livre_Bytes: " + ESP.getFreePsram() + "\n";
    }
    if (cmd == "flash")
    {
        const auto* particao = esp_ota_get_running_partition();
        String r = String("Flash\nCapacidade_Bytes: ") + flashBytes +
                   "\nFrequencia_Hz: " + flashHz + "\nSketch_Bytes: " + sketchBytes +
                   "\nCapacidade_OTA_Bytes: " + slotOTA;
        if (particao)
            r += String("\nParticao_Atual: ") + particao->label +
                 "\nParticao_Bytes: " + particao->size + "\nEndereco: 0x" +
                 String(particao->address, HEX);
        return r + "\n";
    }
    if (cmd == "net_info")
        return redeInfo();
    if (cmd == "ip")
        return "IP: " + ipAcesso().toString() + "\n";
    if (cmd == "mac")
        return "MAC: " + WiFi.macAddress() + "\n";
    if (cmd == "rssi")
        return WiFi.status() == WL_CONNECTED ? "RSSI: " + String(WiFi.RSSI()) + " dBm\n"
                                             : "RSSI: N/D (sem conexao ao roteador)\n";
    if (cmd == "radio" || cmd == "radio_status" || cmd == "lora")
        return radioInfo();
    if (cmd == "counters")
        return contadoresInfo();
    if (cmd == "battery" || cmd == "vbat" || cmd == "batt" || cmd == "bateria")
        return bateriaInfo();
    if (cmd == "ota_status")
        return otaInfo();
    if (cmd == "udp_status")
        return String("UDP_Status\nListener_Local: ") + (udpLocal ? "sim" : "nao") +
               "\nTransporte: " + (udpLocal ? "UDP" : "LoRa via gateway") + "\n";
    return "ERRO: comando desconhecido. Envie help.\n";
}
