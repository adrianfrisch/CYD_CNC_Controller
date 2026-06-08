// =============================================================================
// SD Card Manager — Implementation
// =============================================================================

#include "sd_manager.h"

SDManager sdCard;

bool SDManager::begin() {
    Serial.printf("[SD] Init SPI: SCK=%d MISO=%d MOSI=%d CS=%d\n",
                  SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    _spi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN, _spi, 1000000)) {
        Serial.println("[SD] Mount failed");
        _ready = false;
        return false;
    }
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] No card inserted");
        _ready = false;
        return false;
    }
    Serial.printf("[SD] Card type: %d\n", cardType);
    _ready = true;
    return true;
}

int SDManager::listGCodeFiles(const char* dir, FileInfo* out, int maxFiles) {
    if (!_ready) return 0;

    File root = SD.open(dir);
    if (!root || !root.isDirectory()) {
        Serial.printf("[SD] Cannot open dir: %s\n", dir);
        return 0;
    }

    int count = 0;
    File entry;
    while ((entry = root.openNextFile()) && count < maxFiles) {
        if (entry.isDirectory()) continue;

        const char* name = entry.name();
        // Filter for GCode file extensions
        size_t len = strlen(name);
        bool isGCode = false;
        if (len > 3) {
            const char* ext = name + len - 3;
            if (strcasecmp(ext, ".nc") == 0 || strcasecmp(ext, ".gc") == 0) isGCode = true;
        }
        if (len > 5) {
            const char* ext = name + len - 5;
            if (strcasecmp(ext, ".gcode") == 0) isGCode = true;
        }
        if (len > 6) {
            const char* ext = name + len - 6;
            if (strcasecmp(ext, ".ngc") == 0) isGCode = true;
        }
        if (len > 4) {
            const char* ext = name + len - 4;
            if (strcasecmp(ext, ".tap") == 0 || strcasecmp(ext, ".cnc") == 0 ||
                strcasecmp(ext, ".txt") == 0) isGCode = true;
        }

        if (isGCode) {
            strncpy(out[count].name, name, MAX_FILENAME - 1);
            out[count].name[MAX_FILENAME - 1] = '\0';
            out[count].size = entry.size();
            count++;
        }
        entry.close();
    }
    root.close();
    Serial.printf("[SD] Found %d GCode files in %s\n", count, dir);
    return count;
}

bool SDManager::exists(const char* path) {
    return _ready && SD.exists(path);
}

bool SDManager::remove(const char* path) {
    if (!_ready) return false;
    return SD.remove(path);
}

size_t SDManager::fileSize(const char* path) {
    if (!_ready) return 0;
    File f = SD.open(path);
    if (!f) return 0;
    size_t s = f.size();
    f.close();
    return s;
}

File SDManager::openFile(const char* path) {
    return SD.open(path);
}

uint64_t SDManager::totalBytes() {
    return _ready ? SD.totalBytes() : 0;
}

uint64_t SDManager::usedBytes() {
    return _ready ? SD.usedBytes() : 0;
}

