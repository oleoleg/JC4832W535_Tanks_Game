#pragma once
#include "screen.h"
#include "../game/tank.h"
#include "../game/bullet.h"
#include "../game/game_map.h"

class GameScreen : public Screen {
public:
    GameScreen();
    void onEnter() override;
    void update() override;
    void draw(Arduino_Canvas* gfx) override;
    bool handleTouch(int x, int y, TouchEvent ev) override;
    const char* name() const override { return "Game"; }

private:
    // --- Игровые объекты ---
    GameMap map;
    Tank    player;
    Tank    enemy;
    Bullet  playerBullet;
    Bullet  enemyBullet;

    // --- Состояние игры ---
    int  lives            = 3;
    int  enemiesRemaining = 1;
    bool gameOver         = false;
    bool victory          = false;

    // --- Стрельба игрока ---
    bool     fireRequested = false;
    uint32_t lastFireTime  = 0;
    static constexpr uint32_t FIRE_COOLDOWN_MS = 400;

    // --- ИИ врага ---
    uint32_t nextAiDecisionTime = 0;
    uint32_t nextEnemyFireTime  = 0;

    // --- Управление ---
    int moveAngle = -1;
    bool firePressed = false;

    bool showMoveIndicator = false;
    int  moveIndicatorX = 0;
    int  moveIndicatorY = 0;
    bool showFireIndicator = false;
    int  fireIndicatorX = 0;
    int  fireIndicatorY = 0;

    static constexpr int DPAD_CX = 370;
    static constexpr int DPAD_CY = 200;
    static constexpr int DPAD_R  = 90;
    static constexpr float SPEED_NEAR = 0.5f;
    static constexpr float SPEED_FAR  = 1.5f;

    static constexpr int FIRE_CX = 70;
    static constexpr int FIRE_CY = 275;
    static constexpr int FIRE_R  = 55;

    static constexpr int BACK_X = 10;
    static constexpr int BACK_Y = 10;
    static constexpr int BACK_W = 60;
    static constexpr int BACK_H = 34;

    // Вспомогательные
    void updateDirection(int x, int y);
    bool inCircle(int x, int y, int cx, int cy, int r) const;
    const char* angleToText() const;

    void updateEnemyAI();
    void checkBulletHits();
    void drawHUD(Arduino_Canvas* gfx);
    void resetLevel();
    bool tanksOverlap(const Tank& a, const Tank& b) const;
};