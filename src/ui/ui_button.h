#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "ui_event.h"

struct UIButton {
    int x, y, w, h;
    const char* label;
    uint16_t color;
    ButtonCallback callback;   // может быть nullptr

    // Внутреннее состояние
    bool isPressed;

    void draw(Arduino_Canvas* gfx);
    bool contains(int px, int py);
};