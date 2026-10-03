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

void UIManager::draw(Arduino_Canvas* gfx) {
    for (int i = 0; i < buttonCount; i++) {
        buttons[i].draw(gfx);
    }
}

bool UIManager::handleTouch(int x, int y, TouchEvent ev) {
    if (ev == TouchEvent::PRESSED) {
        for (int i = 0; i < buttonCount; i++) {
            if (buttons[i].contains(x, y)) {
                activeButton = i;
                buttons[i].isPressed = true;
                if (buttons[i].callback)
                    buttons[i].callback(ButtonEvent::PRESSED);
                return true;
            }
        }
    }
    else if (ev == TouchEvent::RELEASED && activeButton >= 0) {
        buttons[activeButton].isPressed = false;
        if (buttons[activeButton].callback)
            buttons[activeButton].callback(ButtonEvent::RELEASED);
        activeButton = -1;
        return true;
    }
    return false;
}