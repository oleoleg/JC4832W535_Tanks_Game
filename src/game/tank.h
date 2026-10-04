#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
class GameMap;

class Tank {
public:
    Tank(int startX, int startY, uint16_t color);
    float speedFactor = 1.0f;   // множитель скорости (0.6..1.0)
    void setSpeedFactor(float f) { speedFactor = f; }
    float getSpeedFactor() const { return speedFactor; }
    void setPosition(int px, int py);
    void setRotationSpeed(float s) { rotationSpeed = s; }
    
    // Управление
    void setTargetAngle(float angleDeg);   // куда хочется повернуть
    void setMoving(bool m) { moving = m; }
    void stop();                           // остановить и зафиксировать угол

    // Обновление и отрисовка
    void update(const GameMap* map);
    void draw(Arduino_Canvas* gfx) const;

    float getX() const { return x; }
    float getY() const { return y; }
    float getAngle() const { return currentAngle; }

    //для противника
    bool wasBlocked() const { return blocked; }
private:
    float x, y;
    float currentAngle;   // текущий угол корпуса (град)
    float targetAngle;    // целевой угол (град)
    bool  moving;
    uint16_t color;
    bool blocked = false;

    // Параметры (легко подкрутить)
    static constexpr float SPEED          = 2.0f;   // пикселей за кадр
     float rotationSpeed = 6.0f;   // градусов за кадр
    static constexpr int   WIDTH          = 16;   // поперёк ствола
    static constexpr int   LENGTH         = 20;   // вдоль ствола
    static constexpr int   COLLISION_SIZE = 18;   // квадрат коллизии

    // Границы игрового поля
    static constexpr int FIELD_LEFT   = 5;
    static constexpr int FIELD_RIGHT  = 475;
    static constexpr int FIELD_TOP    = 55;
    static constexpr int FIELD_BOTTOM = 315;
};