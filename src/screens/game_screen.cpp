#include "game_screen.h"
#include "screen_globals.h"
#include "screen_manager.h"
#include "config.h"
#include <math.h>

GameScreen::GameScreen()
    : player(240, 185, DARKGREEN),
      enemy (60,  85,  RED) {}

void GameScreen::onEnter() {
    moveAngle = -1;
    firePressed = false;
    showMoveIndicator = false;
    showFireIndicator = false;
    fireRequested = false;
    lastFireTime = 0;
    resetLevel();
}

void GameScreen::resetLevel() {
    lives = 3;
    enemiesRemaining = 1;
    gameOver = false;
    victory = false;

    map.reset();

    player.setPosition(240, 185);
    player.stop();

    enemy.setPosition(60, 85);
    enemy.setTargetAngle(0);
    enemy.setMoving(true);

    playerBullet.deactivate();
    enemyBullet.deactivate();

    nextAiDecisionTime = millis() + 800;
    nextEnemyFireTime  = millis() + 1500;
}

void GameScreen::update() {
    if (gameOver || victory) return;

    // Сохраняем позиции для отката при столкновении
    float px = player.getX(), py = player.getY();
    float ex = enemy.getX(),  ey = enemy.getY();

    // Обновляем объекты
    player.update(&map);
    enemy.update(&map);
    playerBullet.update(&map);
    enemyBullet.update(&map);

    // Столкновение танков
    if (tanksOverlap(player, enemy)) {
        // Сначала откатываем игрока
        player.setPosition(px, py);
        if (tanksOverlap(player, enemy)) {
            // Всё ещё пересекаются — откатываем врага
            enemy.setPosition(ex, ey);
        }
    }

    // ИИ врага
    updateEnemyAI();

    // Выстрел игрока
    if (fireRequested) {
        fireRequested = false;
        if (!playerBullet.isActive() &&
            millis() - lastFireTime > FIRE_COOLDOWN_MS) {
            playerBullet.fire(player.getX(), player.getY(), player.getAngle());
            lastFireTime = millis();
        }
    }

    // Проверка попаданий
    checkBulletHits();
}

void GameScreen::updateEnemyAI() {
    uint32_t now = millis();

    // Смена направления
    if (now >= nextAiDecisionTime) {
        static const int dirs[4] = {0, 90, 180, 270};
        enemy.setTargetAngle(dirs[random(4)]);
        enemy.setMoving(true);
        nextAiDecisionTime = now + 800 + random(1500);
    }

    // Если упёрся — сразу новая цель
    if (enemy.wasBlocked()) {
        static const int dirs[4] = {0, 90, 180, 270};
        enemy.setTargetAngle(dirs[random(4)]);
        nextAiDecisionTime = now + 500;
    }

    // Стрельба
    if (now >= nextEnemyFireTime && !enemyBullet.isActive()) {
        enemyBullet.fire(enemy.getX(), enemy.getY(), enemy.getAngle());
        nextEnemyFireTime = now + 1500 + random(1500);
    }
}

void GameScreen::checkBulletHits() {
    const float TANK_HALF    = 10.0f;
    const float BULLET_R     = 3.0f;
    const float HIT_DIST     = TANK_HALF + BULLET_R;
    const float HIT_DIST_SQ  = HIT_DIST * HIT_DIST;

    // Снаряд игрока → враг
    if (playerBullet.isActive()) {
        float dx = playerBullet.getX() - enemy.getX();
        float dy = playerBullet.getY() - enemy.getY();
        if (dx * dx + dy * dy < HIT_DIST_SQ) {
            playerBullet.deactivate();
            enemiesRemaining--;
            Serial.printf("Враг уничтожен! Осталось: %d\n", enemiesRemaining);

            if (enemiesRemaining <= 0) {
                victory = true;
                return;
            }

            // Спавн нового врага в случайной точке
            enemy.setPosition(60 + random(360), 85 + random(180));
            enemy.setMoving(true);
            nextAiDecisionTime = millis() + 500;
            nextEnemyFireTime  = millis() + 1500;
        }
    }

    // Снаряд врага → игрок
    if (enemyBullet.isActive()) {
        float dx = enemyBullet.getX() - player.getX();
        float dy = enemyBullet.getY() - player.getY();
        if (dx * dx + dy * dy < HIT_DIST_SQ) {
            enemyBullet.deactivate();
            lives--;
            Serial.printf("Игрок ранен! Жизней: %d\n", lives);

            if (lives <= 0) {
                gameOver = true;
                return;
            }

            // Респавн игрока
            player.setPosition(240, 185);
            player.stop();
        }
    }
}

bool GameScreen::tanksOverlap(const Tank& a, const Tank& b) const {
    float dx = a.getX() - b.getX();
    float dy = a.getY() - b.getY();
    const float MIN_DIST = 18.0f;   // чуть меньше размера танка (20)
    return (dx * dx + dy * dy) < (MIN_DIST * MIN_DIST);
}

void GameScreen::draw(Arduino_Canvas* gfx) {
    gfx->fillScreen(BLACK);

    // --- Игровое поле ---
    map.draw(gfx);
    enemy.draw(gfx);
    player.draw(gfx);
    enemyBullet.draw(gfx);
    playerBullet.draw(gfx);

    // --- Индикатор DPAD ---
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

    // --- Индикатор FIRE ---
    if (showFireIndicator) {
        gfx->drawCircle(FIRE_CX, FIRE_CY, FIRE_R, DARKGREY);
        gfx->fillCircle(fireIndicatorX, fireIndicatorY, 8, RED);
    }

    // --- HUD и оверлеи ---
    drawHUD(gfx);

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

    // Счётчик врагов (в центре)
    gfx->setTextColor(YELLOW);
    gfx->setTextSize(2);
    gfx->setCursor(190, 15);
    gfx->print("ENEMY: ");
    gfx->print(enemiesRemaining);

    // Жизни (справа)
    gfx->setTextColor(RED);
    gfx->setCursor(360, 15);
    gfx->print("HP: ");
    gfx->print(lives);
}

bool GameScreen::handleTouch(int x, int y, TouchEvent ev) {
    // Выход из оверлея — заново
    if ((gameOver || victory) && ev == TouchEvent::PRESSED) {
        resetLevel();
        return true;
    }

    // Кнопка «Назад»
    if (ev == TouchEvent::PRESSED) {
        if (x >= BACK_X && x <= BACK_X + BACK_W &&
            y >= BACK_Y && y <= BACK_Y + BACK_H) {
            getScreenManager()->pop();
            return true;
        }
    }

    // Зона направления
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

    // Зона выстрела
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
    return (dx * dx + dy * dy) <= (r * r);
}

void GameScreen::updateDirection(int x, int y) {
    int dx = x - DPAD_CX;
    int dy = y - DPAD_CY;
    float dist = sqrtf((float)(dx * dx + dy * dy));

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