#pragma once
#include "screen.h"

class SelectMapScreen : public Screen {
public:
    void onEnter() override;
    void draw(Arduino_Canvas* gfx) override;
    bool handleTouch(int x, int y, TouchEvent ev) override;
    const char* name() const override { return "SelectMap"; }

private:
    static constexpr int MAX_MAPS = 7;
    static constexpr int BTN_X = 60;
    static constexpr int BTN_Y0 = 70;
    static constexpr int BTN_W = 360;
    static constexpr int BTN_H = 30;
    static constexpr int BTN_GAP = 4;
    static constexpr int DEF_Y = 270;   // Y-координата кнопки DEFAULT
    
    String mapFiles[MAX_MAPS];
    int    mapCount = 0;

    void scanMaps();
};