// =============================================================================
// GRBL Communication Layer — Implementation
// =============================================================================

#include "grbl_comm.h"
#include "machine_config.h"
#include <cstring>
#include <cstdio>

GrblComm grbl;

// In DEBUG_SERIAL_GRBL mode, GRBL communication goes over USB Serial
// instead of UART2. Debug log lines are prefixed with '[' so the
// simulator can ignore them.
#if DEBUG_SERIAL_GRBL
  #define GRBL_PORT Serial
#else
  #define GRBL_PORT GRBL_SERIAL
#endif

void GrblComm::begin() {
#if DEVELOP_MODE && !DEBUG_SERIAL_GRBL
    // Pure develop mode — no serial, simulated state
    _connected = true;
    _status.state = GrblState::Idle;
    DebugSerial.println("[GRBL] DEVELOP MODE — UART disabled, simulating Idle state");
#elif DEBUG_SERIAL_GRBL
    // Debug mode — GRBL comms over USB Serial (shared with debug output)
    // Serial is already initialized by Arduino framework
    _rxPos = 0;
    _grblBufFree = GRBL_RX_BUFFER;
    _sentHead = _sentTail = _sentCount = 0;
    _connected = false;
    DebugSerial.println("[GRBL] DEBUG MODE — using USB Serial for GRBL communication");
    DebugSerial.println("[GRBL] Connect grbl_simulator.py to this port");
#else
    GRBL_PORT.begin(GRBL_BAUD_RATE, SERIAL_8N1, GRBL_RX_PIN, GRBL_TX_PIN);
    _rxPos = 0;
    _grblBufFree = GRBL_RX_BUFFER;
    _sentHead = _sentTail = _sentCount = 0;
    _connected = false;
    DebugSerial.println("[GRBL] UART2 initialized");
#endif
}

void GrblComm::loop() {
#if DEVELOP_MODE && !DEBUG_SERIAL_GRBL
    // No serial processing — keep simulated state
    return;
#else
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
        DBG("GRBL connection lost");
    }
#endif
}

// ---------------------------------------------------------------------------
// Outgoing commands
// ---------------------------------------------------------------------------

void GrblComm::sendLine(const char* cmd) {
    if (!cmd || cmd[0] == '\0') return;
#if DEVELOP_MODE && !DEBUG_SERIAL_GRBL
    DBG("GRBL sendLine: %s", cmd);
#else
    int len = strlen(cmd) + 1; // +1 for \n

    // Character-counting: track buffer usage
    if (_sentCount < GCODE_BUFFER_LINES && _grblBufFree >= len) {
        GRBL_PORT.print(cmd);
        GRBL_PORT.print('\n');
        _sentLens[_sentHead] = len;
        _sentHead = (_sentHead + 1) % GCODE_BUFFER_LINES;
        _sentCount++;
        _grblBufFree -= len;
        DBG("GRBL >> %s (buf free: %d)", cmd, _grblBufFree);
    }
#endif
}

void GrblComm::sendRealtime(char c) {
#if DEVELOP_MODE && !DEBUG_SERIAL_GRBL
    DBG("GRBL realtime: 0x%02X", (int)c);
#else
    GRBL_PORT.write(c);
#endif
}

void GrblComm::softReset() {
    sendRealtime(0x18);
    // Clear character-counting state
    _grblBufFree = GRBL_RX_BUFFER;
    _sentCount = 0;
    _sentHead = _sentTail = 0;
    // Flush partial RX data — a half-received line would corrupt the
    // GRBL welcome message that follows the reset
    _rxPos = 0;
    // Mark disconnected; the GRBL welcome banner ("Grbl 1.1h ...")
    // arriving in parseLine() will set _connected = true again
    _connected = false;
    _status.state = GrblState::Unknown;
    // GRBL 1.1 enters ALARM state when soft-reset during motion.
    // Set flag so we send $X as soon as we see the "Grbl " welcome banner.
    _postResetUnlock = true;
    // Drain any stale bytes already in the serial hardware buffer
    flushInput();
}
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
    float clearance = machineConfig.clearanceHeight();
    if (clearance > 0.0f) {
        // Safe return-to-zero: raise Z to clearance height, move XY, lower Z
        char buf[64];
        snprintf(buf, sizeof(buf), "G90 G0 Z%.3f", clearance);
        sendLine(buf);
        sendLine("G90 G0 X0 Y0");
        sendLine("G90 G0 Z0");
    } else {
        sendLine("G90 G0 X0 Y0 Z0");
    }
}

