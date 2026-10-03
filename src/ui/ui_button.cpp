#include "ui_button.h"
#include <Arduino_GFX_Library.h>

void UIButton::draw(Arduino_Canvas* gfx) {
    uint16_t fill = isPressed ? WHITE : color;
    uint16_t text = isPressed ? BLACK : WHITE;

    gfx->fillRect(x, y, w, h, fill);
    gfx->drawRect(x, y, w, h, WHITE);

    gfx->setTextColor(text);
    gfx->setTextSize(2);
    gfx->setCursor(x + 20, y + 25);
    gfx->print(label);
}

bool UIButton::contains(int px, int py) {
    return px >= x && px <= x + w &&
           py >= y && py <= y + h;
}