#include <Arduino.h>
#include <JC3248W535.h>
#include <SPI.h>
#include <SD.h>
#define SCREEN_W 480
#define SCREEN_H 320


// Поворот: 0 = портрет 320x480, 1 = альбом 480x320, 3 = альбом перевёрнутый
JC3248W535_Display display;
JC3248W535_Touch touch;

Arduino_Canvas *gfx = nullptr;

int lastX = -1;
int lastY = -1;
uint32_t lastDraw = 0;
int prevX = -1, prevY = -1;


void drawTestScreen() {
    int w = display.width();
    int h = display.height();

    gfx->fillScreen(BLACK);
    gfx->drawRect(0, 0, w, h, WHITE);

    // Четыре угла
    gfx->fillRect(0, 0, 40, 40, RED);                    // левый верх
    gfx->fillRect(w - 40, 0, 40, 40, GREEN);             // правый верх
    gfx->fillRect(0, h - 40, 40, 40, BLUE);              // левый низ
    gfx->fillRect(w - 40, h - 40, 40, 40, YELLOW);       // правый низ

    // Центр
    gfx->fillRect(w/2 - 30, h/2 - 30, 60, 60, MAGENTA);

    // Точка последнего касания
    if (lastX >= 0 && lastY >= 0) {
        gfx->fillCircle(lastX, lastY, 6, WHITE);
    }
}

void printSDCardInfo() {
    Serial.println("\n---------------------------------------------");
    Serial.println("         АНАЛИЗ TF / MicroSD КАРТЫ");
    Serial.println("---------------------------------------------");

    // Для JC3248W535 пины SPI для SD-карты обычно следующие:
    // MISO = 13, MOSI = 11, SCK = 12, CS = 10 (или 14 в зависимости от ревизии)
    // Если на пине 10 не заведется, попробуйте изменить CS_PIN на 14
    const int SD_CS_PIN = 10; 
    
    // Явно инициализируем шину SPI для SD-карты
    SPI.begin(12, 13, 11, SD_CS_PIN); // SCK, MISO, MOSI, SS

    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("❌ Ошибка: TF-карта не найдена или не вставлена!");
        Serial.println("Проверьте, что карта отформатирована в FAT32/exFAT.");
        Serial.println("---------------------------------------------");
        return;
    }

    // Определяем тип карты
    uint8_t cardType = SD.cardType();
    Serial.print("Тип карты:          ");
    if (cardType == CARD_MMC)  Serial.println("MMC");
    else if (cardType == CARD_SD)   Serial.println("SDSC");
    else if (cardType == CARD_SDHC) Serial.println("SDHC (Высокая емкость)");
    else                            Serial.println("Неизвестный тип");

    // Получаем размеры
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);

    Serial.printf("Полный объем карты:  %llu МБ\n", cardSize);
    Serial.printf("Общий объем FAT:     %llu МБ\n", totalBytes);
    Serial.printf("Использовано:        %llu МБ\n", usedBytes);
    Serial.printf("Свободно на карте:   %llu МБ\n", totalBytes - usedBytes);
    Serial.println("---------------------------------------------");
}


void printMemInfo() {

   Serial.println("\n=============================================");
    Serial.println("   ПОДРОБНЫЙ ОТЧЕТ О ПАМЯТИ ESP32-S3");
    Serial.println("=============================================");

    // 1. Внутренняя память (SRAM)
    Serial.printf("Внутренняя RAM (Всего кучи):    %d байт\n", ESP.getHeapSize());
    Serial.printf("Внутренняя RAM (Свободно):       %d байт\n", ESP.getFreeHeap());
    Serial.printf("Макс. неделимый блок в RAM:     %d байт\n", ESP.getMaxAllocHeap());
    Serial.println("---------------------------------------------");

    // 2. Внешняя память (PSRAM)
    Serial.printf("Внешняя PSRAM (Всего):          %d байт (8 МБ)\n", ESP.getPsramSize());
    Serial.printf("Внешняя PSRAM (Свободно):       %d байт\n", ESP.getFreePsram());
    Serial.printf("Макс. неделимый блок в PSRAM:   %d байт\n", ESP.getMaxAllocPsram());
    Serial.println("---------------------------------------------");

    // 3. Flash-память (Память программ)
    Serial.printf("Размер Flash-памяти чипа:       %d байт (%d МБ)\n", 
                  ESP.getFlashChipSize(), ESP.getFlashChipSize() / 1024 / 1024);
    Serial.printf("Скорость Flash-шины:            %d Гц\n", ESP.getFlashChipSpeed());
    
    // Определение режима Flash
    FlashMode_t mode = ESP.getFlashChipMode();
    Serial.printf("Режим работы Flash:             %s\n", 
                  (mode == FM_QIO ? "QIO (Quad IO)" : 
                  (mode == FM_QOUT ? "QOUT (Quad Output)" : 
                  (mode == FM_DIO ? "DIO (Dual IO)" : 
                  (mode == FM_DOUT ? "DOUT (Dual Output)" : "Неизвестно")))));
    Serial.println("=============================================\n");

}

void setupDisplayTouch() {
  // 1. Инициализируем дисплей
    if (!display.begin()) {
        Serial.println("Ошибка инициализации дисплея!");
        while (1) delay(1000);
    }

    // 2. Устанавливаем ПОВОРОТ (альбомная ориентация)
    display.setRotation(1); 
    
    // 3. Получаем канвас для рисования
    gfx = display.getCanvas();

    // 3. Инициализируем тач
    if (!touch.begin()) {
        Serial.println("Ошибка инициализации тачскрина!");
        while (1) delay(1000);
    }
    Serial.println("Дисплей и тачскрин проиницилизирован успешно!");
    Serial.println("---------------------------------------------");
}

void readTouch() {
    
TouchPoint point;
if (touch.read(point)) {
    int rawX = point.x;
    int rawY = point.y;

    // Переставляем оси и зеркалим X
    int x = rawY;
    int y = 319 - rawX;

    // Ограничение границ
    if (x < 0) x = 0;
    if (x > SCREEN_W - 1) x = SCREEN_W - 1;
    if (y < 0) y = 0;
    if (y > SCREEN_H - 1) y = SCREEN_H - 1;

    lastX = x;
    lastY = y;

    Serial.print("Raw(");
    Serial.print(rawX);
    Serial.print(",");
    Serial.print(rawY);
    Serial.print(") -> Screen(");
    Serial.print(x);
    Serial.print(",");
    Serial.print(y);
    Serial.println(")");
    }

}

void updateScreenAfterTouch()
{
    // Рисуем ТОЛЬКО если точка сдвинулась или раз в 50 мс
    if ((lastX != prevX || lastY != prevY) || (millis() - lastDraw > 50)) {
        prevX = lastX;
        prevY = lastY;
        lastDraw = millis();
        drawTestScreen();
        display.flush();
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);


    setupDisplayTouch();
    printMemInfo();
    printSDCardInfo();
 
    Serial.println("=== ГОТОВО ===");
}


void loop() {

    readTouch();
    updateScreenAfterTouch();

      

    //delay(16);
    
}