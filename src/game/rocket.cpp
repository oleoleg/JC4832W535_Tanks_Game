#include "rocket.h"
#include "game_map.h"

Rocket::Rocket() : targetX(0), targetY(0), launchTime(0), state(0) {}

void Rocket::launch(int tx, int ty, uint32_t now) {
    targetX = tx;
    targetY = ty;
    launchTime = now;
    state = 1;   // помечено
}

void Rocket::update(uint32_t now, GameMap* map) {
    if (state == 0) return;

    uint32_t elapsed = now - launchTime;

    if (state == 1 && elapsed >= MARK_TIME) {
        state = 2;   // взрыв
        // Не разрушаем карту — залп бьёт по площади, а не по кирпичам
    }
    else if (state == 2 && elapsed >= MARK_TIME + BOOM_TIME) {
        state = 0;   // закончился
    }
}

void Rocket::draw(Arduino_Canvas* gfx) const {
    if (state == 1) {
        // Мигающий маркер цели
        uint16_t color = ((millis() / 200) % 2) ? RED : YELLOW;
        gfx->drawCircle(targetX, targetY, (int)DAMAGE_RADIUS, color);
        gfx->drawCircle(targetX, targetY, (int)DAMAGE_RADIUS - 2, color);
        gfx->drawLine(targetX - 4, targetY, targetX + 4, targetY, color);
        gfx->drawLine(targetX, targetY - 4, targetX, targetY + 4, color);
    }
    else if (state == 2) {
        // Взрыв — заливка оранжевая/жёлтая
        gfx->fillCircle(targetX, targetY, (int)DAMAGE_RADIUS, ORANGE);
        gfx->fillCircle(targetX, targetY, (int)DAMAGE_RADIUS - 6, YELLOW);
        gfx->fillCircle(targetX, targetY, 6, WHITE);
    }
}

bool Rocket::inBlast(float x, float y) const {
    if (state != 2) return false;
    float dx = x - targetX;
    float dy = y - targetY;
    return (dx*dx + dy*dy) < (DAMAGE_RADIUS * DAMAGE_RADIUS);
}