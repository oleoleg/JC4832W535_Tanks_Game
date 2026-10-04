#include "game_map.h"
#include <SD.h>

// ============================================================
// Встроенная дефолтная карта — тот же формат, что и файл на SD.
// Разделители — пробелы или табы. Строки с < 10 токенами игнорируются
// как комментарии, что позволяет писать пояснения прямо в файле.
// ============================================================
static const char* DEFAULT_MAP_STR[] = {
    "#  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #",
    "#  .  .  E  .  .  .  .  .  E  .  .  .  .  .  E  .  .  .  .  .  .  #",
    "#  .  .  .  #  #  #  .  .  .  .  .  .  .  #  #  .  .  .  .  .  .  #",
    "#  .  .  .  #  #  #  .  .  .  .  .  .  .  #  #  .  .  .  .  .  .  #",
    "#  .  .  .  .  .  .  .  #  @  @  @  #  .  #  #  .  ^  ^  .  .  .  #",
    "#  ~  ~  ~  .  .  .  .  #  ^  ^  ^  #  .  .  .  .  ^  ^  .  .  .  #",
    "#  ~  ~  ~  .  .  .  .  @  ^  G  ^  @  .  .  .  .  ^  ^  .  .  .  #",
    "#  ~  ~  ~  .  .  .  .  #  ^  ^  ^  #  .  .  .  .  ^  ^  .  .  .  #",
    "#  .  .  .  .  .  .  .  #  @  @  @  #  .  .  .  .  .  .  .  .  .  #",
    "#  .  .  .  #  #  #  .  .  .  .  .  .  .  .  .  .  #  #  #  .  .  #",
    "#  .  .  .  #  #  #  .  .  .  P  .  .  .  .  .  .  #  #  #  .  .  #",
    "#  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  #",
    "#  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #  #"
};

// ============================================================

GameMap::GameMap() {
    loadDefault();
}

void GameMap::clearSpecialPoints() {
    playerStartSet = false;
    playerStartX = 240;
    playerStartY = 185;

    baseSet = false;
    baseDestroyed = false;
    baseCol = baseRow = -1;
    baseX = baseY = 0;

    spawnCount = 0;
}

void GameMap::reset() {
    loadDefault();
}

void GameMap::initParsed(ParsedMap& pm) const {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            pm.tiles[r][c] = Tile::EMPTY;

    pm.playerStartSet = false;
    pm.playerStartX = 240;
    pm.playerStartY = 185;

    pm.baseSet = false;
    pm.baseCol = -1;
    pm.baseRow = -1;

    pm.spawnCount = 0;
}



// Парсер одной строки.
// Правила:
//   • Если в строке есть табы — делим строго по табам (пустые токены = пустые клетки).
//   • Если табов нет — пробелы схлопываются (несколько пробелов = один разделитель).
//   • Строка с < 10 токенов считается комментарием и пропускается.
bool GameMap::parseLine(const String& line, int row, ParsedMap& out) const {
    char tokens[COLS + 1];
    int  count = 0;
    int  len = line.length();

    bool useTab = (line.indexOf('\t') >= 0);

    if (useTab) {
        // Строгое разбиение по табам, без схлопывания
        int start = 0;
        for (int i = 0; i <= len && count < COLS; i++) {
            if (i == len || line[i] == '\t') {
                char ch = (i > start) ? line[start] : '.';
                tokens[count++] = ch;
                start = i + 1;
            }
        }
    } else {
        // Схлопывание пробелов
        int i = 0;
        while (i < len && count < COLS) {
            while (i < len && line[i] == ' ') i++;
            if (i >= len) break;
            tokens[count++] = line[i];
            while (i < len && line[i] != ' ') i++;
        }
    }

    if (count < 10) return false;

    for (int c = 0; c < COLS; c++) {
        char ch = (c < count) ? tokens[c] : '.';
        Tile t = Tile::EMPTY;

        switch (ch) {
            case '#': t = Tile::BRICK; break;
            case '@': t = Tile::STEEL; break;
            case '~': t = Tile::ICE;   break;
            case '^': t = Tile::FOG;   break;
            case 'P':
                out.playerStartSet = true;
                out.playerStartX = ORIGIN_X + c * TILE_SIZE + TILE_SIZE / 2;
                out.playerStartY = ORIGIN_Y + row * TILE_SIZE + TILE_SIZE / 2;
                break;
            case 'G':
                out.baseSet = true;
                out.baseCol = c;
                out.baseRow = row;
                break;
            case 'E':
                if (out.spawnCount < MAX_SPAWNS) {
                    out.spawnX[out.spawnCount] =
                        ORIGIN_X + c * TILE_SIZE + TILE_SIZE / 2;
                    out.spawnY[out.spawnCount] =
                        ORIGIN_Y + row * TILE_SIZE + TILE_SIZE / 2;
                    out.spawnCount++;
                }
                break;
            default:
                t = Tile::EMPTY;
                break;
        }
        out.tiles[row][c] = t;
    }
    return true;
}


