#pragma once
#include "screen.h"

#define MAX_SCREEN_STACK 8

class ScreenManager {
public:
    // Открыть новый экран (поверх текущего)
    void push(Screen* screen);

    // Закрыть текущий экран, вернуться к предыдущему
    void pop();

    // Заменить текущий экран (без сохранения в стек)
    void replace(Screen* screen);

    // Текущий экран
    Screen* current();

    // Отрисовать текущий экран
    void draw(Arduino_Canvas* gfx);

    // Обработать касание на текущем экране
    bool handleTouch(int x, int y, TouchEvent ev);

private:
    Screen* stack[MAX_SCREEN_STACK];
    int depth = 0;
};