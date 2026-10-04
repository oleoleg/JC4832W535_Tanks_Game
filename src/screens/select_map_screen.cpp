#include "select_map_screen.h"
#include "screen_globals.h"
#include "screen_manager.h"
#include "game_screen.h"
#include <SD.h>

void SelectMapScreen::scanMaps() {
    mapCount = 0;
    File dir = SD.open("/tanks");
    if (!dir || !dir.isDirectory()) {
        Serial.println("Папка /tanks/ не найдена");
        return;
    }

    File f = dir.openNextFile();
    while (f && mapCount < MAX_MAPS) {
        if (!f.isDirectory()) {
            String name = String(f.name());
            if (name.endsWith(".map") || name.endsWith(".MAP")) {
                mapFiles[mapCount++] = name;
            }
        }
        f = dir.openNextFile();
    }
    dir.close();
    Serial.printf("Найдено карт: %d\n", mapCount);
}

void SelectMapScreen::onEnter() {
    scanMaps();
}

void SelectMapScreen::draw(Arduino_Canvas* gfx) {
    gfx->fillScreen(BLACK);

    // === BACK ===
    gfx->fillRect(10, 10, 60, 34, DARKGREY);
    gfx->drawRect(10, 10, 60, 34, WHITE);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(24, 23);
    gfx->print("BACK");

    // === Заголовок ===
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(120, 20);
    gfx->print("SELECT MAP");

    // === Список карт ===
    if (mapCount == 0) {
        gfx->setTextColor(DARKGREY);
        gfx->setTextSize(1);
        gfx->setCursor(140, 90);
        gfx->print("No maps on SD");
    } else {
        for (int i = 0; i < mapCount; i++) {
            int y = BTN_Y0 + i * (BTN_H + BTN_GAP);
            gfx->fillRect(BTN_X, y, BTN_W, BTN_H, NAVY);
            gfx->drawRect(BTN_X, y, BTN_W, BTN_H, WHITE);

            gfx->setTextColor(WHITE);
            gfx->setTextSize(1);
            gfx->setCursor(BTN_X + 10, y + 11);
            gfx->print(mapFiles[i]);
        }
    }

    // === Кнопка DEFAULT — всегда внизу ===
    int dy = DEF_Y;
    gfx->fillRect(BTN_X, dy, BTN_W, BTN_H, DARKGREEN);
    gfx->drawRect(BTN_X, dy, BTN_W, BTN_H, WHITE);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(BTN_X + 130, dy + 11);
    gfx->print("USE DEFAULT MAP");
}


bool SelectMapScreen::handleTouch(int x, int y, TouchEvent ev) {
    if (ev != TouchEvent::PRESSED) return false;

    // Выход (BACK)
    if (x >= 10 && x <= 70 && y >= 10 && y <= 44) {
        getScreenManager()->pop();
        return true;
    }

    // === Кнопка DEFAULT — всегда работает ===
    if (x >= BTN_X && x <= BTN_X + BTN_W &&
        y >= DEF_Y && y <= DEF_Y + BTN_H) {
        GameScreen::selectedMapPath = "";   // пустая = дефолт
        Serial.println("Выбрана дефолтная карта");
        getScreenManager()->push(GameScreen::getInstance());
        return true;
    }

    // === Выбор карты из списка ===
    for (int i = 0; i < mapCount; i++) {
        int by = BTN_Y0 + i * (BTN_H + BTN_GAP);
        if (x >= BTN_X && x <= BTN_X + BTN_W &&
            y >= by && y <= by + BTN_H) {

            GameScreen::selectedMapPath = "/tanks/" + mapFiles[i];
            Serial.printf("Выбрана карта: %s\n",
                          GameScreen::selectedMapPath.c_str());

            getScreenManager()->push(GameScreen::getInstance());
            return true;
        }
    }
    return false;
}
