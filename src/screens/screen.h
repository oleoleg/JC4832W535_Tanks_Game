#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "../ui/ui_event.h"  


// Базовый класс для всех экранов приложения.
// Каждый экран реализует эти методы по-своему.
class Screen {
public:
    virtual ~Screen() {}

    // Вызывается один раз при входе на экран
    virtual void onEnter() {}

    // Вызывается один раз при выходе с экрана
    virtual void onExit() {}

    // Вызывается каждый кадр для отрисовки
    virtual void draw(Arduino_Canvas* gfx) = 0;

    // Вызывается при касании экрана.
    // Возвращает true, если касание обработано.
    virtual bool handleTouch(int x, int y, TouchEvent ev) { return false; }

    // Имя экрана (для отладки)
    virtual const char* name() const { return "Screen"; }

    // Вызывается каждый кадр ПЕРЕД draw — для обновления состояния
    virtual void update() {}

};