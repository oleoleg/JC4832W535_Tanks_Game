#pragma once

// Что произошло с кнопкой
enum class ButtonEvent {
    PRESSED,    // палец только что коснулся
    RELEASED,   // палец отпущен
    HELD        // палец удерживается (для долгих нажатий)
};

// Тип функции-обработчика
using ButtonCallback = void(*)(ButtonEvent event);