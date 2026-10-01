#include <Arduino.h>
#include <JC3248W535.h>
#define SCREEN_W 480
#define SCREEN_H 320

// Поворот: 0 = портрет 320x480, 1 = альбом 480x320, 3 = альбом перевёрнутый
JC3248W535_Display display;
JC3248W535_Touch touch;

Arduino_Canvas *gfx = nullptr;

int lastX = -1;
int lastY = -1;

void drawTestScreen() {
    int w = display.width();
    int h = display.height();

    gfx->fillScreen(BLACK);
    gfx->drawRect(0, 0, w, h, WHITE);

    // Четыре угла
    gfx->fillRect(0, 0, 40, 40, RED);                    // левый верх
    gfx->fillRect(w - 40, 0, 40, 40, GREEN);             // правый верх
    gfx->fillRect(0, h - 40, 40, 40, BLUE);              // левый низ
    gfx->fillRect(w - 40, h - 40, 40, 40, YELLOW);       // правый низ

    // Центр
    gfx->fillRect(w/2 - 30, h/2 - 30, 60, 60, MAGENTA);

    // Точка последнего касания
    if (lastX >= 0 && lastY >= 0) {
        gfx->fillCircle(lastX, lastY, 6, WHITE);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    // 2. Инициализируем дисплей
    if (!display.begin()) {
        Serial.println("Ошибка инициализации дисплея!");
        while (1) delay(1000);
    }

    // 3. Устанавливаем ПОВОРОТ (альбомная ориентация)
    // Попробуй 1 или 3
    display.setRotation(1); 
    
    // 4. Получаем канвас для рисования
    gfx = display.getCanvas();

    // 5. Инициализируем тач
    touch.begin();

    Serial.println("=== ГОТОВО ===");
}


void loop() {
TouchPoint point;
if (touch.read(point)) {
    int rawX = point.x;
    int rawY = point.y;

    // Переставляем оси и зеркалим X
    int x = rawY;
    int y = 319 - rawX;

    // Ограничение границ
    if (x < 0) x = 0;
    if (x > SCREEN_W - 1) x = SCREEN_W - 1;
    if (y < 0) y = 0;
    if (y > SCREEN_H - 1) y = SCREEN_H - 1;

    lastX = x;
    lastY = y;

    Serial.print("Raw(");
    Serial.print(rawX);
    Serial.print(",");
    Serial.print(rawY);
    Serial.print(") -> Screen(");
    Serial.print(x);
    Serial.print(",");
    Serial.print(y);
    Serial.println(")");
    }

    drawTestScreen();
    display.flush();   // обязательно, иначе экран не обновится

    delay(16);
}