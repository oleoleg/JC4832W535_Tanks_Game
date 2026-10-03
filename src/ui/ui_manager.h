#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "ui_button.h"
#include "ui_event.h" 

#define MAX_BUTTONS 16

class UIManager {
public:
    void addButton(int x, int y, int w, int h,
                   const char* label, uint16_t color,
                   ButtonCallback callback);

    void clearButtons();

    // Нарисовать все кнопки
    void draw(Arduino_Canvas* gfx);

    // Обработать касание. Возвращает true, если попали в кнопку.
    bool handleTouch(int x, int y, TouchEvent ev);
    
private:
    UIButton buttons[MAX_BUTTONS];
    int buttonCount = 0;
    int activeButton = -1;
};