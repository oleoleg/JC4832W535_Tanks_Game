//объявления меню

#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>  
#include "ui/ui_manager.h"

// Создаёт кнопки меню в переданном UIManager
void menuSetup(UIManager &ui);

// Рисует фон и заголовок меню (кнопки рисует UIManager)
void menuDraw(Arduino_Canvas *gfx);