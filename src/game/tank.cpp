#include "tank.h"
#include <math.h>
#include "game_map.h"


Tank::Tank(int startX, int startY, uint16_t col)
    : x(startX), y(startY),
      currentAngle(0), targetAngle(0),
      moving(false), color(col) {}

void Tank::setPosition(int px, int py) {
    x = px;
    y = py;
}

void Tank::setTargetAngle(float angleDeg) {
    targetAngle = angleDeg;
}

void Tank::stop() {
    moving = false;
    targetAngle = currentAngle;
}

void Tank::update(const GameMap* map) {

    blocked = false;
    // === Плавный поворот к targetAngle ===
    float diff = targetAngle - currentAngle;
    while (diff >  180) diff -= 360;
    while (diff < -180) diff += 360;

    if (fabs(diff) < rotationSpeed) {
        currentAngle = targetAngle;
    } else if (diff > 0) {
        currentAngle += rotationSpeed;
    } else {
        currentAngle -= rotationSpeed;
    }
    if (currentAngle >= 360) currentAngle -= 360;
    if (currentAngle < 0)    currentAngle += 360;

    // === Движение вперёд с проверкой коллизии ===
    if (moving && map) {
        blocked = false;
        float rad = currentAngle * PI / 180.0f;
        float step = SPEED * speedFactor;
        float newX = x + cosf(rad) * step;
        float newY = y + sinf(rad) * step;

        float hw = COLLISION_SIZE / 2.0f;

        // Попробовать движение целиком
        if (!map->isSolidRect((int)(newX - hw), (int)(newY - hw),
                              (int)(newX + hw), (int)(newY + hw))) {
            x = newX;
            y = newY;
        }
        // Иначе — попробовать только по X, потом только по Y
        else if (!map->isSolidRect((int)(newX - hw), (int)(y - hw),
                                   (int)(newX + hw), (int)(y + hw))) {
            x = newX;
            blocked = true;
        }
        else if (!map->isSolidRect((int)(x - hw), (int)(newY - hw),
                                   (int)(x + hw), (int)(newY + hw))) {
            y = newY;
            blocked = true;
        }
        // Иначе — упёрся, стоим
    }
}

void Tank::draw(Arduino_Canvas* gfx) const {
    float rad = currentAngle * PI / 180.0f;
    float c = cosf(rad);
    float s = sinf(rad);

    float hl = LENGTH / 2.0f;   // вдоль ствола
    float hw = WIDTH  / 2.0f;   // поперёк ствола

    // 4 угла корпуса в локальных координатах:
    // X — вдоль ствола (LENGTH), Y — поперёк (WIDTH)
    float lx[4] = { -hl,  hl,  hl, -hl };
    float ly[4] = { -hw, -hw,  hw,  hw };

    int px[4], py[4];
    for (int i = 0; i < 4; i++) {
        px[i] = (int)x + (int)(lx[i] * c - ly[i] * s);
        py[i] = (int)y + (int)(lx[i] * s + ly[i] * c);
    }

    // Заливка корпуса — два треугольника
    gfx->fillTriangle(px[0], py[0], px[1], py[1], px[2], py[2], color);
    gfx->fillTriangle(px[0], py[0], px[2], py[2], px[3], py[3], color);

    // Контур
    gfx->drawLine(px[0], py[0], px[1], py[1], WHITE);
    gfx->drawLine(px[1], py[1], px[2], py[2], WHITE);
    gfx->drawLine(px[2], py[2], px[3], py[3], WHITE);
    gfx->drawLine(px[3], py[3], px[0], py[0], WHITE);

    // Ствол — линия от центра вперёд
    int bx = (int)x + (int)(c * (hl + 6));
    int by = (int)y + (int)(s * (hl + 6));
    gfx->drawLine((int)x,     (int)y,     bx,     by, WHITE);
    gfx->drawLine((int)x + 1, (int)y,     bx + 1, by, WHITE);
}