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

bool sdIsReady() { return sdReady; }

String sdFreeSpace() {
    if (!sdReady) return "SD not ready";
    uint64_t total = SD.totalBytes() / (1024 * 1024);
    uint64_t used  = SD.usedBytes()  / (1024 * 1024);
    char buf[64];
    snprintf(buf, sizeof(buf), "Free: %llu MB / Total: %llu MB",
             total - used, total);
    return String(buf);
}

bool sdIsValidPath(const String& path) {
    if (path.length() == 0) return false;
    if (path.indexOf("..") >= 0) return false;
    if (path.indexOf('\\') >= 0) return false;
    return true;
}

// Убирает трейлинг-слеш (кроме корня). "/tanks/" → "/tanks", "/" → "/"
static String trimSlash(const String& p) {
    if (p.length() > 1 && p.endsWith("/")) {
        return p.substring(0, p.length() - 1);
    }
    return p;
}

// Гарантирует слеш в конце (для ссылок). "/tanks" → "/tanks/", "/" → "/"
static String ensureSlash(const String& p) {
    if (!p.endsWith("/")) return p + "/";
    return p;
}

String sdListItems(const String& path) {
    if (!sdReady) return "<li>SD not ready</li>";

    String full = path.length() == 0 ? "/" : path;
    String openPath = trimSlash(full);        // для SD.open
    String base     = ensureSlash(full);      // для ссылок

    Serial.printf("sdListItems: open '%s', links '%s'\n",
                  openPath.c_str(), base.c_str());

    File dir = SD.open(openPath);
    if (!dir) {
        Serial.printf("sdListItems: SD.open('%s') failed\n", openPath.c_str());
        return "<li>Cannot open dir</li>";
    }
    if (!dir.isDirectory()) {
        Serial.printf("sdListItems: '%s' is not a directory\n", openPath.c_str());
        dir.close();
        return "<li>Not a directory</li>";
    }

    String folders = "";
    String files   = "";

    File f = dir.openNextFile();
    int count = 0;
    while (f && count < 100) {
        String name = String(f.name());
        int slash = name.lastIndexOf('/');
        if (slash >= 0) name = name.substring(slash + 1);

        if (f.isDirectory()) {
            folders += "<li>&#128193; <a href='/?dir=" + base + name + "/'>"
                     + name + "/</a>"
                     + " <a href='/delete?path=" + base + name
                     + "&from=" + base
                     + "' onclick='return confirm(\"Удалить папку?\")'>[X]</a></li>";
        } else {
            files += "<li>" + name + " (" + String(f.size()) + " b)"
                   + " <a href='/delete?path=" + base + name
                   + "&from=" + base
                   + "' onclick='return confirm(\"Удалить?\")'>[X]</a></li>";
        }
        f = dir.openNextFile();
        count++;
    }
    dir.close();

    String html = folders + files;
    if (html.length() == 0) html = "<li>(пусто)</li>";
    return html;
}

bool sdCreateDir(const String& fullPath) {
    if (!sdReady) {
        Serial.println("sdCreateDir: SD not ready");
        return false;
    }
    if (!sdIsValidPath(fullPath)) {
        Serial.printf("sdCreateDir: недопустимый путь '%s'\n", fullPath.c_str());
        return false;
    }

    String p = trimSlash(fullPath);

    if (SD.exists(p)) {
        Serial.printf("sdCreateDir: '%s' уже существует\n", p.c_str());
        return false;
    }

    bool ok = SD.mkdir(p);
    Serial.printf("sdCreateDir('%s'): %s\n", p.c_str(), ok ? "OK" : "FAIL");
    return ok;
}

bool sdDeletePath(const String& fullPath) {
    if (!sdReady) return false;
    if (!sdIsValidPath(fullPath)) return false;

    String p = trimSlash(fullPath);
    if (!SD.exists(p)) {
        Serial.printf("sdDeletePath: '%s' не найден\n", p.c_str());
        return false;
    }
    bool ok = SD.remove(p);
    Serial.printf("sdDeletePath('%s'): %s\n", p.c_str(), ok ? "OK" : "FAIL");
    return ok;
}