#include "game_map.h"

// Простая карта: кирпичные стены по периметру + несколько внутренних блоков.
// 1 = кирпич, 0 = пусто.
static const uint8_t LAYOUT[13][23] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

GameMap::GameMap() {
    reset();
}

void GameMap::reset() {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            tiles[r][c] = LAYOUT[r][c] ? Tile::BRICK : Tile::EMPTY;
        }
    }
}

bool GameMap::inBounds(int col, int row) const {
    return col >= 0 && col < COLS && row >= 0 && row < ROWS;
}

bool GameMap::isSolid(int x, int y) const {
    if (x < ORIGIN_X || y < ORIGIN_Y) return true;
    int col = (x - ORIGIN_X) / TILE_SIZE;
    int row = (y - ORIGIN_Y) / TILE_SIZE;
    if (!inBounds(col, row)) return true;
    return tiles[row][col] != Tile::EMPTY;
}

bool GameMap::isSolidRect(int x1, int y1, int x2, int y2) const {
    // Проверяем 4 угла
    if (isSolid(x1, y1)) return true;
    if (isSolid(x2, y1)) return true;
    if (isSolid(x1, y2)) return true;
    if (isSolid(x2, y2)) return true;
    return false;
}

bool GameMap::destroyAt(int x, int y) {
    if (x < ORIGIN_X || y < ORIGIN_Y) return false;
    int col = (x - ORIGIN_X) / TILE_SIZE;
    int row = (y - ORIGIN_Y) / TILE_SIZE;
    if (!inBounds(col, row)) return false;

    if (tiles[row][col] == Tile::BRICK) {
        tiles[row][col] = Tile::EMPTY;
        return true;
    }
    return false;
}

void GameMap::draw(Arduino_Canvas* gfx) const {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (tiles[r][c] == Tile::EMPTY) continue;

            int x = ORIGIN_X + c * TILE_SIZE;
            int y = ORIGIN_Y + r * TILE_SIZE;

            if (tiles[r][c] == Tile::BRICK) {
                gfx->fillRect(x, y, TILE_SIZE, TILE_SIZE, MAROON);
                gfx->drawRect(x, y, TILE_SIZE, TILE_SIZE, ORANGE);
                // «кладка» — две горизонтальные полосы
                gfx->drawFastHLine(x + 1, y + TILE_SIZE / 2,     TILE_SIZE - 2, ORANGE);
                gfx->drawFastVLine(x + TILE_SIZE / 2, y + 1,     TILE_SIZE / 2 - 1, ORANGE);
                gfx->drawFastVLine(x + TILE_SIZE / 2, y + TILE_SIZE / 2 + 1, TILE_SIZE / 2 - 1, ORANGE);
            }
            else if (tiles[r][c] == Tile::STEEL) {
                gfx->fillRect(x, y, TILE_SIZE, TILE_SIZE, DARKGREY);
                gfx->drawRect(x, y, TILE_SIZE, TILE_SIZE, WHITE);
            }
        }
    }
}