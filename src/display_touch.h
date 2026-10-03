//объявления дисплея и тача


#pragma once
#include <Arduino_GFX_Library.h>

// Возвращает указатель на канвас для рисования
Arduino_Canvas* getCanvas();

// Инициализация дисплея и тача
bool initDisplayTouch();

// Чтение тача с маппингом в альбомные координаты
// Возвращает true, если есть касание, и заполняет x, y
bool readTouch(int &x, int &y);

void flushDisplay();

bool touchJustReleased();  // true один раз после отпускания