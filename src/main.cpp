#include <Arduino.h>
#include "config.h"
#include "display_touch.h"
#include "screens/screen_manager.h"
#include "screens/menu_screen.h"
#include "wifi_manager.h"
#include "sd_manager.h"
#include "screens/screen_globals.h"


ScreenManager screens;
MenuScreen* menuScreen = nullptr;
static bool lastTouched = false;
static int lastTx = -1;
static int lastTy = -1;

// Глобальная функция доступа к менеджеру (для заглушек экранов)
ScreenManager* getScreenManager() {
    return &screens;
}

void setup() {
    Serial.begin(115200);
    randomSeed(esp_random());   // энтропия от железа
    delay(500);

    if (!initDisplayTouch()) {
        Serial.println("Ошибка дисплея/тача");
        while (1) delay(1000);
    }

    Arduino_Canvas* gfx = getCanvas();
    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(40, 140);
    gfx->print("WiFi connecting...");
    flushDisplay();

    wifiConnect();
    sdInit();
    wifiStartServer();

    // Создаём меню и кладём в стек первым
    menuScreen = new MenuScreen(&screens);
    screens.push(menuScreen);

    Serial.println("=== READY ===");
}



void loop() {
    Arduino_Canvas* gfx = getCanvas();

    int tx = 0, ty = 0;
    bool touched = readTouch(tx, ty);

    TouchEvent ev = TouchEvent::NONE;
    if (touched && !lastTouched) {
        ev = TouchEvent::PRESSED;
    } else if (touched && lastTouched) {
        // Палец на экране, проверяем — двигался ли
        if (abs(tx - lastTx) > 2 || abs(ty - lastTy) > 2) {
            ev = TouchEvent::MOVED;
        }
    } else if (!touched && lastTouched) {
        ev = TouchEvent::RELEASED;
    }

    if (ev != TouchEvent::NONE) {
        screens.handleTouch(tx, ty, ev);
    }

    lastTouched = touched;
    lastTx = tx;
    lastTy = ty;
    
    screens.update();
    screens.draw(gfx);
    flushDisplay();
    wifiLoop();
    delay(10);
}