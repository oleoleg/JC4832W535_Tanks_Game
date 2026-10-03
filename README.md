# JC4832W535 Tanks Game — Display Starter

Заготовка для проектов на плате JC4832W535 (ESP32-S3 + AXS15231B).
Рабочий каркас: дисплей, тач, SD-карта, отладка.

## Что работает

- ✅ Дисплей AXS15231B, альбомная ориентация 480×320
- ✅ Тачскрин AXS15231B, координаты попадают точно
- ✅ SD-карта (TF), FAT32, чтение размера и типа
- ✅ Отчёт по памяти: 16MB Flash, 8MB PSRAM, QIO
- ✅ Отладка через встроенный USB-JTAG (точки останова, переменные)
- ⏳ Bluetooth-геймпад — в разработке

## Что не работает / ограничения

- ⚠️ Полный кадр `display.flush()` занимает ~200 мс (из-за ограничение QSPI скорей всего)


## Аппаратная конфигурация

### Дисплей (QSPI)
- BL:   GPIO 1
- CS:   GPIO 45
- SCK:  GPIO 47
- D0:   GPIO 21
- D1:   GPIO 48
- D2:   GPIO 40
- D3:   GPIO 39

### Тачскрин (I2C)
- SDA:  GPIO 4
- SCL:  GPIO 8
- INT:  не используется (-1)
- RST:  не используется (-1)


## Ключевые моменты

### Структура файлов cpp:

src/
├── main.cpp              — точка входа, setup() и loop()
├── config.h              — все пины, константы, размеры
├── app_state.h           — перечисление состояний приложения
├── display_touch.h       — объявления дисплея и тача
├── display_touch.cpp     — инициализация и вспомогательные функции
├── menu.h                — объявления меню
├── menu.cpp              — отрисовка и обработка меню
├── wifi_manager.h        — объявления Wi-Fi и веб-сервера
├── wifi_manager.cpp      — точка доступа, сервер, загрузка файлов
└── sd_manager.h          — объявления работы с SD

### Инициализация дисплея:

```cpp
JC3248W535_Display display;   // без аргументов
display.begin(); //Поворот задаётся после этой строки
display.setRotation(1);        // 1 = альбом, 3 = альбом перевёрнутый
gfx = display.getCanvas();
```

### Маппинг координат тача:

Тач отдаёт портретные координаты (320×480), экран альбомный (480×320).
Формула преобразования:

```cpp
int x = rawY;
int y = 319 - rawX;
```

## Полная перерисовка кадра

AXS15231B не поддерживает частичное обновление. Каждый кадр нужно:

1. gfx->fillScreen(BLACK) — очистить
2. Нарисовать всё заново
3. display.flush() — отправить один полный кадр

## Отладка

В platformio.ini:

    build_type = debug
    debug_tool = esp-builtin
    debug_speed = 5000

Точки останова работают, переменные видны.
Для загрузки без отладки — закомментировать build_type = debug и нажать Upload.

## Зависимости

- moononournation/GFX Library for Arduino@1.4.9
- https://github.com/me-processware/JC3248W535-Driver

## Платформа

- PlatformIO
- Framework: Arduino
- Board: esp32-s3-devkitc-1
- Flash: 16MB, QIO, 80MHz
- PSRAM: включена, QIO OPI