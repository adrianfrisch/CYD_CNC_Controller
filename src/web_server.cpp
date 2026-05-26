// =============================================================================
// Web Server — Implementation
// =============================================================================

#include "web_server.h"
#include "config.h"
#include "sd_manager.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SD.h>

WebUploadServer webServer;

static AsyncWebServer server(80);

// Embedded HTML page for file upload
static const char UPLOAD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>CYD CNC - File Upload</title>
<style>
body { font-family: Arial, sans-serif; background: #1a1a2e; color: #eee; max-width: 600px; margin: 40px auto; padding: 20px; }
h1 { color: #e94560; }
.upload-area { border: 2px dashed #555; border-radius: 12px; padding: 40px; text-align: center; margin: 20px 0; }
.upload-area.dragover { border-color: #e94560; background: #16213e; }
input[type=file] { margin: 10px 0; }
button { background: #e94560; color: white; border: none; padding: 12px 24px; border-radius: 6px; cursor: pointer; font-size: 16px; }
button:hover { background: #c73a52; }
.progress { width: 100%; height: 20px; background: #333; border-radius: 10px; margin: 10px 0; display: none; }
.progress-bar { height: 100%; background: #0f3460; border-radius: 10px; transition: width 0.3s; }
.files { margin-top: 20px; }
.file-item { background: #16213e; padding: 10px; margin: 5px 0; border-radius: 6px; display: flex; justify-content: space-between; }
.msg { padding: 10px; margin: 10px 0; border-radius: 6px; display: none; }
.msg.ok { background: #1b4332; display: block; }
.msg.err { background: #641220; display: block; }
</style>
</head>
<body>
<h1>&#9881; CYD CNC Controller</h1>
<h2>Upload GCode File</h2>
<div class="upload-area" id="dropzone">
  <p>Drag & drop GCode file here or click below</p>
  <input type="file" id="fileInput" accept=".nc,.gcode,.gc,.ngc,.tap,.cnc,.txt">
  <br><br>
  <button onclick="uploadFile()">Upload</button>
</div>
<div class="progress" id="progress">
  <div class="progress-bar" id="progressBar" style="width:0%"></div>
</div>
<div class="msg" id="msg"></div>
<h3>Files on SD Card</h3>
<div class="files" id="fileList">Loading...</div>
<script>
const dz = document.getElementById('dropzone');
dz.addEventListener('dragover', e => { e.preventDefault(); dz.classList.add('dragover'); });
dz.addEventListener('dragleave', () => dz.classList.remove('dragover'));
dz.addEventListener('drop', e => { e.preventDefault(); dz.classList.remove('dragover');
  document.getElementById('fileInput').files = e.dataTransfer.files; });

function uploadFile() {
  const f = document.getElementById('fileInput').files[0];
  if (!f) { showMsg('Select a file first', true); return; }
  const xhr = new XMLHttpRequest();
  const fd = new FormData();
  fd.append('file', f);
  const pb = document.getElementById('progress');
  const bar = document.getElementById('progressBar');
  pb.style.display = 'block';
  xhr.upload.onprogress = e => { if(e.lengthComputable) bar.style.width = (e.loaded/e.total*100)+'%'; };
  xhr.onload = () => { pb.style.display='none'; showMsg(xhr.responseText, xhr.status!==200); loadFiles(); };
  xhr.onerror = () => { pb.style.display='none'; showMsg('Upload failed', true); };
  xhr.open('POST', '/upload');
  xhr.send(fd);
}

function showMsg(t, err) {
  const m = document.getElementById('msg');
  m.textContent = t; m.className = 'msg ' + (err?'err':'ok');
}

function loadFiles() {
  fetch('/files').then(r=>r.json()).then(files => {
    const el = document.getElementById('fileList');
    if(!files.length) { el.innerHTML='<p>No files</p>'; return; }
    el.innerHTML = files.map(f =>
      `<div class="file-item"><span>${f.name} (${(f.size/1024).toFixed(1)} KB)</span><button onclick="delFile('${f.name}')">Delete</button></div>`
    ).join('');
  });
}

function delFile(name) {
  if(!confirm('Delete '+name+'?')) return;
  fetch('/delete?name='+encodeURIComponent(name), {method:'DELETE'}).then(()=>loadFiles());
}

loadFiles();
</script>
</body>
</html>
)rawliteral";

void WebUploadServer::begin() {
    // Connect to WiFi network in station mode
    if (connectWiFi()) {
        Serial.printf("[WEB] Connected to %s  IP: %s\n", WIFI_SSID, WiFi.localIP().toString().c_str());
    } else {
        Serial.printf("[WEB] Failed to connect to %s — web upload unavailable\n", WIFI_SSID);
        Serial.println("[WEB] Will retry in background...");
    }

    // Serve upload page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", UPLOAD_HTML);
    });

    // File listing (JSON)
    server.on("/files", HTTP_GET, [](AsyncWebServerRequest* req) {
        String json = "[";
        FileInfo files[MAX_FILES];
        int count = sdCard.listGCodeFiles("/", files, MAX_FILES);
        for (int i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += "{\"name\":\"" + String(files[i].name) + "\",\"size\":" + String(files[i].size) + "}";
        }
        json += "]";
        req->send(200, "application/json", json);
    });

    // File upload
    server.on("/upload", HTTP_POST,
        // On request complete
        [](AsyncWebServerRequest* req) {
            req->send(200, "text/plain", "Upload complete");
        },
        // On file upload
        [](AsyncWebServerRequest* req, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
            static File uploadFile;
            if (index == 0) {
                String path = "/" + filename;
                Serial.printf("[WEB] Upload start: %s\n", path.c_str());
                uploadFile = SD.open(path, FILE_WRITE);
            }
            if (uploadFile && len > 0) {
                uploadFile.write(data, len);
            }
            if (final) {
                if (uploadFile) {
                    uploadFile.close();
                    Serial.printf("[WEB] Upload done: %s (%u bytes)\n", filename.c_str(), index + len);
                }
            }
        }
    );

    // File deletion
    server.on("/delete", HTTP_DELETE, [](AsyncWebServerRequest* req) {
        if (req->hasParam("name")) {
            String name = "/" + req->getParam("name")->value();
            if (SD.exists(name)) {
                SD.remove(name);
                req->send(200, "text/plain", "Deleted");
            } else {
                req->send(404, "text/plain", "Not found");
            }
        } else {
            req->send(400, "text/plain", "Missing name");
        }
    });

    server.begin();
    _running = true;
    Serial.println("[WEB] Server started on port 80");
}

void WebUploadServer::loop() {
    // Reconnect WiFi if connection was lost
    if (!isConnected() && (millis() - _lastReconnectAttempt > 30000)) {
        Serial.println("[WEB] WiFi disconnected, attempting reconnect...");
        connectWiFi();
    }
}

bool WebUploadServer::connectWiFi() {
    _lastReconnectAttempt = millis();
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("[WEB] Connecting to WiFi '%s'...", WIFI_SSID);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    return WiFi.status() == WL_CONNECTED;
}

bool WebUploadServer::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WebUploadServer::getIP() const {
    return WiFi.localIP().toString();
}


