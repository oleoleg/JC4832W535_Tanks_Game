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

bool uploadSuccess = false;

// --- Утилиты путей ---

static String normalizePath(const String& p) {
    String r = p;
    if (r.length() == 0) r = "/";
    if (!r.startsWith("/")) r = "/" + r;
    if (!r.endsWith("/")) r += "/";
    while (r.indexOf("//") >= 0) r.replace("//", "/");
    return r;
}

static String parentPath(const String& p) {
    String path = normalizePath(p);
    if (path == "/") return "/";
    String t = path.substring(0, path.length() - 1);
    int s = t.lastIndexOf('/');
    if (s <= 0) return "/";
    return t.substring(0, s + 1);
}

static String breadcrumbs(const String& p) {
    String path = normalizePath(p);
    String html = "<a href='/?dir=/'>root</a>";
    if (path == "/") return html;

    String acc = "/";
    int start = 1;
    for (int i = 1; i <= (int)path.length(); i++) {
        if (i == path.length() || path[i] == '/') {
            String seg = path.substring(start, i);
            if (seg.length() > 0) {
                acc += seg + "/";
                html += " / <a href='/?dir=" + acc + "'>" + seg + "</a>";
            }
            start = i + 1;
        }
    }
    return html;
}

// --- Загрузка ---

void handleUpload(AsyncWebServerRequest *request, String filename,
                  size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
        uploadSuccess = false;

        if (!sdIsReady()) {
            Serial.println("Загрузка: SD not ready");
            return;
        }
        if (filename.indexOf("..") >= 0 || filename.indexOf('/') >= 0) {
            Serial.println("Загрузка: недопустимое имя");
            return;
        }

        String dir = "/";
        if (request->hasParam("dir", false))
            dir = request->getParam("dir", false)->value();
        dir = normalizePath(dir);

        String full = dir + filename;
        Serial.printf("Загрузка: %s\n", full.c_str());
        uploadFile = SD.open(full, FILE_WRITE);
        if (!uploadFile) {
            Serial.println("Загрузка: не удалось открыть файл");
            return;
        }
    }
    if (uploadFile) uploadFile.write(data, len);
    if (final && uploadFile) {
        uploadFile.close();
        uploadSuccess = true;
        Serial.println("Загрузка: OK");
    }
}

void handleUploadComplete(AsyncWebServerRequest *request) {
    if (uploadSuccess) request->send(200, "text/plain", "OK");
    else               request->send(400, "text/plain", "Upload failed");
    uploadSuccess = false;
}

// --- Wi-Fi ---