bool GameMap::applyParsed(const ParsedMap& pm) {
    // Проверка, что все строки заполнены (иначе откат)
    // Тут уже доверяем парсеру

    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            tiles[r][c] = pm.tiles[r][c];

    playerStartSet = pm.playerStartSet;
    playerStartX   = pm.playerStartX;
    playerStartY   = pm.playerStartY;

    baseSet = pm.baseSet;
    baseDestroyed = false;
    baseCol = pm.baseCol;
    baseRow = pm.baseRow;
    if (baseSet) {
        baseX = ORIGIN_X + baseCol * TILE_SIZE + TILE_SIZE / 2;
        baseY = ORIGIN_Y + baseRow * TILE_SIZE + TILE_SIZE / 2;
    } else {
        baseX = baseY = 0;
    }

    spawnCount = pm.spawnCount;
    for (int i = 0; i < spawnCount; i++) {
        spawnX[i] = pm.spawnX[i];
        spawnY[i] = pm.spawnY[i];
    }
    return true;
}

bool GameMap::loadDefault() {
    int lineCount = sizeof(DEFAULT_MAP_STR) / sizeof(DEFAULT_MAP_STR[0]);

    ParsedMap pm;
    initParsed(pm);

    int row = 0;
    for (int i = 0; i < lineCount && row < ROWS; i++) {
        String line(DEFAULT_MAP_STR[i]);
        if (parseLine(line, row, pm)) row++;
    }

    if (row < ROWS) {
        Serial.printf("Дефолтная карта неполная: %d из %d строк\n", row, ROWS);
        return false;
    }
    return applyParsed(pm);
}

bool GameMap::loadFromFile(const char* path) {
    File f = SD.open(path);
    if (!f) {
        Serial.printf("Карта не найдена: %s\n", path);
        return false;
    }
    Serial.printf("Загрузка карты: %s\n", path);

    ParsedMap pm;
    initParsed(pm);

    int row = 0;
    while (f.available() && row < ROWS) {
        String line = f.readStringUntil('\n');
        line.replace("\r", "");
        if (parseLine(line, row, pm)) row++;
    }
    f.close();

    if (row < ROWS) {
        Serial.printf("Карта неполная: %d из %d строк\n", row, ROWS);
        return false;
    }

    bool ok = applyParsed(pm);
    if (ok) {
        Serial.printf("Карта OK: P=%d, G=%d, E=%d\n",
                    pm.playerStartSet ? 1 : 0,
                    pm.baseSet ? 1 : 0,
                    pm.spawnCount);
        for (int i = 0; i < pm.spawnCount; i++) {
            Serial.printf("  Точка %d: (%d, %d)\n", i,
                        pm.spawnX[i], pm.spawnY[i]);
        }
    }
    return ok;
}

bool GameMap::inBounds(int col, int row) const {
    return col >= 0 && col < COLS && row >= 0 && row < ROWS;
}

Tile GameMap::tileAt(int x, int y) const {
    if (x < ORIGIN_X || y < ORIGIN_Y) return Tile::EMPTY;
    int col = (x - ORIGIN_X) / TILE_SIZE;
    int row = (y - ORIGIN_Y) / TILE_SIZE;
    if (!inBounds(col, row)) return Tile::EMPTY;
    return tiles[row][col];
}

bool GameMap::isSolid(int x, int y) const {
    if (x < ORIGIN_X || y < ORIGIN_Y) return true;

    int col = (x - ORIGIN_X) / TILE_SIZE;
    int row = (y - ORIGIN_Y) / TILE_SIZE;
    if (!inBounds(col, row)) return true;

    if (baseSet && !baseDestroyed && col == baseCol && row == baseRow) return true;

    Tile t = tiles[row][col];
    // Лёд и туман не блокируют
    if (t == Tile::ICE || t == Tile::FOG) return false;

    return t != Tile::EMPTY;
}

