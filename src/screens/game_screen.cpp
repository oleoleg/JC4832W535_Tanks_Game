#include "game_screen.h"
#include "screen_globals.h"
#include "screen_manager.h"
#include "config.h"
#include <math.h>


const TankTypeInfo TANK_TYPES[T_COUNT] = {
    // name      color     hp  speed  pts   fire
    { "BASIC",   RED,      1,  1.0f,  100,  2000 },
    { "FAST",    MAGENTA,  1,  1.8f,  200,  2500 },
    { "HEAVY",   NAVY,     3,  0.7f,  300,  2200 },
    { "SNIPER",  CYAN,     1,  1.0f,  400,  1200 },
    { "BOSS",    LIGHTGREY,5,  0.6f,  1000, 1500 },
    { "KATYUSHA", ORANGE,  1,  0.6f,  800,  0 },
};

static int angleToCenter(float fromX, float fromY) {
    float cx = GameMap::ORIGIN_X + GameMap::COLS * GameMap::TILE_SIZE / 2.0f;
    float cy = GameMap::ORIGIN_Y + GameMap::ROWS * GameMap::TILE_SIZE / 2.0f;
    float rad = atan2f(cy - fromY, cx - fromX);
    int deg = (int)(rad * 180.0f / PI);
    if (deg < 0) deg += 360;
    return deg;
}



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
    lives   = 3;
    score   = 0;
    spawnedCount = 0;
    gameOver  = false;
    victory   = false;
    autoFire  = false;

    // Сброс флага разрушения базы
    // (это делает applyParsed, но на всякий случай)

    // Загрузка карты
    bool loaded = false;
    if (selectedMapPath.length() > 0) {
        loaded = map.loadFromFile(selectedMapPath.c_str());
    }
    if (!loaded) map.loadDefault();


    // Состав врагов
    int counts[T_COUNT];
    map.getEnemyTypeCounts(counts);
    enemiesTotal = 0;
   for (int i = 0; i < T_COUNT - 1; i++) {   // БЕЗ KATYUSHA
        remainingTypeCount[i] = counts[i];
        enemiesTotal += counts[i];
    }
    remainingTypeCount[T_KATYUSHA] = 0;   // в основном счёте не участвует
    katyushaMax = counts[T_KATYUSHA];   // максимум одновременно
    maxOnField = map.getMaxEnemies();
    Serial.printf("Состав врагов: ");
    for (int i = 0; i < T_COUNT; i++) {
        Serial.printf("%s=%d ", TANK_TYPES[i].name, counts[i]);
    }
    Serial.printf("(всего %d)\n", enemiesTotal);


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
        enemies[i].alive            = false;
        enemies[i].katyushaState    = 0;
        enemies[i].katyushaTargetX  = 0;
        enemies[i].katyushaTimer    = 0;
        enemies[i].rocketsFired     = 0;
        enemies[i].hp               = 0;
        enemies[i].points           = 0;
        enemies[i].bullet->deactivate();
        enemies[i].tank->reset(90);
        enemies[i].tank->setRotationSpeed(6.0f);
    }

    for (int i = 0; i < Rocket::MAX_ROCKETS; i++) {
        rockets[i].deactivate();
    }

    lastSpawnTime = 0;
    nextKatyushaTime = millis() + 6000;   // первая Катюша через 6 секунд

    trySpawnWave();
}

void GameScreen::update() {
    if (gameOver || victory) return;

    // Пересчитываем «сколько врагов осталось» каждый кадр:
    // = не заспавнены + живые сейчас
    int aliveCount = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].alive && enemies[i].katyushaState == 0) aliveCount++;
    }
    enemiesRemaining = enemiesTotal - (spawnedCount - aliveCount);

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

    
      // === Спавн Катюши ===
    if (katyushaMax > 0 && millis() >= nextKatyushaTime) {
        // Считаем активных
        int activeKatyusha = 0;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].alive && enemies[i].katyushaState != 0) activeKatyusha++;
        }

        if (activeKatyusha < katyushaMax) {
            // Спавним до 2 за раз, чтобы они появлялись кучнее
            int spawnedNow = 0;
            while (activeKatyusha < katyushaMax && spawnedNow < 2) {
                if (!spawnKatyusha()) break;
                activeKatyusha++;
                spawnedNow++;
            }

            // Интервал зависит от katyushaMax:
            //  1 → ~15-20 сек
            //  3 → ~7-10 сек
            //  5 → ~4-6 сек
            // 10 → ~2-3 сек
            uint32_t base = 20000 / katyushaMax;
            if (base < 2000) base = 2000;   // минимум 2 сек
            uint32_t jitter = base / 3;
            nextKatyushaTime = millis() + base + random(jitter * 2) - jitter;
        } else {
            // На поле уже максимум — проверяем через 1 сек
            nextKatyushaTime = millis() + 1000;
        }
    }

        // === Ракеты ===
    updateRockets(millis());

    // === Проверка попаданий ===
    checkBulletHits();
}

