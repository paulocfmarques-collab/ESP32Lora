#include "TarefaComunicacao.h"
#include "ConfiguracaoComum.h"

TarefaComunicacao::TarefaComunicacao(ExecutarCiclo ciclo, void* contexto)
    : ciclo(ciclo), contexto(contexto) {}

bool TarefaComunicacao::iniciar()
{
    if (tarefa != nullptr) return true;
    return xTaskCreatePinnedToCore(executarTarefa, "lora", 8192, this, 1,
                                   &tarefa, Configuracao::CORE_COMUNICACAO) == pdPASS;
}

void TarefaComunicacao::executarTarefa(void* contexto)
{
    auto& comunicacao = *static_cast<TarefaComunicacao*>(contexto);
    for (;;)
    {
        comunicacao.ciclo(comunicacao.contexto);
        // Libera tempo para as demais tarefas mesmo apos erros imediatos.
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
