#pragma once
// =============================================================================
// SD Card Manager — file listing, reading, deletion for GCode files
// =============================================================================

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "config.h"

#define MAX_FILES 64
#define MAX_FILENAME 64

struct FileInfo {
    char name[MAX_FILENAME];
    size_t size;
};

class SDManager {
public:
    bool begin();
    bool isReady() const { return _ready; }

    // Directory listing (root or subdir)
    int listGCodeFiles(const char* dir, FileInfo* out, int maxFiles);

    // File operations
    bool exists(const char* path);
    bool remove(const char* path);
    size_t fileSize(const char* path);

    // Streaming read
    File openFile(const char* path);

    // Total / free space
    uint64_t totalBytes();
    uint64_t usedBytes();

private:
    bool _ready = false;
    SPIClass _spi = SPIClass(HSPI);
};

extern SDManager sdCard;

