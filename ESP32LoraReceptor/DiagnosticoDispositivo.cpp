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
        return "POWERON";
    case ESP_RST_SW:
        return "SOFTWARE";
    case ESP_RST_PANIC:
        return "PANIC";
    case ESP_RST_INT_WDT:
        return "INT_WATCHDOG";
    case ESP_RST_TASK_WDT:
        return "TASK_WATCHDOG";
    case ESP_RST_WDT:
        return "WATCHDOG";
    case ESP_RST_BROWNOUT:
        return "BROWNOUT";
    case ESP_RST_DEEPSLEEP:
        return "DEEP_SLEEP";
    case ESP_RST_EXT:
        return "EXTERNAL";
    default:
        return "UNKNOWN";
    }
}

String DiagnosticoDispositivo::redeInfo()
{
    const bool conectado = WiFi.status() == WL_CONNECTED;
    String r = String("NET_INFO\nCONECTADO: ") + (conectado ? "sim" : "nao");
    r += "\nSSID: " + json(conectado ? WiFi.SSID() : WiFi.softAPSSID());
    r += "\nIP: " + ipAcesso().toString();
    r += "\nGATEWAY_STA: " + WiFi.gatewayIP().toString();
    r += "\nMASCARA_STA: " + WiFi.subnetMask().toString();
    r += "\nDNS_STA: " + WiFi.dnsIP().toString();
    r += "\nMAC_STA: " + WiFi.macAddress();
    r += "\nAP_SSID: " + json(WiFi.softAPSSID());
    r += "\nAP_IP: " + WiFi.softAPIP().toString();
    r += "\nAP_CLIENTES: " + String(WiFi.softAPgetStationNum());
    r += "\nRSSI_DBM: " + (conectado ? String(WiFi.RSSI()) : String("N/D"));
    r +=
        "\nPORTA_UDP: " + (udpLocal ? String(Configuracao::PORTA_UDP) : String("N/D via gateway")) +
        "\n";
    return r;
}

String DiagnosticoDispositivo::radioInfo()
{
    const auto estado = radio.obterEstado();
    const auto app = monitor.obter();
    String r = String("RADIO\nINICIALIZADO: ") + (estado.inicializado ? "sim" : "nao");
    r += "\nMODO: " + String(RadioLoRa::nomeModo(estado.modo));
    r += "\nULTIMO_CODIGO: " + String(estado.codigo);
    r += "\nFASE_APLICACAO: " + String(app.fase);
    r += "\nPACOTE: " + String(app.pacote) + "\nTENTATIVA: " + String(app.tentativa);
    r += "\nFREQUENCIA_MHZ: " + String(Configuracao::FREQUENCIA_MHZ, 1);
    r += "\nBANDWIDTH_KHZ: " + String(Configuracao::LARGURA_BANDA_KHZ, 1);
    r += "\nSF: " + String(Configuracao::SPREADING_FACTOR);
    r += "\nCR: 4/" + String(Configuracao::CODING_RATE);
    r += "\nSYNC_WORD: 0x" + String(Configuracao::SYNC_WORD, HEX);
    r += "\nPOTENCIA_DBM: " + String(int(Configuracao::POTENCIA_DBM));
    r += "\nPREAMBULO: " + String(Configuracao::PREAMBULO);
    r += "\nULTIMO_RSSI_DBM: " + (estado.temRecepcao ? numero(estado.rssi) : String("N/D"));
    r += "\nULTIMO_SNR_DB: " + (estado.temRecepcao ? numero(estado.snr) : String("N/D"));
    r += "\nERRO_FREQUENCIA_HZ: " +
         (estado.temRecepcao ? numero(estado.erroFrequencia) : String("N/D"));
    r +=
        "\nRECEPCAO_IDADE_MS: " +
        (estado.temRecepcao ? String(uint32_t(millis() - estado.ultimaRecepcaoEm)) : String("N/D"));
    r += "\nULTIMO_PEER: " + String(app.ultimoPeer) + "\nULTIMO_ACK_ID: " + String(app.ultimoACK) +
         "\n";
    return r;
}

String DiagnosticoDispositivo::contadoresInfo()
{
    const auto c = monitor.obter().contadores;
    return String("COUNTERS\nCONFIRMADOS: ") + c.pacotesConfirmados +
           "\nNAO_CONFIRMADOS: " + c.pacotesNaoConfirmados +
           "\nRETRANSMISSOES: " + c.totalRetransmissoes + "\nACKS_INVALIDOS: " + c.acksInvalidos +
           "\nERROS_CRC: " + c.errosCRC + "\nTIMEOUTS_ACK: " + c.timeoutsACK +
           "\nERROS_TX: " + c.errosTransmissao + "\nCOMANDOS: " + c.comandosRecebidos +
           "\nRESPOSTAS: " + c.respostasEnviadas + "\nFRAGMENTOS_RX: " + c.fragmentosRecebidos +
           "\nFRAGMENTOS_TX: " + c.fragmentosEnviados + "\nDUPLICADOS: " + c.pacotesDuplicados +
           "\n";
}

