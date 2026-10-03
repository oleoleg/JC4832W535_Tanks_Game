//инициализация и вспомогательные функции

#include "display_touch.h"
#include "config.h"
#include <JC3248W535.h>

// Поворот: 0 = портрет 320x480, 1 = альбом 480x320, 3 = альбом перевёрнутый
JC3248W535_Display display;
JC3248W535_Touch touch;

Arduino_Canvas* gfx = nullptr;

int lastX = -1;
int lastY = -1;
uint32_t lastDraw = 0;
int prevX = -1, prevY = -1;


Arduino_Canvas* getCanvas() {
    return gfx;
}

bool initDisplayTouch() {
    if (!display.begin()) {
        Serial.println("Ошибка инициализации дисплея!");
        return false;
    }
    display.setRotation(1);  // альбомная
    gfx = display.getCanvas();  // Получаем канвас для рисования

    if (!touch.begin()) {
        Serial.println("Ошибка инициализации тачскрина!");
        return false;
    }
    return true;
}

bool readTouch(int &x, int &y) {
    TouchPoint point;
    if (!touch.read(point)) return false;

    int rawX = point.x;
    int rawY = point.y;

    // Маппинг в альбомные координаты
    x = rawY;
    y = 319 - rawX;

    // Ограничение границ
    if (x < 0) x = 0;
    if (x > SCREEN_W - 1) x = SCREEN_W - 1;
    if (y < 0) y = 0;
    if (y > SCREEN_H - 1) y = SCREEN_H - 1;

    lastX = x;
    lastY = y;

    return true;
}

void flushDisplay() {
    display.flush();
}


static bool wasTouched = false;

bool touchJustReleased() {
    int x, y;
    bool nowTouched = readTouch(x, y);
    bool released = (wasTouched && !nowTouched);
    wasTouched = nowTouched;
    return released;
}

