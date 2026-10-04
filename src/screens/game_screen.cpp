#include "game_screen.h"
#include "screen_globals.h"
#include "screen_manager.h"
#include "config.h"
#include <math.h>

GameScreen* GameScreen::getInstance() {
    static GameScreen instance;
    return &instance;
}

String GameScreen::selectedMapPath = "";


GameScreen::GameScreen() : player(240, 185, DARKGREEN) {}

void GameScreen::initEnemies() {
    if (enemiesInit) return;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].tank   = new Tank(0, 0, RED);
        enemies[i].bullet = new Bullet();
        enemies[i].alive  = false;
    }
    enemiesInit = true;
}

void GameScreen::onEnter() {
    initEnemies();

    moveAngle = -1;
    firePressed = false;
    showMoveIndicator = false;
    showFireIndicator = false;
    fireRequested = false;
    lastFireTime = 0;

    resetLevel();
}

void GameScreen::resetLevel() {
    lives            = 3;
     spawnedCount = 0;
    enemiesRemaining = TOTAL_ENEMIES;
    gameOver         = false;
    victory          = false;
    autoFire         = false;

    // Сброс флага разрушения базы
    // (это делает applyParsed, но на всякий случай)

    bool loaded = false;
    if (selectedMapPath.length() > 0) {
        loaded = map.loadFromFile(selectedMapPath.c_str());
        if (!loaded) {
            Serial.printf("Не удалось загрузить %s, дефолт\n",
                          selectedMapPath.c_str());
        }
    }
    if (!loaded) {
        map.loadDefault();
    }

    // Игрок — на позиции из карты или в центре
    if (map.hasPlayerStart()) {
        player.setPosition(map.getPlayerStartX(), map.getPlayerStartY());
    } else {
        player.setPosition(240, 185);
    }
    player.stop();
    player.setRotationSpeed(12.0f);   // игрок резче врагов
    playerBullet.deactivate();

    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].alive = false;
        enemies[i].bullet->deactivate();
        enemies[i].tank->setRotationSpeed(6.0f);
    }

    lastSpawnTime = 0;
    trySpawnWave();
}

void GameScreen::update() {
    if (gameOver || victory) return;

    // Пересчитываем «сколько врагов осталось» каждый кадр:
    // = не заспавнены + живые сейчас
    int aliveCount = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].alive) aliveCount++;
    }
    enemiesRemaining = TOTAL_ENEMIES - (spawnedCount - aliveCount);

    if (enemiesRemaining <= 0 && !gameOver) {
        victory = true;
        return;
    }

    // === Сохраняем позиции для отката при столкновении ===
    float px = player.getX(), py = player.getY();

    // === Обновление объектов ===
    player.update(&map);
    playerBullet.update(&map);
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        enemies[i].tank->update(&map);
        enemies[i].bullet->update(&map);
    }

        // === Проверка: не разрушена ли база ===
    if (map.hasBase() && map.isBaseDestroyed() && !gameOver) {
        gameOver = true;
        Serial.println("База уничтожена — GAME OVER");
        return;
    }

    // === Столкновения танков: игрок ↔ враги ===
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;

        if (tanksOverlap(player, *enemies[i].tank)) {
            // Откатываем игрока
            player.setPosition(px, py);
            // Если всё ещё пересекается — откатываем врага
            if (tanksOverlap(player, *enemies[i].tank)) {
                // Не можем легко откатить врага, т.к. не сохранили — просто толкаем
                float dx = enemies[i].tank->getX() - player.getX();
                float dy = enemies[i].tank->getY() - player.getY();
                float len = sqrtf(dx*dx + dy*dy);
                if (len > 0.1f) {
                    enemies[i].tank->setPosition(
                        enemies[i].tank->getX() + dx / len * 2,
                        enemies[i].tank->getY() + dy / len * 2);
                }
            }
        }
    }

    // === ИИ врагов ===
    updateEnemyAI();

    // === Выстрел игрока ===
    bool wantFire = fireRequested || (autoFire && moveAngle >= 0);

    if (wantFire) {
        fireRequested = false;
        if (!playerBullet.isActive() &&
            millis() - lastFireTime > FIRE_COOLDOWN_MS) {
            playerBullet.fire(player.getX(), player.getY(), player.getAngle());
            lastFireTime = millis();
        }
    }
    // === Спавн новых волн ===
    trySpawnWave();

    
    // === Проверка попаданий ===
    checkBulletHits();
}

