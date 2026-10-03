#include "game_screen.h"
#include "screen_globals.h"
#include "screen_manager.h"


void GameScreen::draw(Arduino_Canvas* gfx) {
    gfx->fillScreen(BLACK);
    gfx->setTextColor(GREEN);
    gfx->setTextSize(3);
    gfx->setCursor(140, 140);
    gfx->print("GAME");
    gfx->setTextSize(1);
    gfx->setTextColor(WHITE);
    gfx->setCursor(140, 180);
    gfx->print("Tap to return");
}

bool GameScreen::handleTouch(int x, int y, TouchEvent ev) {
    if (ev == TouchEvent::PRESSED) {
        getScreenManager()->pop();
        return true;
    }
    return false;
}