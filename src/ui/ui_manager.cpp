#include "ui_manager.h"

void UIManager::addButton(int x, int y, int w, int h,
                          const char* label, uint16_t color,
                          ButtonCallback callback) {
    if (buttonCount >= MAX_BUTTONS) return;
    buttons[buttonCount++] = { x, y, w, h, label, color, callback, false };
}

void UIManager::clearButtons() {
    buttonCount = 0;
    activeButton = -1;
}

void UIManager::loop(Arduino_Canvas* gfx,
                     bool (*readTouch)(int&, int&),
                     void (*flush)()) {
    int tx = 0, ty = 0;
    bool touched = readTouch(tx, ty);

    // Отпускание: палец ушёл, а был на кнопке — вызвать RELEASED
    if (!touched && activeButton >= 0) {
        buttons[activeButton].isPressed = false;
        if (buttons[activeButton].callback)
            buttons[activeButton].callback(ButtonEvent::RELEASED);
        activeButton = -1;
    }

    // Новое касание
    if (touched && activeButton < 0) {
        for (int i = 0; i < buttonCount; i++) {
            if (buttons[i].contains(tx, ty)) {
                activeButton = i;
                buttons[i].isPressed = true;
                if (buttons[i].callback)
                    buttons[i].callback(ButtonEvent::PRESSED);
                break;
            }
        }
    }

    // Отрисовка
    for (int i = 0; i < buttonCount; i++) {
        buttons[i].draw(gfx);
    }

    flush();
}