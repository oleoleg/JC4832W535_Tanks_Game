#include "menu_screen.h"
#include "screen_manager.h"
#include "select_map_screen.h"

// Экраны — заглушки, которые открывает меню
#include "game_screen.h"
#include "wifi_screen.h"
#include "test_screen.h"

// Глобальные экземпляры (создаются при первом входе)
static Screen* wifiScreen = nullptr;
static Screen* testScreen = nullptr;

// Храним указатель на менеджер, чтобы обработчики могли пушить экраны
static ScreenManager* gManager = nullptr;

MenuScreen::MenuScreen(ScreenManager* manager) : screens(manager) {
    gManager = manager;
}

void MenuScreen::draw(Arduino_Canvas* gfx) {
    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(3);
    gfx->setCursor(120, 20);
    gfx->print("BATTLE CITY");

    // Регистрируем кнопки при первом вызове
    static bool initialized = false;
    if (!initialized) {
        ui.clearButtons();
        ui.addButton( 40,  70, 180, 70, "GAME",     DARKGREEN, onGame);
        ui.addButton(260,  70, 180, 70, "DOORBELL", NAVY,      onDoorbell);
        ui.addButton( 40, 170, 180, 70, "WIFI SD",  DARKCYAN,  onWifi);
        ui.addButton(260, 170, 180, 70, "TEST",     DARKGREY,  onTest);
        initialized = true;
    }

    ui.draw(gfx);
}

bool MenuScreen::handleTouch(int x, int y, TouchEvent ev) {
    return ui.handleTouch(x, y, ev);
}

// --- Обработчики кнопок ---

void MenuScreen::onGame(ButtonEvent ev) {
    if (ev != ButtonEvent::RELEASED) return;
    static SelectMapScreen* selScreen = nullptr;
    if (!selScreen) selScreen = new SelectMapScreen();
    gManager->push(selScreen);
}

void MenuScreen::onDoorbell(ButtonEvent ev) {
    if (ev != ButtonEvent::RELEASED) return;
    Serial.println("Doorbell: ещё не реализовано");
}

void MenuScreen::onWifi(ButtonEvent ev) {
    if (ev != ButtonEvent::RELEASED) return;
    if (!wifiScreen) wifiScreen = new WiFiScreen();
    gManager->push(wifiScreen);
}

void MenuScreen::onTest(ButtonEvent ev) {
    if (ev != ButtonEvent::RELEASED) return;
    if (!testScreen) testScreen = new TestScreen();
    gManager->push(testScreen);
}