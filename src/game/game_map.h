#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

enum class Tile : uint8_t {
    EMPTY  = 0,
    BRICK  = 1,
    STEEL  = 2,
    ICE    = 3,   // лёд — не блокирует, танк скользит
    FOG    = 4    // туман — не блокирует, визуальный эффект
};

class GameMap {
public:
    static constexpr int TILE_SIZE = 20;
    static constexpr int COLS      = 23;
    static constexpr int ROWS      = 13;
    static constexpr int ORIGIN_X  = 10;
    static constexpr int ORIGIN_Y  = 55;

    static constexpr int MAX_SPAWNS = 12;

    GameMap();

    // Сбросить на встроенную карту
    void reset();

    // Загрузить встроенную дефолтную карту (из строк в коде)
    bool loadDefault();

    // Загрузить карту из файла на SD
    bool loadFromFile(const char* path);

    bool isSolid(int x, int y) const;
    bool isSolidRect(int x1, int y1, int x2, int y2) const;
    bool destroyAt(int x, int y);

    int getMaxEnemies() const { return maxOnField; }
    int getKatyushaCount() const { return katyushaCount; }


    // Тип тайла в точке (для проверки льда/тумана)
    Tile tileAt(int x, int y) const;

    void draw(Arduino_Canvas* gfx) const;

    bool hasPlayerStart() const { return playerStartSet; }
    int  getPlayerStartX() const { return playerStartX; }
    int  getPlayerStartY() const { return playerStartY; }

    bool hasBase() const { return baseSet; }
    bool isBaseDestroyed() const { return baseDestroyed; }
    int  getBaseX() const { return baseX; }
    int  getBaseY() const { return baseY; }

    int  getSpawnCount() const { return spawnCount; }
    void getSpawnPoint(int i, int& outX, int& outY) const;


    static constexpr int MAX_TYPES = 6;
    void getEnemyTypeCounts(int out[MAX_TYPES]) const;


private:
    // Промежуточная структура — заполняется парсером, потом применяется
    struct ParsedMap {
        Tile tiles[ROWS][COLS];
        bool playerStartSet;
        int  playerStartX, playerStartY;
        bool baseSet;
        int  baseCol, baseRow;
        int  spawnX[MAX_SPAWNS];
        int  spawnY[MAX_SPAWNS];
        int  spawnCount;
        int  typeCounts[MAX_TYPES];
        bool configFound;
        int  maxEnemies;
    };

    Tile tiles[ROWS][COLS];

    bool playerStartSet = false;
    int  playerStartX = 240, playerStartY = 185;

    bool baseSet = false;
    bool baseDestroyed = false;
    int  baseCol = -1, baseRow = -1;
    int  baseX = 0, baseY = 0;

    int spawnX[MAX_SPAWNS];
    int spawnY[MAX_SPAWNS];
    int spawnCount = 0;

    int maxOnField = 3;
    int katyushaCount = 0;

    bool inBounds(int col, int row) const;
    void clearSpecialPoints();
    void initParsed(ParsedMap& pm) const;
    bool applyParsed(const ParsedMap& pm);

    // Парсит одну строку карты. Возвращает true, если строка похожа на строку карты
    // (>= 10 токенов) и была обработана.
    bool parseLine(const String& line, int row, ParsedMap& out) const;

    void parseConfig(const String& line, ParsedMap& out) const;
    int typeCounts[MAX_TYPES] = {15, 0, 0, 0, 0};
};