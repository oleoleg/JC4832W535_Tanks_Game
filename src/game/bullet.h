#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
class GameMap;

class Bullet {
public:
    Bullet();

    // Запустить снаряд из точки (x, y) в направлении angle (градусы)
    void fire(float x, float y, float angle);

    // Обновление и отрисовка
    void update(GameMap* map);
    void draw(Arduino_Canvas* gfx) const;

    bool isActive() const { return active; }
    void deactivate()     { active = false; }

    float getX() const { return x; }
    float getY() const { return y; }

private:
    float x, y;
    float dx, dy;       // вектор скорости
    bool  active;

    static constexpr float SPEED = 8.0f;   // пикс/кадр — быстрее танка
    static constexpr int   RADIUS = 3;

    // Границы игрового поля (совпадают с танком)
    static constexpr int FIELD_LEFT   = 5;
    static constexpr int FIELD_RIGHT  = 475;
    static constexpr int FIELD_TOP    = 55;
    static constexpr int FIELD_BOTTOM = 315;
};