bool wifiConnect() {
    wm.setConfigPortalTimeout(180);
    wm.setDebugOutput(true);
    Serial.println("WiFi: подключение...");
    if (!wm.autoConnect("Tanks-Setup", "12345678")) {
        Serial.println("WiFi: не удалось подключиться");
        return false;
    }
    Serial.print("WiFi: подключено, IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

// --- HTML ---

static const char INDEX_HEAD[] =
"<!DOCTYPE html><html><head><meta charset='utf-8'>"
"<title>Tanks SD</title><style>"
"body{font-family:sans-serif;margin:20px}"
"#status{margin:10px 0;font-weight:bold}"
"#status.ok{color:green}#status.err{color:red}#status.load{color:orange}"
"ul{list-style:none;padding-left:0}"
"li{padding:4px 0;border-bottom:1px solid #eee}"
"a{text-decoration:none;color:#06c}"
".path{background:#eef;padding:8px;border-radius:4px;margin:8px 0}"
"form{margin:8px 0}input[type=text]{padding:4px}button{padding:5px 12px}"
"</style></head><body><h1>SD Card</h1>";

void wifiStartServer() {

    // === Главная ===
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String dir = "/";
        if (request->hasParam("dir"))
            dir = request->getParam("dir")->value();
        dir = normalizePath(dir);

        String html = INDEX_HEAD;
        html += "<p><b>" + sdFreeSpace() + "</b></p>";

        html += "<div class='path'>Path: " + breadcrumbs(dir);
        if (dir != "/")
            html += " &nbsp; <a href='/?dir=" + parentPath(dir) + "'>&#8593; Up</a>";
        html += "</div>";

        html += "<h2>Files</h2><ul id='fileList'>";
        html += sdListItems(dir);
        html += "</ul>";

        html += "<h2>New folder</h2>";
        html += "<form action='/mkdir' method='GET'>";
        html += "<input type='hidden' name='dir' value='" + dir + "'>";
        html += "<input type='text' name='name' placeholder='folder name' required>";
        html += "<button type='submit'>Create</button></form>";

        html += "<h2>Upload</h2>";
        html += "<form id='uploadForm'>";
        html += "<input type='file' id='fileInput'>";
        html += "<button type='submit'>Upload</button></form>";
        html += "<div id='status'></div>";

        html += "<script>const CURRENT_DIR='" + dir + "';";
        html += R"JS(
document.getElementById('uploadForm').addEventListener('submit',function(e){
  e.preventDefault();
  const st=document.getElementById('status');
  const inp=document.getElementById('fileInput');
  if(!inp.files.length){st.className='err';st.textContent='Файл не выбран';return;}
  const fd=new FormData();fd.append('data',inp.files[0]);
  const x=new XMLHttpRequest();
  x.upload.onprogress=function(e){
    if(e.lengthComputable){const p=Math.round(e.loaded/e.total*100);
      st.className='load';st.textContent='Загрузка: '+p+'%';}
  };
  x.onload=function(){
    if(x.status===200){st.className='ok';st.textContent='Файл загружен';inp.value='';
      fetch('/list?dir='+encodeURIComponent(CURRENT_DIR))
        .then(r=>r.text()).then(h=>document.getElementById('fileList').innerHTML=h);
    }else{st.className='err';st.textContent='Ошибка: '+x.responseText;}
  };
  x.onerror=function(){st.className='err';st.textContent='Ошибка сети';};
  x.open('POST','/upload?dir='+encodeURIComponent(CURRENT_DIR));
  x.send(fd);
});
)JS";
        html += "</script></body></html>";
        request->send(200, "text/html", html);
    });

    // === AJAX список ===
    server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        String dir = "/";
        if (request->hasParam("dir"))
            dir = request->getParam("dir")->value();
        request->send(200, "text/html", sdListItems(normalizePath(dir)));
    });

    // === Создание папки ===
    server.on("/mkdir", HTTP_GET, [](AsyncWebServerRequest *request) {
        String dir = "/";
        if (request->hasParam("dir"))
            dir = request->getParam("dir")->value();
        dir = normalizePath(dir);

        String name = "";
        if (request->hasParam("name"))
            name = request->getParam("name")->value();
        name.trim();

        if (name.length() == 0 || name.indexOf('/') >= 0 ||
            name.indexOf('\\') >= 0 || name.indexOf("..") >= 0) {
            request->send(400, "text/plain", "Invalid name");
            return;
        }

        String full = dir + name;
        Serial.printf("mkdir: %s\n", full.c_str());
        sdCreateDir(full);
        request->redirect("/?dir=" + dir);
    });

    // === Удаление ===
    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
        String path = "";
        if (request->hasParam("path"))
            path = request->getParam("path")->value();

        String from = "/";
        if (request->hasParam("from"))
            from = request->getParam("from")->value();
        from = normalizePath(from);

        Serial.printf("delete: %s\n", path.c_str());
        sdDeletePath(path);
        request->redirect("/?dir=" + from);
    });

    // === Загрузка ===
    server.on("/upload", HTTP_POST, handleUploadComplete, handleUpload);

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(204);
    });

    server.begin();
    Serial.println("Веб-сервер запущен");
}

void wifiLoop() { wm.process(); }