bool GameMap::isSolidRect(int x1, int y1, int x2, int y2) const {
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

    if (baseSet && !baseDestroyed && col == baseCol && row == baseRow) {
        baseDestroyed = true;
        Serial.println("БАЗА РАЗРУШЕНА!");
        return true;
    }

    if (tiles[row][col] == Tile::BRICK) {
        tiles[row][col] = Tile::EMPTY;
        return true;
    }
    return false;
}

void GameMap::getSpawnPoint(int i, int& outX, int& outY) const {
    if (i < 0 || i >= spawnCount) {
        outX = 240; outY = 85; return;
    }
    outX = spawnX[i];
    outY = spawnY[i];
}

void GameMap::draw(Arduino_Canvas* gfx) const {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            Tile t = tiles[r][c];
            if (t == Tile::EMPTY) continue;

            int x = ORIGIN_X + c * TILE_SIZE;
            int y = ORIGIN_Y + r * TILE_SIZE;

            switch (t) {
                case Tile::BRICK:
                    gfx->fillRect(x, y, TILE_SIZE, TILE_SIZE, MAROON);
                    gfx->drawRect(x, y, TILE_SIZE, TILE_SIZE, ORANGE);
                    gfx->drawFastHLine(x + 1, y + TILE_SIZE / 2,
                                       TILE_SIZE - 2, ORANGE);
                    gfx->drawFastVLine(x + TILE_SIZE / 2, y + 1,
                                       TILE_SIZE / 2 - 1, ORANGE);
                    break;

                case Tile::STEEL:
                    gfx->fillRect(x, y, TILE_SIZE, TILE_SIZE, DARKGREY);
                    gfx->drawRect(x, y, TILE_SIZE, TILE_SIZE, WHITE);
                    gfx->drawRect(x + 3, y + 3, TILE_SIZE - 6, TILE_SIZE - 6, LIGHTGREY);
                    break;

                case Tile::ICE:
                    // Светло-голубой + диагональный блеск
                    gfx->fillRect(x, y, TILE_SIZE, TILE_SIZE, 0xB7FF);
                    gfx->drawRect(x, y, TILE_SIZE, TILE_SIZE, WHITE);
                    gfx->drawLine(x + 2, y + TILE_SIZE - 3,
                                  x + TILE_SIZE - 3, y + 2, WHITE);
                    gfx->drawLine(x + 6, y + TILE_SIZE - 3,
                                  x + TILE_SIZE - 3, y + 6, 0xDEFB);
                    break;

                case Tile::FOG:
                    // Серый полупрозрачный — рисуем сеткой
                    gfx->fillRect(x, y, TILE_SIZE, TILE_SIZE, 0x7BEF);
                    for (int i = 2; i < TILE_SIZE; i += 4) {
                        gfx->drawFastHLine(x + 1, y + i, TILE_SIZE - 2, LIGHTGREY);
                    }
                    break;

                default: break;
            }
        }
    }

    // База
    if (baseSet) {
        int x = ORIGIN_X + baseCol * TILE_SIZE;
        int y = ORIGIN_Y + baseRow * TILE_SIZE;

        if (baseDestroyed) {
            gfx->fillRect(x, y, TILE_SIZE, TILE_SIZE, DARKGREY);
            gfx->drawLine(x + 2, y + 2, x + TILE_SIZE - 2, y + TILE_SIZE - 2, RED);
            gfx->drawLine(x + TILE_SIZE - 2, y + 2, x + 2, y + TILE_SIZE - 2, RED);
        } else {
            gfx->fillRect(x, y, TILE_SIZE, TILE_SIZE, YELLOW);
            gfx->drawRect(x, y, TILE_SIZE, TILE_SIZE, ORANGE);
            gfx->fillTriangle(x + 4, y + 14,
                              x + 10, y + 4,
                              x + 10, y + 14, BLACK);
            gfx->fillTriangle(x + TILE_SIZE - 4, y + 14,
                              x + TILE_SIZE - 10, y + 4,
                              x + TILE_SIZE - 10, y + 14, BLACK);
        }
    }
}