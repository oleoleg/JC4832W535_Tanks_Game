#pragma once
#include "screen.h"
#include "../ui/ui_manager.h"

class ScreenManager;  // forward declaration

class MenuScreen : public Screen {
public:
    MenuScreen(ScreenManager* manager);
    void draw(Arduino_Canvas* gfx) override;
    bool handleTouch(int x, int y, TouchEvent ev) override;
    const char* name() const override { return "Menu"; }

private:
    ScreenManager* screens;
    UIManager ui;

    // Обработчики кнопок
    static void onGame(ButtonEvent ev);
    static void onDoorbell(ButtonEvent ev);
    static void onWifi(ButtonEvent ev);
    static void onTest(ButtonEvent ev);
};