void GameScreen::trySpawnWave() {
    if (gameOver || victory) return;

    int aliveCount = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].alive && enemies[i].katyushaState == 0) aliveCount++;
    }


// ОТЛАДКА — покажет реальные цифры
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 1000) {
        Serial.printf("[trySpawn] alive=%d, maxOnField=%d, spawned=%d/%d\n",
                      aliveCount, maxOnField, spawnedCount, enemiesTotal);
        lastPrint = millis();
    }

    if (aliveCount >= maxOnField) return;
    if (spawnedCount >= enemiesTotal) return;

    int freeSlot = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) { freeSlot = i; break; }
    }
    if (freeSlot < 0) return;

    if (lastSpawnTime != 0) {
        uint32_t delay = 1000 + random(3000);
        if (millis() - lastSpawnTime < delay) return;
    }


    //if (aliveCount >= maxOnField && maxOnField > 0) return;
   //if (aliveCount >= 6) return;   // жёсткий кап на всякий случай


    spawnEnemy(freeSlot);
}

void GameScreen::spawnEnemy(int slot) {
    if (slot < 0 || slot >= MAX_ENEMIES) return;

    int typeIdx = pickNextType();
    if (typeIdx < 0) return;

    const TankTypeInfo& info = TANK_TYPES[typeIdx];

    int spawnN = map.getSpawnCount();
    if (spawnN <= 0) return;

    int start = spawnedCount % spawnN;
    for (int k = 0; k < spawnN; k++) {
        int p = (start + k) % spawnN;
        int sx, sy;
        map.getSpawnPoint(p, sx, sy);

        if (map.isSolid(sx, sy)) continue;
        if (isTankAt(sx, sy, slot)) continue;

        float dx = sx - player.getX();
        float dy = sy - player.getY();
        if (dx*dx + dy*dy < 900) continue;

        // Настраиваем танк
        int ang = angleToCenter(sx, sy); 
        enemies[slot].tank->setPosition(sx, sy);
        enemies[slot].tank->reset(ang);
        enemies[slot].tank->setTargetAngle(ang);  
        enemies[slot].tank->setMoving(true);
        enemies[slot].tank->setColor(info.color);
        enemies[slot].tank->setTypeSpeed(info.typeSpeed);
        enemies[slot].tank->setRotationSpeed(typeIdx == T_FAST ? 10.0f : 6.0f);

        enemies[slot].bullet->deactivate();
        enemies[slot].nextAi   = millis() + 500;
        enemies[slot].nextFire = millis() + 1000 + random(1500);
        enemies[slot].alive    = true;
        enemies[slot].hp       = info.hp;
        enemies[slot].maxHp    = info.hp;
        enemies[slot].points   = info.points;

        spawnedCount++;
        lastSpawnTime = millis();

        Serial.printf("Спавн: %s (HP=%d) слот %d, точка %d/%d\n",
                      info.name, info.hp, slot, p + 1, spawnN);
        return;
    }
    // Не нашли место — вернём тип обратно
    remainingTypeCount[typeIdx]++;
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

        if (e.katyushaState != 0) {
            updateKatyusha(e, now);
            continue;   // Катюша не подчиняется обычному ИИ
        }

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
            e.bullet->fire(e.tank->getX(), e.tank->getY(), e.tank->getAngle());
            uint32_t base = TANK_TYPES[T_BASIC].fireDelay;
            // Найдём тип по цвету/ссылке — проще хранить в Enemy
            // Здесь используем фиксированный интервал
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
                enemies[i].hp--;

                if (enemies[i].hp <= 0) {
                    enemies[i].alive = false;
                    enemies[i].bullet->deactivate();
                    enemies[i].tank->reset(90);  
                    enemies[i].katyushaState = 0;  
                    enemies[i].katyushaTargetX = 0;
                    score += enemies[i].points;
                    Serial.printf("Враг уничтожен! +%d очков (всего %d)\n",
                                  enemies[i].points, score);
                } else {
                    Serial.printf("Попадание! HP врага: %d\n", enemies[i].hp);
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

            // Респавн игрока — в точке P из карты
            if (map.hasPlayerStart()) {
                player.setPosition(map.getPlayerStartX(), map.getPlayerStartY());
            } else {
                player.setPosition(240, 185);
            }
            player.stop();
            player.setTargetAngle(player.getAngle());   // фиксируем текущий угол
            player.setSpeedFactor(1.0f);                // сброс скорости
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

        // Туман: скрываем врага, если он на клетке FOG
        Tile t = map.tileAt((int)enemies[i].tank->getX(),
                            (int)enemies[i].tank->getY());
        if (t == Tile::FOG) {
            // Полупрозрачная подсказка — просто точка
            gfx->fillCircle((int)enemies[i].tank->getX(),
                            (int)enemies[i].tank->getY(), 2, DARKGREY);
            continue;
        }
        
        if (enemies[i].katyushaState != 0) {
            enemies[i].tank->drawKatyusha(gfx);
        } else {
            enemies[i].tank->draw(gfx);
        }
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


    // Ракеты — поверх всего игрового поля
    for (int i = 0; i < Rocket::MAX_ROCKETS; i++) {
        rockets[i].draw(gfx);
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
    gfx->setTextSize(1);
    gfx->setCursor(150, 20);
    gfx->print("E:");
    gfx->setTextSize(2);
    gfx->setCursor(165, 18);
    gfx->print(enemiesRemaining);

    // Очки
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(210, 20);
    gfx->print("SCORE:");
    gfx->setTextSize(2);
    gfx->setCursor(250, 18);
    gfx->print(score);

    // Жизни
    gfx->setTextColor(RED);
    gfx->setTextSize(1);
    gfx->setCursor(400, 20);
    gfx->print("HP:");
    gfx->setTextSize(2);
    gfx->setCursor(420, 18);
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


int GameScreen::pickNextType() {
    int totalRemaining = 0;
    for (int i = 0; i < T_COUNT; i++) totalRemaining += remainingTypeCount[i];
    if (totalRemaining == 0) return -1;

    int pick = random(totalRemaining);
    for (int i = 0; i < T_COUNT; i++) {
        if (pick < remainingTypeCount[i]) {
            remainingTypeCount[i]--;
            return i;
        }
        pick -= remainingTypeCount[i];
    }
    return -1;
}

bool GameScreen::spawnKatyusha() {
    // Найти свободный слот
    int slot = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) { slot = i; break; }
    }
    if (slot < 0) return false;

    Enemy& e = enemies[slot];
    const TankTypeInfo& info = TANK_TYPES[T_KATYUSHA];

    // Въезжает слева или справа, на уровне середины поля
    bool fromLeft = random(2) == 0;
    int y = GameMap::ORIGIN_Y + 4 * GameMap::TILE_SIZE + random(4) * GameMap::TILE_SIZE;

    float startX, startY, targetX;

    if (fromLeft) {
        startX  = GameMap::ORIGIN_X + GameMap::TILE_SIZE * 0.5f;
        targetX = GameMap::ORIGIN_X + GameMap::TILE_SIZE * 3.0f;
    } else {
        startX  = GameMap::ORIGIN_X + (GameMap::COLS - 0.5f) * GameMap::TILE_SIZE;
        targetX = GameMap::ORIGIN_X + (GameMap::COLS - 3.0f) * GameMap::TILE_SIZE;
    }
    startY = y;

    e.tank->setPosition((int)startX, (int)startY);
    e.tank->reset(fromLeft ? 0 : 180);
    e.tank->setTargetAngle(fromLeft ? 0 : 180);
    e.tank->setMoving(true);
    e.tank->setColor(info.color);
    e.tank->setTypeSpeed(info.typeSpeed);
    e.tank->setRotationSpeed(4.0f);

    e.bullet->deactivate();
    e.alive = true;
    e.hp = info.hp;
    e.maxHp = info.hp;
    e.points = info.points;

    e.katyushaState  = 1;              // въезжает
    e.katyushaTimer  = millis();
    e.rocketsFired   = 0;
    e.rocketsTotal   = 3 + random(3);  // 3-5 ракет

    // Сохраняем целевую X во временном поле nextAi
    // (нет, лучше в отдельном поле)
    // ... см. ниже

    e.katyushaTargetX = targetX;
 
    Serial.println("Катюша выехала!");
    return true;
}

void GameScreen::updateKatyusha(Enemy& e, uint32_t now) {
    const int leftEdge  = GameMap::ORIGIN_X - 25;
    const int rightEdge = GameMap::ORIGIN_X
                        + GameMap::COLS * GameMap::TILE_SIZE + 25;

    switch (e.katyushaState) {

        // 1. Въезжает. Таймаут 2 сек → всё равно стреляет
        case 1: {
            float cx = e.tank->getX();
            float dx = e.katyushaTargetX - cx;
            bool arrived = (fabs(dx) < 4.0f);
            bool timeout = (now - e.katyushaTimer > 2000);

            if (arrived || timeout) {
                e.tank->stop();
                e.katyushaState = 2;
                e.katyushaTimer = now;
                Serial.printf("Катюша: %s, готов к залпу\n",
                              arrived ? "доехала" : "таймаут въезда");
            }
            break;
        }

        // 2. Стоит 400 мс и делает залп, потом уезжает
        case 2: {
            if (now - e.katyushaTimer < 400) break;

            for (int k = 0; k < e.rocketsTotal; k++) {
                launchRocketBarrage(e);
            }
            Serial.printf("Катюша: залп %d ракет\n", e.rocketsTotal);

            // Разворот к ближайшему краю
            float cx = e.tank->getX();
            float centerX = GameMap::ORIGIN_X
                          + (GameMap::COLS * GameMap::TILE_SIZE) / 2.0f;
            int outAngle = (cx < centerX) ? 180 : 0;

            e.tank->setTargetAngle(outAngle);
            e.tank->setMoving(true);
            e.katyushaState = 3;
            e.katyushaTimer = now;
            break;
        }

        // 3. Уезжает. Отъехала за край → скрыть.
        //    Или застряла (3 сек) → тоже скрыть.
        case 3: {
            float cx = e.tank->getX();
            bool offMap  = (cx < leftEdge || cx > rightEdge);
            bool timeout = (now - e.katyushaTimer > 3000);

            if (offMap || timeout) {
                e.alive = false;
                e.katyushaState = 0;
                Serial.printf("Катюша уехала (%s)\n",
                              offMap ? "за край" : "по таймауту");
            }
            break;
        }
    }
}


void GameScreen::launchRocketBarrage(Enemy& e) {

    // Локальные алиасы для удобства
    const int OX = GameMap::ORIGIN_X;
    const int OY = GameMap::ORIGIN_Y;
    const int TS = GameMap::TILE_SIZE;
    const int NC = GameMap::COLS;
    const int NR = GameMap::ROWS;


    for (int i = 0; i < Rocket::MAX_ROCKETS; i++) {
        if (!rockets[i].isActive()) {
            int rx = OX + random(NC) * TS + TS / 2;
            int ry = OY + random(NR) * TS + TS / 2;

            if (random(100) < 50) {
                rx = (int)player.getX() + random(-60, 60);
                ry = (int)player.getY() + random(-60, 60);
                if (rx < OX + 10) rx = OX + 10;
                if (rx > OX + NC * TS - 10) rx = OX + NC * TS - 10;
                if (ry < OY + 10) ry = OY + 10;
                if (ry > OY + NR * TS - 10) ry = OY + NR * TS - 10;
            }

            rockets[i].launch(rx, ry, millis());
            Serial.printf("Катюша: ракета %d в (%d, %d)\n", i, rx, ry);
            return;
        }
    }
}



void GameScreen::updateRockets(uint32_t now) {
    for (int i = 0; i < Rocket::MAX_ROCKETS; i++) {
        if (!rockets[i].isActive()) continue;

        bool wasExploding = rockets[i].isExploding();
        rockets[i].update(now, &map);

        // Если только что начал взрываться — проверяем урон
        if (!wasExploding && rockets[i].isExploding()) {
            if (rockets[i].inBlast(player.getX(), player.getY())) {
                lives--;
                Serial.printf("Катюша попала! Жизней: %d\n", lives);

                if (lives <= 0) {
                    gameOver = true;
                    return;
                }

                // Респавн в точке P
                if (map.hasPlayerStart()) {
                    player.setPosition(map.getPlayerStartX(),
                                       map.getPlayerStartY());
                } else {
                    player.setPosition(240, 185);
                }
                player.stop();
            }
        }
    }
}


