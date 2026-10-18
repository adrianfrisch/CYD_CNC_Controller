#pragma once
// =============================================================================
// GRBL Communication Layer
// Handles serial I/O, command queuing, status parsing, and streaming protocol.
// =============================================================================

#include <Arduino.h>
#include "config.h"
#include "../lib/testable/testable_logic.h"

// Callback types
using GrblResponseCb = void (*)(const char* line);
using GrblStatusCb   = void (*)(const GrblStatus& status);

class GrblComm {
public:
    void begin();
    void loop();                       // Call from main loop

    // Commands
    void sendLine(const char* cmd);    // Queue a single GCode line
    void sendRealtime(char c);         // Send realtime char (!, ~, ?, 0x18)
    void softReset();                  // Ctrl-X
    void feedHold();                   // !
    void cycleResume();                // ~
    void requestStatus();              // ?
    void homeMachine();                // $H
    void unlockAlarm();                // $X
    void jogCancel();                  // 0x85

    void jog(float x, float y, float z, float feed);
    void setZero();                    // G10 L20 P1 X0 Y0 Z0
    void setZeroZ();                   // G10 L20 P1 Z0
    void goToZero();                   // G90 G0 X0 Y0 Z0

    // Status
    const GrblStatus& status() const { return _status; }
    bool isConnected() const { return _connected; }
    bool isIdle() const { return _status.state == GrblState::Idle; }

    // Callbacks
    void onResponse(GrblResponseCb cb) { _responseCb = cb; }
    void onStatus(GrblStatusCb cb)     { _statusCb = cb; }

    // Streaming support
    int  availableInBuffer() const { return _grblBufFree; }
    int  pendingLines() const { return _sentCount; }  // Sent, awaiting 'ok'
    int  lostAcks() const { return _lostAcks; }       // Acks recovered as lost (serial errors)

private:
    void processIncoming();
    void parseLine(const char* line);
    void parseStatus(const char* line);
    void flushInput();
    GrblState parseState(const char* s);

    GrblStatus    _status;
    GrblResponseCb _responseCb = nullptr;
    GrblStatusCb   _statusCb   = nullptr;

    char   _rxBuf[512];
    size_t _rxPos = 0;

    bool   _connected = false;
    unsigned long _lastStatusReq = 0;
    unsigned long _lastRx = 0;

    // Character-counting streaming protocol
    int    _grblBufFree = GRBL_RX_BUFFER;
    int    _sentLens[GCODE_BUFFER_LINES]; // lengths of sent lines awaiting 'ok'
    int    _sentHead = 0;
    int    _sentTail = 0;
    int    _sentCount = 0;
    bool   _postResetUnlock = false;  // auto-send $X after next "Grbl " banner

    // Lost-ack recovery: a corrupted "ok" would leak buffer space forever and
    // eventually block all sends. Idle + nothing sent recently → all lines done.
    unsigned long _lastSendMs = 0;
    unsigned long _lastStatusMs = 0;
    int    _lostAcks = 0;
    void   recoverLostAcks(unsigned long now);

    // Work Coordinate Offset — updated from WCO: field in status reports.
    // Used to derive WPos when GRBL reports MPos ($10=1, the Arduino default).
    // WPos = MPos - WCO
    float  _wcoX = 0, _wcoY = 0, _wcoZ = 0;
};

extern GrblComm grbl;

