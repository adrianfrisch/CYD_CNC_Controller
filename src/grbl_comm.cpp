// =============================================================================
// GRBL Communication Layer — Implementation
// =============================================================================

#include "grbl_comm.h"
#include <cstring>
#include <cstdio>

GrblComm grbl;

void GrblComm::begin() {
    GRBL_SERIAL.begin(GRBL_BAUD_RATE, SERIAL_8N1, GRBL_RX_PIN, GRBL_TX_PIN);
    _rxPos = 0;
    _grblBufFree = GRBL_RX_BUFFER;
    _sentHead = _sentTail = _sentCount = 0;
    _connected = false;
    Serial.println("[GRBL] UART2 initialized");
}

void GrblComm::loop() {
    processIncoming();

    // Auto-poll status
    unsigned long now = millis();
    if (now - _lastStatusReq >= STATUS_POLL_MS) {
        requestStatus();
        _lastStatusReq = now;
    }

    // Connection timeout
    if (_connected && (now - _lastRx > 3000)) {
        _connected = false;
        _status.state = GrblState::Unknown;
        Serial.println("[GRBL] Connection lost");
    }
}

// ---------------------------------------------------------------------------
// Outgoing commands
// ---------------------------------------------------------------------------

void GrblComm::sendLine(const char* cmd) {
    if (!cmd || cmd[0] == '\0') return;
    int len = strlen(cmd) + 1; // +1 for \n

    // Character-counting: track buffer usage
    if (_sentCount < GCODE_BUFFER_LINES && _grblBufFree >= len) {
        GRBL_SERIAL.print(cmd);
        GRBL_SERIAL.print('\n');
        _sentLens[_sentHead] = len;
        _sentHead = (_sentHead + 1) % GCODE_BUFFER_LINES;
        _sentCount++;
        _grblBufFree -= len;
        Serial.printf("[GRBL] >> %s (buf free: %d)\n", cmd, _grblBufFree);
    }
}

void GrblComm::sendRealtime(char c) {
    GRBL_SERIAL.write(c);
}

void GrblComm::softReset()    { sendRealtime(0x18); _grblBufFree = GRBL_RX_BUFFER; _sentCount = 0; _sentHead = _sentTail = 0; }
void GrblComm::feedHold()     { sendRealtime('!'); }
void GrblComm::cycleResume()  { sendRealtime('~'); }
void GrblComm::requestStatus(){ sendRealtime('?'); }
void GrblComm::homeMachine()  { sendLine("$H"); }
void GrblComm::unlockAlarm()  { sendLine("$X"); }
void GrblComm::jogCancel()    { sendRealtime(0x85); }

void GrblComm::jog(float x, float y, float z, float feed) {
    char buf[80];
    snprintf(buf, sizeof(buf), "$J=G91 X%.3f Y%.3f Z%.3f F%.0f", x, y, z, feed);
    sendLine(buf);
}

void GrblComm::setZero() {
    sendLine("G10 L20 P1 X0 Y0 Z0");
}

void GrblComm::setZeroZ() {
    sendLine("G10 L20 P1 Z0");
}

void GrblComm::goToZero() {
    sendLine("G90 G0 X0 Y0 Z0");
}

// ---------------------------------------------------------------------------
// Incoming data
// ---------------------------------------------------------------------------

void GrblComm::processIncoming() {
    while (GRBL_SERIAL.available()) {
        char c = GRBL_SERIAL.read();
        _lastRx = millis();
        _connected = true;

        if (c == '\n' || c == '\r') {
            if (_rxPos > 0) {
                _rxBuf[_rxPos] = '\0';
                parseLine(_rxBuf);
                _rxPos = 0;
            }
        } else if (_rxPos < sizeof(_rxBuf) - 1) {
            _rxBuf[_rxPos++] = c;
        }
    }
}

