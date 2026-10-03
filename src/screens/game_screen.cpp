#include "game_screen.h"
#include "screen_globals.h"
#include "screen_manager.h"
#include "config.h"
#include <math.h>

void GameScreen::onEnter() {
    moveAngle = -1;
    firePressed = false;
    showMoveIndicator = false;
    showFireIndicator = false;
}

void GameScreen::draw(Arduino_Canvas* gfx) {
    gfx->fillScreen(BLACK);

    // Заголовок
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(180, 10);
    gfx->print("BATTLE CITY");

    // Кнопка «Назад»
    gfx->fillRect(BACK_X, BACK_Y, BACK_W, BACK_H, DARKGREY);
    gfx->drawRect(BACK_X, BACK_Y, BACK_W, BACK_H, WHITE);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(BACK_X + 12, BACK_Y + 15);
    gfx->print("BACK");

    // ——— Зона направления ———
    if (showMoveIndicator) {
        gfx->drawCircle(DPAD_CX, DPAD_CY, DPAD_R, DARKGREY);
        gfx->drawCircle(DPAD_CX, DPAD_CY, 20, DARKGREY);

        if (moveAngle >= 0) {
            float rad = moveAngle * PI / 180.0;
            int lx = DPAD_CX + (int)(cos(rad) * DPAD_R * 0.9);
            int ly = DPAD_CY + (int)(sin(rad) * DPAD_R * 0.9);
            gfx->drawLine(DPAD_CX, DPAD_CY, lx, ly, GREEN);
            gfx->fillCircle(lx, ly, 6, GREEN);
        }

        gfx->fillCircle(moveIndicatorX, moveIndicatorY, 4, WHITE);
    }

    // ——— Зона выстрела ———
    if (showFireIndicator) {
        gfx->drawCircle(FIRE_CX, FIRE_CY, FIRE_R, DARKGREY);
        gfx->fillCircle(fireIndicatorX, fireIndicatorY, 8, RED);
    }

    // Отладочная информация
    gfx->setTextColor(YELLOW);
    gfx->setTextSize(2);
    gfx->setCursor(20, 60);
    gfx->print("DIR: ");
    gfx->print(angleToText());

    if (moveAngle >= 0) {
        gfx->setCursor(20, 90);
        gfx->print("ANGLE: ");
        gfx->print(moveAngle);
    }

    if (firePressed) {
        gfx->setTextColor(RED);
        gfx->setCursor(20, 120);
        gfx->print("FIRE!");
    }
}

bool GameScreen::handleTouch(int x, int y, TouchEvent ev) {
    // Кнопка «Назад»
    if (ev == TouchEvent::PRESSED) {
        if (x >= BACK_X && x <= BACK_X + BACK_W &&
            y >= BACK_Y && y <= BACK_Y + BACK_H) {
            getScreenManager()->pop();
            return true;
        }
    }

    // ——— Зона направления ———
    if (ev == TouchEvent::PRESSED && inCircle(x, y, DPAD_CX, DPAD_CY, DPAD_R)) {
        showMoveIndicator = true;
        moveIndicatorX = x;
        moveIndicatorY = y;
        updateDirection(x, y);
        return true;
    }

    if (ev == TouchEvent::MOVED && showMoveIndicator) {
        moveIndicatorX = x;
        moveIndicatorY = y;
        updateDirection(x, y);
        return true;
    }

    if (ev == TouchEvent::RELEASED && showMoveIndicator) {
        showMoveIndicator = false;
        moveAngle = -1;
        return true;
    }

    // ——— Зона выстрела ———
    if (ev == TouchEvent::PRESSED && inCircle(x, y, FIRE_CX, FIRE_CY, FIRE_R)) {
        firePressed = true;
        showFireIndicator = true;
        fireIndicatorX = x;
        fireIndicatorY = y;
        Serial.println("FIRE!");
        return true;
    }

    if (ev == TouchEvent::RELEASED && showFireIndicator) {
        firePressed = false;
        showFireIndicator = false;
        return true;
    }

    return false;
}

// ——— Вспомогательные ———

bool GameScreen::inCircle(int x, int y, int cx, int cy, int r) const {
    int dx = x - cx;
    int dy = y - cy;
    return (dx * dx + dy * dy) <= (r * r);
}

void GameScreen::updateDirection(int x, int y) {
    int dx = x - DPAD_CX;
    int dy = y - DPAD_CY;

    // Мёртвая зона в центре
    if (abs(dx) < 15 && abs(dy) < 15) {
        moveAngle = -1;
        return;
    }

    float rad = atan2((float)dy, (float)dx);
    int deg = (int)(rad * 180.0 / PI);
    if (deg < 0) deg += 360;

    moveAngle = deg;
}

const char* GameScreen::angleToText() const {
    if (moveAngle < 0) return "STOP";

    if (moveAngle < 22 || moveAngle >= 338) return "RIGHT";
    if (moveAngle < 67)  return "DOWN-RIGHT";
    if (moveAngle < 112) return "DOWN";
    if (moveAngle < 157) return "DOWN-LEFT";
    if (moveAngle < 202) return "LEFT";
    if (moveAngle < 247) return "UP-LEFT";
    if (moveAngle < 292) return "UP";
    return "UP-RIGHT";
}