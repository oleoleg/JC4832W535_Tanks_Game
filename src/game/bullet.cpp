#include "bullet.h"
#include <math.h>
#include "game_map.h"


Bullet::Bullet() : x(0), y(0), dx(0), dy(0), active(false) {}

void Bullet::fire(float px, float py, float angleDeg) {
    x = px;
    y = py;
    float rad = angleDeg * PI / 180.0f;
    dx = cosf(rad) * SPEED;
    dy = sinf(rad) * SPEED;
    active = true;
}

void Bullet::update(GameMap* map) {
    if (!active) return;

    float newX = x + dx;
    float newY = y + dy;

    // Проверка попадания в стену
    if (map && map->isSolid((int)newX, (int)newY)) {
        // Разрушить кирпич (если он там был) и деактивировать снаряд
        map->destroyAt((int)newX, (int)newY);
        active = false;
        return;
    }

    x = newX;
    y = newY;

    // Вылет за границы экрана (на случай, если карты нет)
    if (x < 0 || x > 479 || y < 0 || y > 319) {
        active = false;
    }
}

void Bullet::draw(Arduino_Canvas* gfx) const {
    if (!active) return;
    gfx->fillCircle((int)x, (int)y, RADIUS, YELLOW);
}