void GameScreen::trySpawnWave() {
    if (gameOver || victory) return;
    if (spawnedCount >= TOTAL_ENEMIES) return;

    // Свободный слот
    int freeSlot = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) { freeSlot = i; break; }
    }
    if (freeSlot < 0) return;

    // Задержка 1-4 сек
    if (lastSpawnTime != 0) {
        uint32_t delay = 1000 + random(3000);
        if (millis() - lastSpawnTime < delay) return;
    }

    spawnEnemy(freeSlot);
}

void GameScreen::spawnEnemy(int slot) {
    if (slot < 0 || slot >= MAX_ENEMIES) return;

    int spawnN = map.getSpawnCount();
    if (spawnN <= 0) {
        Serial.println("Спавн: нет точек E на карте!");
        return;
    }

    // Идём по точкам по кругу, начиная с (spawnedCount % spawnN)
    int start = spawnedCount % spawnN;

    for (int k = 0; k < spawnN; k++) {
        int p = (start + k) % spawnN;

        int sx, sy;
        map.getSpawnPoint(p, sx, sy);

        float hw = 10.0f;
        if (map.isSolidRect((int)(sx-hw), (int)(sy-hw),
                            (int)(sx+hw), (int)(sy+hw))) continue;
        if (isTankAt(sx, sy, slot)) continue;

        float dx = sx - player.getX();
        float dy = sy - player.getY();
        if (dx*dx + dy*dy < 900) continue;

        enemies[slot].tank->setPosition(sx, sy);
        enemies[slot].tank->setTargetAngle(90);
        enemies[slot].tank->setMoving(true);
        enemies[slot].bullet->deactivate();
        enemies[slot].nextAi   = millis() + 500;
        enemies[slot].nextFire = millis() + 1000 + random(1500);
        enemies[slot].alive    = true;

        spawnedCount++;
        lastSpawnTime = millis();

        Serial.printf("Спавн: слот %d, точка %d/%d, всего %d/%d\n",
                      slot, p + 1, spawnN, spawnedCount, TOTAL_ENEMIES);
        return;
    }

    Serial.println("Спавн: все точки заняты/заблокированы");
}


bool GameScreen::isTankAt(float x, float y, int exceptSlot) const {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (i == exceptSlot || !enemies[i].alive) continue;
        float dx = x - enemies[i].tank->getX();
        float dy = y - enemies[i].tank->getY();
        if (dx*dx + dy*dy < 900) return true;
    }
    return false;
}

void GameScreen::updateEnemyAI() {
    uint32_t now = millis();

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        Enemy& e = enemies[i];

        // Упёрся в стену — сразу новое направление
        if (e.tank->wasBlocked()) {
            int newDir = chooseSmartDirection(*e.tank);
            e.tank->setTargetAngle(newDir);
            e.tank->setMoving(true);
            e.nextAi = now + 400 + random(600);
        }
        // Или по таймеру
        else if (now >= e.nextAi) {
            int newDir = chooseSmartDirection(*e.tank);
            e.tank->setTargetAngle(newDir);
            e.nextAi = now + 1000 + random(1500);
        }

        // Стрельба
        if (now >= e.nextFire && !e.bullet->isActive()) {
            e.bullet->fire(e.tank->getX(), e.tank->getY(),
                           e.tank->getAngle());
            e.nextFire = now + 1500 + random(2000);
        }
    }
}

int GameScreen::chooseSmartDirection(const Tank& t) const {
    static const int dirs[4] = {0, 90, 180, 270};
    float hw = 9.0f;
    float lookahead = 40.0f;

    // Собираем свободные направления
    int free[4];
    int n = 0;
    for (int i = 0; i < 4; i++) {
        float rad = dirs[i] * PI / 180.0f;
        float ex = t.getX() + cosf(rad) * lookahead;
        float ey = t.getY() + sinf(rad) * lookahead;
        if (!map.isSolidRect((int)(ex-hw), (int)(ey-hw),
                             (int)(ex+hw), (int)(ey+hw))) {
            free[n++] = dirs[i];
        }
    }

    if (n == 0) {
        // Всё заблокировано — разворот
        return (int)(t.getAngle() + 180) % 360;
    }

    // 40% шанс идти в сторону игрока
    if (random(100) < 40) {
        float dx = player.getX() - t.getX();
        float dy = player.getY() - t.getY();
        float rad = atan2f(dy, dx);
        int toPlayer = (int)(rad * 180.0f / PI);
        if (toPlayer < 0) toPlayer += 360;

        int best = free[0];
        int bestDiff = 999;
        for (int i = 0; i < n; i++) {
            int diff = abs(free[i] - toPlayer);
            if (diff > 180) diff = 360 - diff;
            if (diff < bestDiff) {
                bestDiff = diff;
                best = free[i];
            }
        }
        return best;
    }

    return free[random(n)];
}

