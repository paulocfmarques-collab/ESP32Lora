#include "ControlePaginas.h"
#include "Configuracao.h"

ControlePaginas::ControlePaginas(AtualizarTela atualizar, void* contexto)
    : atualizar(atualizar), contexto(contexto)
{
}

bool ControlePaginas::iniciar()
{
    if (tarefaBotao != nullptr)
        return true;
    pinMode(Configuracao::BOTAO_PAGINA, INPUT_PULLUP);
    if (tarefaInterface == nullptr &&
        xTaskCreatePinnedToCore(executarInterface, "oled", 4096, this, 1, &tarefaInterface,
                                Configuracao::CORE_INTERFACE) != pdPASS)
    {
        return false;
    }
    // Prioridade maior para manter a leitura regular durante o envio ao OLED.
    if (xTaskCreatePinnedToCore(executarBotao, "botao", 2048, this, 2, &tarefaBotao,
                                Configuracao::CORE_INTERFACE) != pdPASS)
    {
        // Mantem a interface ativa para apresentar a mensagem de erro.
        return false;
    }
    return true;
}

void ControlePaginas::executarBotao(void* contexto)
{
    static_cast<ControlePaginas*>(contexto)->lerBotao();
}

void ControlePaginas::executarInterface(void* contexto)
{
    auto& controle = *static_cast<ControlePaginas*>(contexto);
    for (;;)
    {
        // A notificacao acumula toques mesmo quando o OLED esta ocupado.
        const uint32_t toques = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));
        if (toques == 0)
        {
            controle.atualizar(controle.contexto, false);
        }
        else
        {
            for (uint32_t i = 0; i < toques; ++i)
            {
                controle.atualizar(controle.contexto, true);
            }
        }
    }
}

void ControlePaginas::lerBotao()
{
    // Se ligado com o botao pressionado, exige soltar antes do primeiro toque.
    int ultimaLeitura = digitalRead(Configuracao::BOTAO_PAGINA);
    int estadoEstavel = ultimaLeitura;
    uint32_t ultimaMudanca = millis();
    TickType_t ultimaExecucao = xTaskGetTickCount();
    for (;;)
    {
        const uint32_t agora = millis();
        const int leitura = digitalRead(Configuracao::BOTAO_PAGINA);
        if (leitura != ultimaLeitura)
        {
            ultimaLeitura = leitura;
            ultimaMudanca = agora;
        }
        if (leitura != estadoEstavel &&
            uint32_t(agora - ultimaMudanca) >= Configuracao::DEBOUNCE_BOTAO_MS)
        {
            estadoEstavel = leitura;
            if (estadoEstavel == LOW)
            {
                xTaskNotifyGive(tarefaInterface);
            }
        }
        vTaskDelayUntil(&ultimaExecucao, pdMS_TO_TICKS(10));
    }
}
