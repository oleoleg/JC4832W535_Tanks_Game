//перечисление состояний приложения

#pragma once

enum AppState {
    STATE_MENU,        // главное меню
    STATE_TEST,        // тестовый экран
    STATE_GAME,        // игра «Танчики»
    STATE_DOORBELL,    // видеодомофон
    STATE_WIFI_UPLOAD  // Wi-Fi загрузка файлов
};