#include "screen_manager.h"

void ScreenManager::push(Screen* screen) {
    if (!screen) return;
    if (depth >= MAX_SCREEN_STACK) {
        Serial.println("ScreenManager: стек переполнен!");
        return;
    }
    if (depth > 0) {
        stack[depth - 1]->onExit();
    }
    stack[depth++] = screen;
    screen->onEnter();
    Serial.printf("Screen: push -> %s (depth %d)\n", screen->name(), depth);
}

void ScreenManager::pop() {
    if (depth <= 1) {
        Serial.println("ScreenManager: нельзя закрыть последний экран");
        return;
    }
    stack[depth - 1]->onExit();
    depth--;
    stack[depth - 1]->onEnter();
    Serial.printf("Screen: pop -> %s (depth %d)\n", stack[depth - 1]->name(), depth);
}

void ScreenManager::replace(Screen* screen) {
    if (!screen || depth == 0) return;
    stack[depth - 1]->onExit();
    stack[depth - 1] = screen;
    screen->onEnter();
    Serial.printf("Screen: replace -> %s\n", screen->name());
}

Screen* ScreenManager::current() {
    return (depth > 0) ? stack[depth - 1] : nullptr;
}

void ScreenManager::draw(Arduino_Canvas* gfx) {
    if (depth > 0) stack[depth - 1]->draw(gfx);
}

bool ScreenManager::handleTouch(int x, int y, TouchEvent ev) {
    if (depth > 0) return stack[depth - 1]->handleTouch(x, y, ev);
    return false;
}

void ScreenManager::update() {
    if (depth > 0) stack[depth - 1]->update();
}