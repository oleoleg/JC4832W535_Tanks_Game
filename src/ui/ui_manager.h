#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "ui_button.h"

#define MAX_BUTTONS 16

class UIManager {
public:
    // Добавить кнопку в текущий экран
    void addButton(int x, int y, int w, int h,
                   const char* label, uint16_t color,
                   ButtonCallback callback);

    // Очистить все кнопки (при смене экрана)
    void clearButtons();

    // Главный цикл: читает тач, обрабатывает, рисует
    void loop(Arduino_Canvas* gfx,
              bool (*readTouch)(int&, int&),
              void (*flush)());

private:
    UIButton buttons[MAX_BUTTONS];
    int buttonCount = 0;
    int activeButton = -1;   // индекс кнопки, на которой палец
    bool lastTouched = false;
};