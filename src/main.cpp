#include <Arduino.h>
#include "config.h"
#include "display_touch.h"
#include "ui/ui_manager.h"
#include "menu.h"

UIManager ui;
 
void setup() {
    Serial.begin(115200);
    delay(500);

    if (!initDisplayTouch()) {
        Serial.println("Ошибка инициализации дисплея/тача");
        while (1) delay(1000);
    }
    
    // Регистрируем кнопки меню
    menuSetup(ui);

    Serial.println("=== READY ===");
}

void loop() {
    
    Arduino_Canvas* gfx = getCanvas();
    // Рисуем фон меню
    menuDraw(gfx);

    // UIManager сам нарисует кнопки поверх и обработает тач
    ui.loop(gfx, readTouch, flushDisplay);

    delay(10);
}