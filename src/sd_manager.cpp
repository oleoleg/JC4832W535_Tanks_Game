#include "sd_manager.h"
#include "config.h"
#include <SPI.h>
#include <SD.h>

static SPIClass sdSpi(HSPI);
static bool sdReady = false;

bool sdInit() {
    sdSpi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, sdSpi, 40000000)) {
        Serial.println("SD: ошибка инициализации!");
        sdReady = false;
        return false;
    }
    sdReady = true;
    Serial.printf("SD: OK, %llu MB\n", SD.cardSize() / (1024 * 1024));
    return true;
}

bool sdIsReady() {
    return sdReady;
}

String sdFreeSpace() {
    if (!sdReady) return "SD not ready";
    uint64_t total = SD.totalBytes() / (1024 * 1024);
    uint64_t used  = SD.usedBytes() / (1024 * 1024);
    uint64_t free  = total - used;
    char buf[80];
    snprintf(buf, sizeof(buf), "Free: %llu MB / Total: %llu MB", free, total);
    return String(buf);
}

String sdListFiles() {
    if (!sdReady) return "<li>SD not ready</li>";

    String html = "";
    File root = SD.open("/");
    if (!root) return "<li>Cannot open root</li>";

    int count = 0;
    File file = root.openNextFile();
    while (file && count < 100) {
        if (!file.isDirectory()) {
            String name = String(file.name());
            size_t size = file.size();

            html += "<li>";
            html += name;
            html += " — ";
            html += String(size);
            html += " bytes";

            // Кнопка удаления с подтверждением
            html += " <a href='/delete?file=";
            html += name;
            html += "' onclick='return confirm(\"Delete ";
            html += name;
            html += "?\")'>[X]</a>";
            html += "</li>";
            count++;
        }
        file = root.openNextFile();
    }
    if (count >= 100) html += "<li>... (truncated)</li>";
    return html;
}

bool sdDeleteFile(const String &filename) {
    if (!sdReady) return false;

    // Защита от удаления файлов вне корня
    if (filename.indexOf("..") >= 0 || filename.indexOf("/") >= 0) {
        Serial.println("Удаление отклонено: недопустимое имя");
        return false;
    }

    String path = "/" + filename;
    if (!SD.exists(path)) {
        Serial.printf("Файл не найден: %s\n", path.c_str());
        return false;
    }

    if (SD.remove(path)) {
        Serial.printf("Удалено: %s\n", path.c_str());
        return true;
    }
    Serial.printf("Ошибка удаления: %s\n", path.c_str());
    return false;
}