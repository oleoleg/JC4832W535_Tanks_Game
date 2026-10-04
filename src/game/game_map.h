#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

enum class Tile : uint8_t {
    EMPTY = 0,
    BRICK = 1,
    STEEL = 2      // пока не используется, но пусть будет
};

class GameMap {
public:
    static constexpr int TILE_SIZE = 20;
    static constexpr int COLS      = 23;
    static constexpr int ROWS      = 13;
    static constexpr int ORIGIN_X  = 10;
    static constexpr int ORIGIN_Y  = 55;

    GameMap();

    void reset();

    // Проверка точки (true, если стена или вне поля)
    bool isSolid(int x, int y) const;

    // Проверка прямоугольника (для bounding box танка)
    bool isSolidRect(int x1, int y1, int x2, int y2) const;

    // Разрушить кирпич в точке. true, если что-то было разрушено.
    bool destroyAt(int x, int y);

    // Отрисовка всей карты
    void draw(Arduino_Canvas* gfx) const;

private:
    Tile tiles[ROWS][COLS];
    bool inBounds(int col, int row) const;
};