void GameScreen::checkBulletHits() {
    const float TANK_HALF = 10.0f;
    const float BULLET_R  = 3.0f;
    const float HIT_SQ    = (TANK_HALF + BULLET_R) * (TANK_HALF + BULLET_R);

    // === Снаряд игрока → враги ===
    if (playerBullet.isActive()) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].alive) continue;
            float dx = playerBullet.getX() - enemies[i].tank->getX();
            float dy = playerBullet.getY() - enemies[i].tank->getY();
            if (dx*dx + dy*dy < HIT_SQ) {
                playerBullet.deactivate();
                enemies[i].alive = false;
                enemies[i].bullet->deactivate();
                
                Serial.printf("Враг уничтожен! Осталось: %d\n", enemiesRemaining);

                if (enemiesRemaining <= 0) {
                    victory = true;
                }
                return;
            }
        }
    }

    // === Снаряды врагов → игрок ===
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        if (!enemies[i].bullet->isActive()) continue;

        float dx = enemies[i].bullet->getX() - player.getX();
        float dy = enemies[i].bullet->getY() - player.getY();
        if (dx*dx + dy*dy < HIT_SQ) {
            enemies[i].bullet->deactivate();
            lives--;
            Serial.printf("Игрок ранен! Жизней: %d\n", lives);

            if (lives <= 0) {
                gameOver = true;
                return;
            }

            // Респавн игрока
            player.setPosition(240, 185);
            player.stop();
            return;
        }
    }
}

bool GameScreen::tanksOverlap(const Tank& a, const Tank& b) const {
    float dx = a.getX() - b.getX();
    float dy = a.getY() - b.getY();
    const float MIN_DIST = 18.0f;
    return (dx*dx + dy*dy) < (MIN_DIST * MIN_DIST);
}

void GameScreen::draw(Arduino_Canvas* gfx) {
    gfx->fillScreen(BLACK);

    // === Игровое поле ===
    map.draw(gfx);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        enemies[i].tank->draw(gfx);
    }

    player.draw(gfx);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        enemies[i].bullet->draw(gfx);
    }

    playerBullet.draw(gfx);

    // === Индикатор DPAD ===
    if (showMoveIndicator) {
        gfx->drawCircle(DPAD_CX, DPAD_CY, DPAD_R, DARKGREY);
        gfx->drawCircle(DPAD_CX, DPAD_CY, 15, DARKGREY);
        if (moveAngle >= 0) {
            float rad = moveAngle * PI / 180.0;
            int lx = DPAD_CX + (int)(cos(rad) * DPAD_R * 0.9);
            int ly = DPAD_CY + (int)(sin(rad) * DPAD_R * 0.9);
            gfx->drawLine(DPAD_CX, DPAD_CY, lx, ly, GREEN);
            gfx->fillCircle(lx, ly, 5, GREEN);
        }
        gfx->fillCircle(moveIndicatorX, moveIndicatorY, 3, WHITE);
    }

    // === Индикатор FIRE ===
    if (showFireIndicator) {
        gfx->drawCircle(FIRE_CX, FIRE_CY, FIRE_R, DARKGREY);
        gfx->fillCircle(fireIndicatorX, fireIndicatorY, 8, RED);
    }

    // === HUD ===
    drawHUD(gfx);

    // === Оверлеи ===
    if (gameOver) {
        gfx->fillRect(120, 130, 240, 60, BLACK);
        gfx->drawRect(120, 130, 240, 60, RED);
        gfx->setTextColor(RED);
        gfx->setTextSize(3);
        gfx->setCursor(150, 145);
        gfx->print("GAME OVER");
    }
    else if (victory) {
        gfx->fillRect(120, 130, 240, 60, BLACK);
        gfx->drawRect(120, 130, 240, 60, GREEN);
        gfx->setTextColor(GREEN);
        gfx->setTextSize(3);
        gfx->setCursor(160, 145);
        gfx->print("VICTORY");
    }
}

