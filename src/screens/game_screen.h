#pragma once
#include "screen.h"
#include "../game/tank.h"
#include "../game/bullet.h"
#include "../game/game_map.h"
#include "../game/rocket.h"


enum TankType {
    T_BASIC = 0,
    T_FAST,
    T_HEAVY,
    T_SNIPER,
    T_BOSS,
    T_KATYUSHA,
    T_COUNT
};

struct TankTypeInfo {
    const char* name;
    uint16_t    color;
    int         hp;
    float       typeSpeed;
    int         points;
    uint32_t    fireDelay;
};

extern const TankTypeInfo TANK_TYPES[T_COUNT];


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
    static constexpr int MAX_ENEMIES   = 20;
 
    int nextSpawnIndex = 0;
    int spawnedCount = 0;   // сколько всего заспавнено с начала уровня

    int  score = 0;
    int  enemiesTotal = 15;
    int  remainingTypeCount[T_COUNT] = {15, 0, 0, 0, 0};
    

    struct Enemy {
        Tank*    tank    = nullptr;
        Bullet*  bullet  = nullptr;
        uint32_t nextAi  = 0;
        uint32_t nextFire = 0;
        bool     alive   = false;
        int      hp      = 1;
        int      maxHp   = 1;
        int      points  = 0;
        int      katyushaState = 0;    // 0=нет, 1=въезжает, 2=стреляет, 3=уезжает
        uint32_t katyushaTimer = 0;
        int      rocketsFired = 0;
        int      rocketsTotal = 4;
        int      katyushaTargetX = 0;
    };

    // --- Игровые объекты ---
    GameMap map;
    Tank    player;
    Enemy   enemies[MAX_ENEMIES];
    Bullet  playerBullet;

    // --- Состояние ---
    int  lives            = 3;
    int  enemiesRemaining = 10;
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
    int  pickNextType();
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

    Rocket  rockets[Rocket::MAX_ROCKETS];
    int     maxOnField = 3;      // из карты
    int     katyushaRemaining = 0;
    uint32_t nextKatyushaTime = 0;
    int katyushaMax = 0;   // сколько одновременно
    

    void    updateKatyusha(Enemy& e, uint32_t now);
    void    launchRocketBarrage(Enemy& e);
    void    updateRockets(uint32_t now);
    bool    spawnKatyusha(); 

};