#pragma once
// =============================================================================
// GRBL Communication Layer
// Handles serial I/O, command queuing, status parsing, and streaming protocol.
// =============================================================================

#include <Arduino.h>
#include "config.h"

// GRBL machine states
enum class GrblState : uint8_t {
    Unknown,
    Idle,
    Run,
    Hold,
    Jog,
    Alarm,
    Door,
    Check,
    Home,
    Sleep
};

// Parsed GRBL status
struct GrblStatus {
    GrblState state = GrblState::Unknown;
    float wposX = 0, wposY = 0, wposZ = 0;   // Work position
    float mposX = 0, mposY = 0, mposZ = 0;   // Machine position
    int feedRate = 0;
    int spindleSpeed = 0;
    int bufferAvail = GRBL_RX_BUFFER;         // Planner buffer available
    int rxAvail = GRBL_RX_BUFFER;             // RX buffer available
    bool overrides = false;
    int feedOverride = 100;
    int rapidOverride = 100;
    int spindleOverride = 100;
};

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

private:
    void processIncoming();
    void parseLine(const char* line);
    void parseStatus(const char* line);
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
};

extern GrblComm grbl;

