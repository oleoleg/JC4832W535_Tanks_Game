//точка доступа, сервер, загрузка файлов

#include "wifi_manager.h"
#include "config.h"
#include "sd_manager.h"   
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <SD.h>      

WiFiManager wm;
AsyncWebServer server(80);
File uploadFile;
// Глобальный флаг
bool uploadSuccess = false;
bool uploadStarted = false;

// Обработчик загрузки файла
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
   if (!index) {
        uploadStarted = true;
        uploadSuccess = false;
        if (!sdIsReady()) {
            Serial.println("Загрузка: SD not ready");
            return;
        }
        uploadFile = SD.open("/" + filename, FILE_WRITE);
        if (!uploadFile) {
            Serial.println("Загрузка: не удалось открыть файл");
            return;
        }
        Serial.printf("Загрузка: %s\n", filename.c_str());
    }

    if (uploadFile) {
        uploadFile.write(data, len);
    }

    if (final && uploadFile) {
        uploadFile.close();
        uploadSuccess = true;
        Serial.printf("Готово: %s\n", filename.c_str());
    }
}


// Обработчик ответа — вызывается после приёма тела
void handleUploadComplete(AsyncWebServerRequest *request) {
    if (!uploadStarted || !uploadSuccess) {
        request->send(500, "text/plain", "Upload failed"); 
    } 
    else {
        request->send(200, "text/plain", "OK");
    }
       
    uploadStarted = false;   // сбрасываем для следующего запроса
    uploadSuccess = false;
}

bool wifiConnect() {
    // Настройки портала
    wm.setConfigPortalTimeout(180);   // 3 минуты на настройку
    wm.setDebugOutput(true);

    // Пытаемся подключиться к сохранённой сети.
    // Если не получается — поднимаем точку доступа "Tanks-Setup"
    Serial.println("WiFi: подключение...");
    if (!wm.autoConnect("Tanks-Setup", "12345678")) {
        Serial.println("WiFi: не удалось подключиться, таймаут портала");
        return false;
    }

    Serial.print("WiFi: подключено, IP: ");
    Serial.println(WiFi.localIP());
    return true;
}


void wifiStartServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"HTML(<!DOCTYPE html>
<html><head><meta charset='utf-8'>
<title>Tanks SD</title>
<style>
body { font-family: sans-serif; margin: 20px; }
#status { margin: 10px 0; font-weight: bold; }
#status.ok { color: green; }
#status.err { color: red; }
#status.load { color: orange; }
</style>
</head><body>
<h1>SD Card</h1>
<p><b>)HTML";

        html += sdFreeSpace();
        html += R"HTML(</b></p>

<h2>Files</h2>
<ul id="fileList">)HTML";
        html += sdListFiles();
        html += R"HTML(</ul>

<hr>
<h2>Upload</h2>
<form id="uploadForm">
  <input type="file" name="data" id="fileInput">
  <button type="submit">Upload</button>
</form>
<div id="status"></div>

<script>
document.getElementById('uploadForm').addEventListener('submit', function(e) {
    e.preventDefault();

    const status = document.getElementById('status');
    const input = document.getElementById('fileInput');

    if (!input.files.length) {
        status.className = 'err';
        status.textContent = 'Файл не выбран';
        return;
    }

    const formData = new FormData();
    formData.append('data', input.files[0]);

    const xhr = new XMLHttpRequest();

    xhr.upload.onprogress = function(e) {
        if (e.lengthComputable) {
            const percent = Math.round((e.loaded / e.total) * 100);
            status.className = 'load';
            status.textContent = 'Загрузка: ' + percent + '%';
        }
    };

    xhr.onload = function() {
        if (xhr.status === 200) {
            status.className = 'ok';
            status.textContent = 'Файл загружен';
            input.value = '';

            fetch('/list')
                .then(r => r.text())
                .then(html => {
                    document.getElementById('fileList').innerHTML = html;
                });
        } else {
            status.className = 'err';
            status.textContent = 'Ошибка: ' + xhr.responseText;
        }
    };

    xhr.onerror = function() {
        status.className = 'err';
        status.textContent = 'Ошибка сети';
    };

    xhr.open('POST', '/upload');
    xhr.send(formData);
});
</script>
</body></html>)HTML";

        request->send(200, "text/html", html);
    });

    // Обновление списка файлов (AJAX)
    server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", sdListFiles());
    });

    // Обработка загрузки
    server.on("/upload", HTTP_POST, handleUploadComplete, handleUpload);

    // Удаление файла
    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("file")) {
            request->send(400, "text/plain", "No filename");
            return;
        }
        String filename = request->getParam("file")->value();
        if (sdDeleteFile(filename)) {
            request->redirect("/");
        } else {
            request->send(500, "text/plain", "Delete failed");
        }
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(204);
    });

    server.begin();
    Serial.println("Веб-сервер запущен на порту 80");
}


void wifiLoop() {
    wm.process();   // нужно, если используется non-blocking режим
}

