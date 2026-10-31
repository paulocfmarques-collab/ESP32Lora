#pragma once
#include <Arduino.h>
#include <U8g2lib.h>

namespace LayoutTela
{
enum class Icone : uint8_t { Radio, Estatisticas, Rede, Atualizacao };

inline void texto(U8G2& display, uint8_t x, uint8_t y, const String& valor)
{
    display.setFont(u8g2_font_5x8_tf);
    const uint8_t capacidade = (128 - x) / 5;
    const size_t excesso = valor.length() > capacidade ? valor.length() - capacidade : 0;
    const size_t fase = excesso ? (millis() / 500) % (excesso + 5) : 0;
    const size_t inicio = fase > excesso ? excesso : fase;
    display.drawStr(x, y, valor.substring(inicio, inicio + capacidade).c_str());
}

inline void icone(U8G2& display, Icone tipo)
{
    if (tipo == Icone::Rede)
    {
        display.drawLine(1, 3, 5, 1);
        display.drawLine(5, 1, 10, 3);
        display.drawLine(3, 6, 5, 4);
        display.drawLine(5, 4, 8, 6);
        display.drawDisc(5, 9, 1);
    }
    else if (tipo == Icone::Estatisticas)
    {
        display.drawBox(1, 7, 2, 4);
        display.drawBox(5, 4, 2, 7);
        display.drawBox(9, 1, 2, 10);
    }
    else if (tipo == Icone::Atualizacao)
    {
        display.drawLine(2, 5, 6, 1);
        display.drawLine(6, 1, 10, 5);
        display.drawVLine(6, 1, 8);
        display.drawHLine(1, 11, 10);
    }
    else
    {
        display.drawVLine(5, 3, 8);
        display.drawDisc(5, 2, 1);
        display.drawLine(1, 4, 1, 7);
        display.drawLine(9, 4, 9, 7);
        display.drawHLine(2, 11, 7);
    }
}

inline void cabecalho(U8G2& display, const String& titulo, Icone tipo,
                      uint8_t pagina, uint8_t total)
{
    display.clearBuffer();
    display.setDrawColor(1);
    icone(display, tipo);
    display.setFont(u8g2_font_5x8_tf);
    const size_t excesso = titulo.length() > 17 ? titulo.length() - 17 : 0;
    const size_t fase = excesso ? (millis() / 500) % (excesso + 5) : 0;
    const size_t inicio = fase > excesso ? excesso : fase;
    display.drawStr(15, 9, titulo.substring(inicio, inicio + 17).c_str());
    display.drawRBox(106, 0, 22, 12, 2);
    display.setDrawColor(0);
    display.setCursor(109, 9);
    display.print(pagina + 1);
    display.print("/");
    display.print(total);
    display.setDrawColor(1);
    display.drawHLine(0, 14, 128);
}

inline void campo(U8G2& display, uint8_t y, const char* rotulo, uint32_t valor)
{
    display.setFont(u8g2_font_5x8_tf);
    display.drawStr(2, y, rotulo);
    const String numero(valor);
    display.drawStr(126 - display.getStrWidth(numero.c_str()), y, numero.c_str());
}
} // namespace LayoutTela