String DiagnosticoDispositivo::bateriaInfo()
{
    const auto b = bateria.obter();
    return String("BATTERY\nLEITURA_VALIDA: ") + (b.valida ? "sim" : "nao") +
           "\nTENSAO_V: " + (b.valida ? numero(b.volts, 3) : String("N/D")) +
           "\nADC_MV: " + String(b.adcMV) +
           "\nIDADE_MS: " + (b.lidaEm ? String(uint32_t(millis() - b.lidaEm)) : String("N/D")) +
           "\nDIVISOR: " + String(Configuracao::DIVISOR_BATERIA, 3) +
           "\nCALIBRACAO: " + String(Configuracao::CALIBRACAO_BATERIA, 4) + "\n";
}

String DiagnosticoDispositivo::otaInfo()
{
    const auto ota = web.estadoOTA();
    return String("OTA_STATUS\nMODO: ") + modoOTA(ota.modo) + "\nPROGRESSO: " + ota.progresso +
           "\nSUPORTADO: " + (ota.implementado ? "sim" : "nao") +
           "\nOCUPADO: " + (ota.ocupado ? "sim" : "nao") +
           "\nARDUINO_IDE: " + (ota.ideDisponivel ? "disponivel" : "indisponivel") +
           "\nHOSTNAME: " + Configuracao::HOSTNAME_OTA + "\nPORTA_IDE: " + Configuracao::PORTA_OTA +
           "\nCAPACIDADE_BYTES: " + String((unsigned long)ota.capacidade) + "\n";
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
        return String("STATUS\nNAME: ") + endereco.obter() + "\nUPTIME_S: " + segundos +
               "\nRESET: " + motivoReset() + "\nCPU_TEMP_C: " + numero(temperaturaAtual(), 1) +
               "\nHEAP_LIVRE_BYTES: " + ESP.getFreeHeap() + "\nFLASH_BYTES: " + flashBytes +
               "\nWIFI: " + (conectado ? "conectado" : "acesso_local") + "\nSSID: " + json(rede) +
               "\nIP: " + ipAcesso().toString() + "\nRADIO: " + RadioLoRa::nomeModo(r.modo) +
               "\nRADIO_CODIGO: " + r.codigo + "\nFASE: " + a.fase + "\nPACOTE: " + a.pacote +
               "\nCONFIRMADOS: " + a.contadores.pacotesConfirmados +
               "\nBATERIA_V: " + (b.valida ? numero(b.volts, 3) : String("N/D")) +
               "\nOTA: " + (o.implementado ? modoOTA(o.modo) : "nao_implementado") +
               "\nOTA_PROGRESSO: " + o.progresso + "\n";
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
        return "COMANDOS\nhelp\nname\nalive\ninfo\nstatus\nstatus_json\ninfo_"
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
        return String("UPTIME_S: ") + (unsigned long)(esp_timer_get_time() / 1000000LL) + "\n";
    if (cmd == "reason")
        return String("RESET: ") + motivoReset() + "\n";
    if (cmd == "temp")
        return String("CPU_TEMP_C: ") + numero(temperaturaAtual(), 1) + "\n";
    if (cmd == "cpu")
        return String("CPU\nMODELO: ") + ESP.getChipModel() +
               "\nREVISAO: " + ESP.getChipRevision() + "\nNUCLEOS: " + ESP.getChipCores() +
               "\nFREQUENCIA_MHZ: " + ESP.getCpuFreqMHz() +
               "\nTEMPERATURA_C: " + numero(temperaturaAtual(), 1) + "\n";
    if (cmd == "heap")
        return String("Heap livre: ") + (ESP.getFreeHeap() / 1024) + " KB\n";
    if (cmd == "ram")
    {
        const uint32_t total = ESP.getHeapSize(), livre = ESP.getFreeHeap();
        return String("RAM\nHEAP_TOTAL_BYTES: ") + total + "\nHEAP_LIVRE_BYTES: " + livre +
               "\nHEAP_USADO_BYTES: " + (total >= livre ? total - livre : 0) +
               "\nHEAP_MINIMO_BYTES: " + ESP.getMinFreeHeap() +
               "\nMAIOR_BLOCO_BYTES: " + ESP.getMaxAllocHeap() +
               "\nPSRAM_TOTAL_BYTES: " + ESP.getPsramSize() +
               "\nPSRAM_LIVRE_BYTES: " + ESP.getFreePsram() + "\n";
    }
    if (cmd == "flash")
    {
        const auto* particao = esp_ota_get_running_partition();
        String r = String("FLASH\nCAPACIDADE_BYTES: ") + flashBytes +
                   "\nFREQUENCIA_HZ: " + flashHz + "\nSKETCH_BYTES: " + sketchBytes +
                   "\nCAPACIDADE_OTA_BYTES: " + slotOTA;
        if (particao)
            r += String("\nPARTICAO_ATUAL: ") + particao->label +
                 "\nPARTICAO_BYTES: " + particao->size + "\nENDERECO: 0x" +
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
        return String("UDP_STATUS\nLISTENER_LOCAL: ") + (udpLocal ? "sim" : "nao") +
               "\nTRANSPORTE: " + (udpLocal ? "UDP" : "LoRa via gateway") + "\n";
    return "ERRO: comando desconhecido. Envie help.\n";
}
