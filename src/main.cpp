#include <Arduino.h>
#include "config.h"
#include "display_touch.h"
#include "ui/ui_manager.h"
#include "menu.h"
#include "wifi_manager.h"
#include "sd_manager.h"

UIManager ui;
 
void setup() {
    Serial.begin(115200);
    delay(500);

    // 1. Дисплей и тач ПЕРВЫМИ (буфер в PSRAM выделяется до Wi-Fi)
    if (!initDisplayTouch()) {
        Serial.println("Ошибка дисплея/тача");
        while (1) delay(1000);
    }

    // 2. Показываем на экране сообщение о подключении
    Arduino_Canvas* gfx = getCanvas();
    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(40, 140);
    gfx->print("WiFi connecting...");
    gfx->setCursor(40, 170);
    gfx->print("Connect to Tanks-Setup");
    flushDisplay();

    // 3. Теперь Wi-Fi (он не повредит уже созданный буфер)
    wifiConnect();

    // 4. SD
    sdInit();

    // 5. Веб-сервер
    wifiStartServer();

    // 6. Меню
    menuSetup(ui);

    Serial.println("=== READY ===");
}

void loop() {

    Arduino_Canvas* gfx = getCanvas();
    // Рисуем фон меню
    menuDraw(gfx);

    // UIManager сам нарисует кнопки поверх и обработает тач
    ui.loop(gfx, readTouch, flushDisplay);


    // Wi-Fi
    wifiLoop();

    delay(10);
}