// ---------------------------------------------------------------------------
// Serial buffer maintenance
// ---------------------------------------------------------------------------

void GrblComm::flushInput() {
#if !(DEVELOP_MODE && !DEBUG_SERIAL_GRBL)
    // Drain any bytes already sitting in the serial hardware FIFO.
    // After a soft-reset the controller may still be sending the tail
    // end of a previous response or status report.
    while (GRBL_PORT.available()) {
        GRBL_PORT.read();
    }
#endif
}

// ---------------------------------------------------------------------------
// Incoming data
// ---------------------------------------------------------------------------

void GrblComm::processIncoming() {
    while (GRBL_PORT.available()) {
        char c = GRBL_PORT.read();
        _lastRx = millis();
        _connected = true;

        if (c == '\n' || c == '\r') {
            if (_rxPos > 0) {
                _rxBuf[_rxPos] = '\0';
#if DEBUG_SERIAL_GRBL
                // In debug mode, ignore our own debug output echoed back
                // (lines starting with '[' are debug/log messages, not GRBL responses)
                if (_rxBuf[0] != '[') {
                    parseLine(_rxBuf);
                }
#else
                parseLine(_rxBuf);
#endif
                _rxPos = 0;
            }
        } else if (_rxPos < sizeof(_rxBuf) - 1) {
            _rxBuf[_rxPos++] = c;
        }
    }
}

void GrblComm::parseLine(const char* line) {
    DBG("GRBL << %s", line);

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
        DebugSerial.printf("[GRBL] ERROR: %s\n", line);
    } else if (strncmp(line, "ALARM:", 6) == 0) {
        _status.state = GrblState::Alarm;
        DBG("GRBL ALARM: %s", line);
    } else if (strncmp(line, "Grbl ", 5) == 0) {
        // GRBL startup message — reset buffer tracking and mark connected
        _grblBufFree = GRBL_RX_BUFFER;
        _sentCount = 0;
        _sentHead = _sentTail = 0;
        _connected = true;
        _status.state = GrblState::Unknown;  // will be updated by next status poll
        DBG("GRBL Controller connected: %s", line);
        // If this banner followed a soft-reset, GRBL may be in ALARM state
        // (GRBL 1.1 enters ALARM when reset during motion). $X exits the alarm
        // so jog and gcode work immediately without a manual unlock step.
        if (_postResetUnlock) {
            _postResetUnlock = false;
            sendLine("$X");
            DBG("GRBL post-reset alarm unlock sent ($X)");
        }
    }

    if (_responseCb) _responseCb(line);
}

// GRBL 1.1 status formats:
//   $10=0  → <Idle|WPos:x,y,z|Bf:n,m|FS:f,s>          (work position directly)
//   $10=1  → <Idle|MPos:x,y,z|Bf:n,m|FS:f,s|WCO:x,y,z> (machine pos + offset)
// WPos = MPos - WCO.  WCO is sent whenever the offset changes and periodically.
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

    bool hasMPos = false;
    bool hasWPos = false;

    // Remaining tokens
    while ((tok = strtok(nullptr, "|")) != nullptr) {
        if (strncmp(tok, "WPos:", 5) == 0) {
            sscanf(tok + 5, "%f,%f,%f", &_status.wposX, &_status.wposY, &_status.wposZ);
            hasWPos = true;
        } else if (strncmp(tok, "MPos:", 5) == 0) {
            sscanf(tok + 5, "%f,%f,%f", &_status.mposX, &_status.mposY, &_status.mposZ);
            hasMPos = true;
        } else if (strncmp(tok, "WCO:", 4) == 0) {
            // Work Coordinate Offset — store so we can derive WPos from MPos.
            // G10 L20 changes this offset; the next status report carries the new WCO.
            sscanf(tok + 4, "%f,%f,%f", &_wcoX, &_wcoY, &_wcoZ);
            DBG("GRBL WCO updated: %.3f, %.3f, %.3f", _wcoX, _wcoY, _wcoZ);
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

    // If GRBL reported MPos (not WPos), derive WPos = MPos - WCO.
    // This is the correct calculation for $10=1 mode (Arduino GRBL default).
    if (hasMPos && !hasWPos) {
        _status.wposX = _status.mposX - _wcoX;
        _status.wposY = _status.mposY - _wcoY;
        _status.wposZ = _status.mposZ - _wcoZ;
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

