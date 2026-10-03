#pragma once
#include "screen.h"

class GameScreen : public Screen {
public:
    void onEnter() override;
    void draw(Arduino_Canvas* gfx) override;
    bool handleTouch(int x, int y, TouchEvent ev) override;
    const char* name() const override { return "Game"; }

private:
    // Угол направления в градусах (0=вправо, 90=вниз, 180=влево, 270=вверх)
    // -1 = стоп
    int moveAngle = -1;
    bool firePressed = false;

    bool showMoveIndicator = false;
    int  moveIndicatorX = 0;
    int  moveIndicatorY = 0;
    bool showFireIndicator = false;
    int  fireIndicatorX = 0;
    int  fireIndicatorY = 0;

    static constexpr int DPAD_CX = 400;
    static constexpr int DPAD_CY = 200;
    static constexpr int DPAD_R  = 90;

    static constexpr int FIRE_CX = 90;
    static constexpr int FIRE_CY = 220;
    static constexpr int FIRE_R  = 55;

    static constexpr int BACK_X = 10;
    static constexpr int BACK_Y = 10;
    static constexpr int BACK_W = 60;
    static constexpr int BACK_H = 40;

    void updateDirection(int x, int y);
    bool inCircle(int x, int y, int cx, int cy, int r) const;
    const char* angleToText() const;
};