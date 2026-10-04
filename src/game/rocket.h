#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

class GameMap;

class Rocket {
public:
    static constexpr int   MAX_ROCKETS    = 5;      // одновременно в залпе
    static constexpr float DAMAGE_RADIUS  = 30.0f;  // радиус взрыва
    static constexpr uint32_t MARK_TIME   = 1800;   // мс до падения
    static constexpr uint32_t BOOM_TIME   = 400;    // мс анимации взрыва

    Rocket();

    void launch(int tx, int ty, uint32_t now);
    void update(uint32_t now, GameMap* map);
    void draw(Arduino_Canvas* gfx) const;

    bool isActive()   const { return state != 0; }
    bool isExploding() const { return state == 2; }
    bool isWaiting()  const { return state == 1; }

    int getTargetX() const { return targetX; }
    int getTargetY() const { return targetY; }

    // Проверка попадания в точку (для урона игроку)
    bool inBlast(float x, float y) const;

    void deactivate() { state = 0; }

private:
    int  targetX, targetY;
    uint32_t launchTime;
    int  state;   // 0=idle, 1=marked, 2=exploding
};