#pragma once
// =============================================================================
// Web Server — WiFi STA + async web server for GCode file upload
// =============================================================================

#include <Arduino.h>

class WebUploadServer {
public:
    void begin();
    void loop();   // Monitors WiFi connection, reconnects if needed
    String getIP() const;
    bool isRunning() const { return _running; }
    bool isConnected() const;

private:
    bool connectWiFi();

    bool _running = false;
    unsigned long _lastReconnectAttempt = 0;
};

extern WebUploadServer webServer;

