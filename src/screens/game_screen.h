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
    static GameScreen* getInstance();
    static String selectedMapPath;   // выбранная карта ("" = дефолт)
private:
    static constexpr int MAX_ENEMIES   = 6;
    static constexpr int TOTAL_ENEMIES = 12;
    int nextSpawnIndex = 0;
    int spawnedCount = 0;   // сколько всего заспавнено с начала уровня
    
    struct Enemy {
        Tank*    tank   = nullptr;
        Bullet*  bullet = nullptr;
        uint32_t nextAi = 0;
        uint32_t nextFire = 0;
        bool     alive  = false;
    };

    // --- Игровые объекты ---
    GameMap map;
    Tank    player;
    Enemy   enemies[MAX_ENEMIES];
    Bullet  playerBullet;

    // --- Состояние ---
    int  lives            = 3;
    int  enemiesRemaining = TOTAL_ENEMIES;
    bool gameOver         = false;
    bool victory          = false;
    bool enemiesInit      = false;
    uint32_t lastSpawnTime = 0;

    // --- Стрельба игрока ---
    bool     fireRequested = false;
    uint32_t lastFireTime  = 0;
    static constexpr uint32_t FIRE_COOLDOWN_MS = 400;

    // --- Управление ---
    int moveAngle = -1;
    bool firePressed = false;
    bool autoFire = false;
    bool showMoveIndicator = false;
    int  moveIndicatorX = 0, moveIndicatorY = 0;
    bool showFireIndicator = false;
    int  fireIndicatorX = 0, fireIndicatorY = 0;

    static constexpr int DPAD_CX = 370, DPAD_CY = 200, DPAD_R = 90;
    static constexpr float SPEED_NEAR = 0.5f, SPEED_FAR = 1.5f;
    static constexpr int FIRE_CX = 70, FIRE_CY = 275, FIRE_R = 55;
    static constexpr int BACK_X = 10, BACK_Y = 10, BACK_W = 60, BACK_H = 34;
    // Кнопка AUTO — рядом с BACK, в HUD-полосе
    static constexpr int AUTO_X = 78;
    static constexpr int AUTO_Y = 10;
    static constexpr int AUTO_W = 60;
    static constexpr int AUTO_H = 34;

    // Вспомогательные
    void initEnemies();
    void resetLevel();
    void spawnEnemy(int slot);
    void trySpawnWave();
    void updateEnemyAI();
    void checkBulletHits();
    int  chooseSmartDirection(const Tank& t) const;
    bool tanksOverlap(const Tank& a, const Tank& b) const;
    bool isTankAt(float x, float y, int exceptSlot) const;

    void updateDirection(int x, int y);
    bool inCircle(int x, int y, int cx, int cy, int r) const;
    const char* angleToText() const;
    void drawHUD(Arduino_Canvas* gfx);
};