#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

class GameMap;

class Tank {
public:
    Tank(int startX, int startY, uint16_t color);

    void setPosition(int px, int py);
    void setTargetAngle(float angleDeg);
    void setMoving(bool m) { moving = m; }
    void stop();

    void setSpeedFactor(float f)   { speedFactor = f; }
    float getSpeedFactor() const   { return speedFactor; }

    void setTypeSpeed(float s)     { typeSpeed = s; }
    void setRotationSpeed(float s) { rotationSpeed = s; }
    void setColor(uint16_t c)      { color = c; }

    void update(const GameMap* map);
    void draw(Arduino_Canvas* gfx) const;
    void drawKatyusha(Arduino_Canvas* gfx) const; 

    bool  wasBlocked()  const { return blocked; }
    float getX()        const { return x; }
    float getY()        const { return y; }
    float getAngle()    const { return currentAngle; }

    void reset(int startAngle = 90) {
        currentAngle = startAngle;
        targetAngle  = startAngle;
        blocked      = false;
        slideSpeed   = 0;
        speedFactor  = 1.0f;
    }

private:
    float x, y;
    float currentAngle;
    float targetAngle;
    bool  moving;
    uint16_t color;

    bool  blocked       = false;
    float rotationSpeed = 6.0f;
    float speedFactor   = 1.0f;
    float typeSpeed     = 1.0f;
    float slideSpeed    = 0.0f;   // инерция на льду

    static constexpr float SPEED          = 2.0f;
    static constexpr int   WIDTH          = 16;
    static constexpr int   LENGTH         = 20;
    static constexpr int   COLLISION_SIZE = 18;

    static constexpr int FIELD_LEFT   = 5;
    static constexpr int FIELD_RIGHT  = 475;
    static constexpr int FIELD_TOP    = 55;
    static constexpr int FIELD_BOTTOM = 315;
};