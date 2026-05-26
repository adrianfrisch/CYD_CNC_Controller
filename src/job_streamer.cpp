// =============================================================================
// GCode Job Streamer — Implementation
// =============================================================================

#include "job_streamer.h"
#include "sd_manager.h"
#include <cstring>

JobStreamer job;

void JobStreamer::begin() {
    _state = JobState::Idle;
    // Register a response callback so we can track ok/error from GRBL
    grbl.onResponse([](const char* line) {
        if (strcmp(line, "ok") == 0) {
            job._waitingForOk = false;
        } else if (strncmp(line, "error:", 6) == 0) {
            job._waitingForOk = false;
            Serial.printf("[JOB] GRBL error on line %d: %s\n", job._currentLine, line);
        }
    });
}

void JobStreamer::loop() {
    if (_state != JobState::Running) return;

    // Send-and-wait: only send next line after GRBL replied to the previous one
    if (_waitingForOk) return;

    // If we have a command ready, send it
    if (_currentCmd[0] != '\0') {
        grbl.sendLine(_currentCmd);
        _waitingForOk = true;
        _cmdSentTime = millis();
        DBG("JOB sent line %d: %s", _currentLine, _currentCmd);
        _currentCmd[0] = '\0';
        return; // Wait for ok before doing anything else
    }

    // Read next line from file
    if (!readNextLine()) {
        // File finished
        _state = JobState::Completed;
        _file.close();
        Serial.println("[JOB] Completed");
        if (_doneCb) _doneCb(true);
        return;
    }
}

bool JobStreamer::startJob(const char* filePath) {
    if (_state == JobState::Running) {
        Serial.println("[JOB] Already running");
        return false;
    }

    _file = sdCard.openFile(filePath);
    if (!_file) {
        Serial.printf("[JOB] Failed to open: %s\n", filePath);
        return false;
    }

    strncpy(_fileName, filePath, MAX_FILENAME - 1);
    _fileSize = _file.size();
    _bytesRead = 0;
    _currentLine = 0;
    _waitingForOk = false;
    _currentCmd[0] = '\0';

    // No full-file scan — progress is byte-based (_bytesRead / _fileSize)

    _startTime = millis();
    _state = JobState::Running;

    // Read first line
    readNextLine();

    Serial.printf("[JOB] Started: %s (%d bytes)\n", filePath, _fileSize);
    return true;
}

void JobStreamer::pause() {
    if (_state == JobState::Running) {
        grbl.feedHold();
        _state = JobState::Paused;
        Serial.println("[JOB] Paused");
    }
}

void JobStreamer::resume() {
    if (_state == JobState::Paused) {
        grbl.cycleResume();
        _state = JobState::Running;
        Serial.println("[JOB] Resumed");
    }
}

void JobStreamer::stop() {
    if (_state == JobState::Running || _state == JobState::Paused) {
        grbl.softReset();
        _state = JobState::Idle;
        _waitingForOk = false;
        _file.close();
        Serial.println("[JOB] Stopped");
        if (_doneCb) _doneCb(false);
    }
}

int JobStreamer::percentComplete() const {
    if (_fileSize == 0) return 0;
    return (int)((_bytesRead * 100UL) / _fileSize);
}

unsigned long JobStreamer::elapsedMs() const {
    if (_state == JobState::Idle) return 0;
    return millis() - _startTime;
}

unsigned long JobStreamer::estimatedRemainingMs() const {
    int pct = percentComplete();
    if (pct <= 0) return 0;
    unsigned long elapsed = elapsedMs();
    return (elapsed * (100 - pct)) / pct;
}

bool JobStreamer::readNextLine() {
    if (!_file || !_file.available()) return false;

    int i = 0;
    while (_file.available() && i < GCODE_LINE_MAX - 1) {
        char c = _file.read();
        _bytesRead++;
        if (c == '\n') break;
        if (c == '\r') continue;
        _currentCmd[i++] = c;
    }
    _currentCmd[i] = '\0';
    _currentLine++;

    // Strip comments and whitespace
    stripComments(_currentCmd);

    // Skip empty lines — recurse to next
    if (_currentCmd[0] == '\0') {
        return readNextLine();
    }

    return true;
}

void JobStreamer::stripComments(char* line) {
    // Remove parenthetical comments: (...)
    char* p = strchr(line, '(');
    if (p) *p = '\0';

    // Remove semicolon comments: ;...
    p = strchr(line, ';');
    if (p) *p = '\0';

    // Trim trailing whitespace
    int len = strlen(line);
    while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t')) {
        line[--len] = '\0';
    }

    // Trim leading whitespace
    p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (p != line) memmove(line, p, strlen(p) + 1);
}