void GameScreen::drawHUD(Arduino_Canvas* gfx) {
    // BACK
    gfx->fillRect(BACK_X, BACK_Y, BACK_W, BACK_H, DARKGREY);
    gfx->drawRect(BACK_X, BACK_Y, BACK_W, BACK_H, WHITE);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(BACK_X + 14, BACK_Y + 13);
    gfx->print("BACK");

    // Счётчик врагов
    gfx->setTextColor(YELLOW);
    gfx->setTextSize(2);
    gfx->setCursor(150, 15);
    gfx->print("ENEMY: ");
    gfx->print(enemiesRemaining);

    // Жизни
    gfx->setTextColor(RED);
    gfx->setCursor(370, 15);
    gfx->print("HP: ");
    gfx->print(lives);

    // Кнопка AUTO рядом с BACK
    uint16_t autoFill = autoFire ? RED : DARKGREY;
    gfx->fillRect(AUTO_X, AUTO_Y, AUTO_W, AUTO_H, autoFill);
    gfx->drawRect(AUTO_X, AUTO_Y, AUTO_W, AUTO_H, WHITE);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(AUTO_X + 14, AUTO_Y + 13);
    gfx->print("AUTO");
}

bool GameScreen::handleTouch(int x, int y, TouchEvent ev) {
    if ((gameOver || victory) && ev == TouchEvent::PRESSED) {
        resetLevel();
        return true;
    }

    if (ev == TouchEvent::PRESSED) {
        if (x >= BACK_X && x <= BACK_X + BACK_W &&
            y >= BACK_Y && y <= BACK_Y + BACK_H) {
            getScreenManager()->pop();
            return true;
        }
    }

    // DPAD
    if (ev == TouchEvent::PRESSED && inCircle(x, y, DPAD_CX, DPAD_CY, DPAD_R)) {
        showMoveIndicator = true;
        moveIndicatorX = x;
        moveIndicatorY = y;
        updateDirection(x, y);
        if (moveAngle >= 0) {
            player.setTargetAngle(moveAngle);
            player.setMoving(true);
        }
        return true;
    }
    if (ev == TouchEvent::MOVED && showMoveIndicator) {
        moveIndicatorX = x;
        moveIndicatorY = y;
        updateDirection(x, y);
        if (moveAngle >= 0) {
            player.setTargetAngle(moveAngle);
            player.setMoving(true);
        }
        return true;
    }
    if (ev == TouchEvent::RELEASED && showMoveIndicator) {
        showMoveIndicator = false;
        moveAngle = -1;
        player.stop();
        return true;
    }

    // ——— Кнопка AUTO (toggle) ———
    if (ev == TouchEvent::PRESSED &&
        x >= AUTO_X && x <= AUTO_X + AUTO_W &&
        y >= AUTO_Y && y <= AUTO_Y + AUTO_H) {
        autoFire = !autoFire;
        Serial.printf("AUTO FIRE: %s\n", autoFire ? "ON" : "OFF");
        return true;
    }

    // FIRE
    if (ev == TouchEvent::PRESSED && inCircle(x, y, FIRE_CX, FIRE_CY, FIRE_R)) {
        firePressed = true;
        fireRequested = true;
        showFireIndicator = true;
        fireIndicatorX = x;
        fireIndicatorY = y;
        return true;
    }
    if (ev == TouchEvent::RELEASED && showFireIndicator) {
        firePressed = false;
        showFireIndicator = false;
        return true;
    }

    return false;
}

// ——— Вспомогательные ———

bool GameScreen::inCircle(int x, int y, int cx, int cy, int r) const {
    int dx = x - cx;
    int dy = y - cy;
    return (dx*dx + dy*dy) <= (r*r);
}

void GameScreen::updateDirection(int x, int y) {
    int dx = x - DPAD_CX;
    int dy = y - DPAD_CY;
    float dist = sqrtf((float)(dx*dx + dy*dy));

    if (dist < 15) { moveAngle = -1; return; }

    float rad = atan2((float)dy, (float)dx);
    int deg = (int)(rad * 180.0 / PI);
    if (deg < 0) deg += 360;
    moveAngle = deg;

    float norm = (dist - 15.0f) / (DPAD_R - 15.0f);
    if (norm > 1.0f) norm = 1.0f;
    if (norm < 0.0f) norm = 0.0f;
    player.setSpeedFactor(SPEED_NEAR + (SPEED_FAR - SPEED_NEAR) * norm);
}

const char* GameScreen::angleToText() const {
    if (moveAngle < 0) return "STOP";
    if (moveAngle < 22 || moveAngle >= 338) return "RIGHT";
    if (moveAngle < 67)  return "DOWN-RIGHT";
    if (moveAngle < 112) return "DOWN";
    if (moveAngle < 157) return "DOWN-LEFT";
    if (moveAngle < 202) return "LEFT";
    if (moveAngle < 247) return "UP-LEFT";
    if (moveAngle < 292) return "UP";
    return "UP-RIGHT";
}