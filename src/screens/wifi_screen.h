#pragma once
#include "screen.h"

class WiFiScreen : public Screen {
public:
    void draw(Arduino_Canvas* gfx) override;
    bool handleTouch(int x, int y, TouchEvent ev) override;
    const char* name() const override { return "WiFi"; }
};