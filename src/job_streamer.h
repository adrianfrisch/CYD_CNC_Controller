#pragma once
// =============================================================================
// GCode Job Streamer — reads file, streams to GRBL with flow control
// =============================================================================

#include <Arduino.h>
#include <SD.h>
#include "config.h"
#include "grbl_comm.h"
#include "sd_manager.h"

enum class JobState : uint8_t {
    Idle,
    Running,
    Paused,
    Completed,
    Error
};

class JobStreamer {
public:
    void begin();
    void loop();  // Call every main loop iteration

    bool startJob(const char* filePath);
    void pause();
    void resume();
    void stop();

    // Status
    JobState state() const { return _state; }
    const char* fileName() const { return _fileName; }
    size_t fileSize() const { return _fileSize; }
    size_t bytesRead() const { return _bytesRead; }
    int percentComplete() const;
    unsigned long elapsedMs() const;
    unsigned long estimatedRemainingMs() const;
    int totalLines() const { return _totalLines; }
    int currentLine() const { return _currentLine; }
    const char* currentGCodeLine() const { return _currentCmd; }

    // Callbacks
    using JobDoneCb = void (*)(bool success);
    void onJobDone(JobDoneCb cb) { _doneCb = cb; }

private:
    bool readNextLine();
    void stripComments(char* line);

    JobState _state = JobState::Idle;
    File     _file;
    char     _fileName[MAX_FILENAME] = {};
    size_t   _fileSize = 0;
    size_t   _bytesRead = 0;
    int      _totalLines = 0;
    int      _currentLine = 0;
    char     _currentCmd[GCODE_LINE_MAX] = {};

    unsigned long _startTime = 0;
    int      _pendingOks = 0;

    JobDoneCb _doneCb = nullptr;
};

extern JobStreamer job;


