
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

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
