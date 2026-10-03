#include "test_screen.h"
#include "screen_globals.h"
#include "screen_manager.h"


void TestScreen::draw(Arduino_Canvas* gfx) {
    gfx->fillScreen(BLACK);
    gfx->setTextColor(LIGHTGREY);
    gfx->setTextSize(3);
    gfx->setCursor(140, 140);
    gfx->print("test");
    gfx->setTextSize(1);
    gfx->setTextColor(WHITE);
    gfx->setCursor(140, 180);
    gfx->print("Tap to return");
}


bool TestScreen::handleTouch(int x, int y, TouchEvent ev) {
    if (ev == TouchEvent::PRESSED) {
        getScreenManager()->pop();
        return true;
    }
    return false;
}