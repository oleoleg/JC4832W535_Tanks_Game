#include "menu.h"
#include "config.h"
#include "display_touch.h"

// ================== ОБРАБОТЧИКИ КНОПОК ==================

void onGameClick(ButtonEvent ev) {
    if (ev == ButtonEvent::RELEASED) {
        Serial.println("GAME clicked");
        // TODO: переход на экран игры
    }
}

void onDoorbellClick(ButtonEvent ev) {
    if (ev == ButtonEvent::RELEASED) {
        Serial.println("DOORBELL clicked");
        // TODO: переход на экран домофона
    }
}

void onWifiClick(ButtonEvent ev) {
    if (ev == ButtonEvent::RELEASED) {
        Serial.println("WIFI clicked");
        // TODO: переход на экран Wi-Fi
    }
}

void onTestClick(ButtonEvent ev) {
    if (ev == ButtonEvent::RELEASED) {
        Serial.println("TEST clicked");
        // TODO: переход на тестовый экран
    }
}

// ================== РЕГИСТРАЦИЯ КНОПОК ==================

void menuSetup(UIManager &ui) {
    ui.clearButtons();

    // Заголовок занимает верх экрана, кнопки — ниже
    ui.addButton( 40,  70, 180, 70, "GAME",     DARKGREEN, onGameClick);
    ui.addButton(260,  70, 180, 70, "DOORBELL", NAVY,      onDoorbellClick);
    ui.addButton( 40, 170, 180, 70, "WIFI SD",  DARKCYAN,  onWifiClick);
    ui.addButton(260, 170, 180, 70, "TEST",     DARKGREY,  onTestClick);
}

// ================== ОТРИСОВКА ФОНА ==================

void menuDraw(Arduino_Canvas *gfx) {
    gfx->fillScreen(BLACK);

    // Заголовок
    gfx->setTextColor(WHITE);
    gfx->setTextSize(3);
    gfx->setCursor(120, 20);
    gfx->print("BATTLE CITY");

    // Подзаголовок
    gfx->setTextColor(CYAN);
    gfx->setTextSize(1);
    gfx->setCursor(170, 55);
    gfx->print("ESP32-S3 Edition");
}