void GrblComm::parseLine(const char* line) {
    Serial.printf("[GRBL] << %s\n", line);

    if (line[0] == '<') {
        // Status report
        parseStatus(line);
        return;
    }

    if (strcmp(line, "ok") == 0) {
        // Free buffer space for the oldest unacknowledged line
        if (_sentCount > 0) {
            _grblBufFree += _sentLens[_sentTail];
            _sentTail = (_sentTail + 1) % GCODE_BUFFER_LINES;
            _sentCount--;
        }
    } else if (strncmp(line, "error:", 6) == 0) {
        // Error — also frees buffer
        if (_sentCount > 0) {
            _grblBufFree += _sentLens[_sentTail];
            _sentTail = (_sentTail + 1) % GCODE_BUFFER_LINES;
            _sentCount--;
        }
        Serial.printf("[GRBL] ERROR: %s\n", line);
    } else if (strncmp(line, "ALARM:", 6) == 0) {
        _status.state = GrblState::Alarm;
        Serial.printf("[GRBL] ALARM: %s\n", line);
    } else if (strncmp(line, "Grbl ", 5) == 0) {
        // GRBL startup message — reset buffer tracking
        _grblBufFree = GRBL_RX_BUFFER;
        _sentCount = 0;
        _sentHead = _sentTail = 0;
        Serial.printf("[GRBL] Controller connected: %s\n", line);
    }

    if (_responseCb) _responseCb(line);
}

// Parse: <Idle|WPos:0.000,0.000,0.000|Bf:15,128|FS:0,0|Ov:100,100,100>
void GrblComm::parseStatus(const char* line) {
    char buf[256];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    // Remove < and >
    char* p = buf;
    if (*p == '<') p++;
    char* end = strrchr(p, '>');
    if (end) *end = '\0';

    // First token is state
    char* tok = strtok(p, "|");
    if (tok) {
        _status.state = parseState(tok);
    }

    // Remaining tokens
    while ((tok = strtok(nullptr, "|")) != nullptr) {
        if (strncmp(tok, "WPos:", 5) == 0) {
            sscanf(tok + 5, "%f,%f,%f", &_status.wposX, &_status.wposY, &_status.wposZ);
        } else if (strncmp(tok, "MPos:", 5) == 0) {
            sscanf(tok + 5, "%f,%f,%f", &_status.mposX, &_status.mposY, &_status.mposZ);
            // If WPos not reported, copy MPos (simplified)
            _status.wposX = _status.mposX;
            _status.wposY = _status.mposY;
            _status.wposZ = _status.mposZ;
        } else if (strncmp(tok, "Bf:", 3) == 0) {
            sscanf(tok + 3, "%d,%d", &_status.bufferAvail, &_status.rxAvail);
        } else if (strncmp(tok, "FS:", 3) == 0) {
            sscanf(tok + 3, "%d,%d", &_status.feedRate, &_status.spindleSpeed);
        } else if (strncmp(tok, "F:", 2) == 0) {
            sscanf(tok + 2, "%d", &_status.feedRate);
        } else if (strncmp(tok, "Ov:", 3) == 0) {
            sscanf(tok + 3, "%d,%d,%d", &_status.feedOverride,
                   &_status.rapidOverride, &_status.spindleOverride);
            _status.overrides = true;
        }
    }

    if (_statusCb) _statusCb(_status);
}

GrblState GrblComm::parseState(const char* s) {
    if (strcmp(s, "Idle")  == 0) return GrblState::Idle;
    if (strcmp(s, "Run")   == 0) return GrblState::Run;
    if (strcmp(s, "Hold")  == 0 || strncmp(s, "Hold:", 5) == 0) return GrblState::Hold;
    if (strcmp(s, "Jog")   == 0) return GrblState::Jog;
    if (strcmp(s, "Alarm") == 0) return GrblState::Alarm;
    if (strcmp(s, "Door")  == 0 || strncmp(s, "Door:", 5) == 0) return GrblState::Door;
    if (strcmp(s, "Check") == 0) return GrblState::Check;
    if (strcmp(s, "Home")  == 0) return GrblState::Home;
    if (strcmp(s, "Sleep") == 0) return GrblState::Sleep;
    return GrblState::Unknown;
}

