#pragma once
#include <Arduino.h>
#include <U8g2lib.h>

// O estado deve ser acessado sob o mutex da tela; o desenho pertence a tarefa OLED.
class ProtecaoTela
{
public:
    void registrarAtividade(uint32_t agora)
    {
        ultimaAtividade = agora;
        ativa = false;
    }

    bool tocar(uint32_t agora)
    {
        const bool acordando = ativa || uint32_t(agora - ultimaAtividade) >= TEMPO_MS;
        registrarAtividade(agora);
        return !acordando;
    }

    bool atualizar(uint32_t agora)
    {
        ativa = uint32_t(agora - ultimaAtividade) >= TEMPO_MS;
        return ativa;
    }

    static void desenhar(U8G2& display, uint32_t agora)
    {
        const uint32_t quadro = agora / 250;
        const uint8_t horizontal = (quadro / 2) % 64;
        const uint8_t vertical = (quadro / 3) % 16;
        const int x = 8 + (horizontal <= 32 ? horizontal : 64 - horizontal);
        const int y = 10 + (vertical <= 8 ? vertical : 16 - vertical);
        const uint8_t expressao = (quadro / 16) % 4;
        const bool piscando = quadro % 16 == 14;
        display.clearBuffer();
        display.setDrawColor(1);
        display.drawRFrame(x, y, 80, 40, 7);
        display.drawVLine(x + 40, y - 6, 6);
        display.drawDisc(x + 40, y - 7, 2);
        display.drawRFrame(x - 4, y + 13, 4, 14, 1);
        display.drawRFrame(x + 80, y + 13, 4, 14, 1);
        for (uint8_t olho = 0; olho < 2; ++olho)
        {
            const int olhoX = x + 17 + olho * 36;
            if (piscando || (expressao == 2 && olho == 1))
            {
                display.drawHLine(olhoX, y + 18, 12);
            }
            else if (expressao == 1)
            {
                display.drawLine(olhoX, y + 18, olhoX + 6, y + 12);
                display.drawLine(olhoX + 6, y + 12, olhoX + 12, y + 18);
            }
            else
            {
                const int olhar = int((quadro / 5) % 3) - 1;
                display.drawRBox(olhoX + olhar, y + 10, 12, 12, 3);
                display.setDrawColor(0);
                display.drawBox(olhoX + olhar + 4, y + 13, 4, 6);
                display.setDrawColor(1);
            }
        }
        if (expressao == 3)
        {
            display.drawCircle(x + 40, y + 30, 4);
        }
        else
        {
            display.drawLine(x + 29, y + 28, x + 34, y + 32);
            display.drawHLine(x + 34, y + 32, 12);
            display.drawLine(x + 46, y + 32, x + 51, y + 28);
        }
        display.sendBuffer();
    }

private:
    static constexpr uint32_t TEMPO_MS = 5UL * 60 * 1000;
    uint32_t ultimaAtividade = 0;
    bool ativa = false;
};
